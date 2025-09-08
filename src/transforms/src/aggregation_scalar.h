#pragma once
//
// Created by adesola on 1/26/25.
//
#include "epoch_metadata/transforms/itransform.h"
#include <arrow/compute/api.h>
#include <arrow/scalar.h>
#include <arrow/type.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>

namespace epoch_metadata::transform {

/**
 * Templated AggregationScalar transform that applies specific Apache Arrow
 * aggregation functions and returns a scalar result as a DataFrame using the
 * last index
 */
template <const char *FunctionName>
class AggregationScalar : public ITransform {
public:
  explicit AggregationScalar(const TransformConfiguration &config)
      : ITransform(config) {
    if constexpr (RequiresSkipNull()) {
      m_skip_nulls =
          config
              .GetOptionValue("skip_nulls",
                              epoch_metadata::MetaDataOptionDefinition{true})
              .GetBoolean();
    }

    if constexpr (RequiresMinCount()) {
      m_min_count =
          config
              .GetOptionValue("min_count",
                              epoch_metadata::MetaDataOptionDefinition{1.0})
              .GetInteger();
    }

    // Additional options for specific aggregation functions
    if constexpr (RequiresDDOF()) {
      m_ddof = config
                   .GetOptionValue(
                       "ddof", epoch_metadata::MetaDataOptionDefinition{1.0})
                   .GetInteger();
    }
    if constexpr (RequiresQuantile()) {
      m_quantile =
          config
              .GetOptionValue("quantile",
                              epoch_metadata::MetaDataOptionDefinition{0.5})
              .GetDecimal();
    }
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    // Get input data
    auto input_series = bars[GetInputId()];
    auto input_array = input_series.array();

    // Create appropriate FunctionOptions based on the function
    auto options = CreateFunctionOptions();

    // Call the aggregation function
    // count_all expects 0 arguments in Arrow; map it to count with ALL mode
    const char *fnToCall = (std::string_view(FunctionName) == "count_all")
                               ? "count"
                               : FunctionName;
    auto result =
        arrow::compute::CallFunction(fnToCall, {input_array}, options.get());

    if (!result.ok()) {
      throw std::runtime_error("Aggregation failed: " +
                               result.status().ToString());
    }

    // Extract scalar value
    std::shared_ptr<arrow::Scalar> scalar_result;
    if (result->is_array()) {
      auto arr = result->make_array();
      AssertFromStream(arr->length() == 1, "Expected scalar result");
      scalar_result = arr->GetScalar(0).MoveValueUnsafe();
    } else {
      scalar_result = result->scalar();
    }

    // Create a single-row DataFrame with the last index
    auto last_row_df = bars.tail(1);

    // Handle simple scalar types (most aggregation functions)
    auto result_array =
        arrow::MakeArrayFromScalar(*scalar_result, 1).ValueOrDie();

    // Create arrow table with the result
    auto field = arrow::field(GetOutputId(), result_array->type());
    auto schema = arrow::schema({field});
    auto table = arrow::Table::Make(schema, {result_array});

    return epoch_frame::DataFrame(last_row_df.index(), table);
  }

private:
  bool m_skip_nulls = true;
  int64_t m_min_count = 1;

  // Optional parameters
  int64_t m_ddof = 0;
  double m_quantile = 0.5;

  static constexpr bool RequiresDDOF() {
    return std::string_view(FunctionName) == "stddev" ||
           std::string_view(FunctionName) == "variance";
  }

  static constexpr bool RequiresQuantile() {
    return std::string_view(FunctionName) == "quantile";
  }

  static constexpr bool RequiresMinCount() {
    return !(std::string_view(FunctionName) == "count" ||
             std::string_view(FunctionName) == "count_all" ||
             std::string_view(FunctionName) == "count_distinct" ||
             std::string_view(FunctionName) == "skew" ||
             std::string_view(FunctionName) == "kurtosis");
  }

  static constexpr bool RequiresSkipNull() {
    return !(std::string_view(FunctionName) == "count_all" ||
             std::string_view(FunctionName) == "count_distinct" ||
             std::string_view(FunctionName) == "skew" ||
             std::string_view(FunctionName) == "kurtosis");
  }

  std::shared_ptr<arrow::compute::FunctionOptions>
  CreateFunctionOptions() const {
    using namespace arrow::compute;

    if constexpr (std::string_view(FunctionName) == "all" ||
                  std::string_view(FunctionName) == "any" ||
                  std::string_view(FunctionName) == "approximate_median" ||
                  std::string_view(FunctionName) == "first" ||
                  std::string_view(FunctionName) == "last" ||
                  std::string_view(FunctionName) == "max" ||
                  std::string_view(FunctionName) == "mean" ||
                  std::string_view(FunctionName) == "min" ||
                  std::string_view(FunctionName) == "product" ||
                  std::string_view(FunctionName) == "sum") {
      return std::make_shared<ScalarAggregateOptions>(m_skip_nulls,
                                                      m_min_count);
    } else if constexpr (std::string_view(FunctionName) == "count" ||
                         std::string_view(FunctionName) == "count_all" ||
                         std::string_view(FunctionName) == "count_distinct") {
      auto mode = m_skip_nulls ? CountOptions::ONLY_VALID : CountOptions::ALL;
      return std::make_shared<CountOptions>(mode);
    } else if constexpr (std::string_view(FunctionName) == "mode") {
      return std::make_shared<ModeOptions>(1, m_skip_nulls, m_min_count);
    } else if constexpr (std::string_view(FunctionName) == "quantile") {
      return std::make_shared<QuantileOptions>(std::vector<double>{m_quantile},
                                               QuantileOptions::LINEAR,
                                               m_skip_nulls, m_min_count);
    } else if constexpr (std::string_view(FunctionName) == "stddev" ||
                         std::string_view(FunctionName) == "variance") {
      return std::make_shared<VarianceOptions>(m_ddof, m_skip_nulls,
                                               m_min_count);
    } else if constexpr (std::string_view(FunctionName) == "tdigest") {
      return std::make_shared<TDigestOptions>(std::vector<double>{m_quantile},
                                              100, m_skip_nulls, m_min_count);
    } else if constexpr (std::string_view(FunctionName) == "skew" ||
                         std::string_view(FunctionName) == "kurtosis") {
      return std::make_shared<SkewOptions>(m_skip_nulls, true, m_min_count);
    } else {
      // Default to ScalarAggregateOptions
      return std::make_shared<ScalarAggregateOptions>(m_skip_nulls,
                                                      m_min_count);
    }
  }
};

// Define function name constants for template instantiation
inline constexpr char ALL[] = "all";
inline constexpr char ANY[] = "any";
inline constexpr char APPROXIMATE_MEDIAN[] = "approximate_median";
inline constexpr char COUNT[] = "count";
inline constexpr char COUNT_ALL[] = "count_all";
inline constexpr char COUNT_DISTINCT[] = "count_distinct";
inline constexpr char FIRST[] = "first";
inline constexpr char INDEX[] = "index";
inline constexpr char KURTOSIS[] = "kurtosis";
inline constexpr char LAST[] = "last";
inline constexpr char MAX[] = "max";
inline constexpr char MEAN[] = "mean";
inline constexpr char MIN[] = "min";
inline constexpr char PRODUCT[] = "product";
inline constexpr char QUANTILE[] = "quantile";
inline constexpr char SKEW[] = "skew";
inline constexpr char STDDEV[] = "stddev";
inline constexpr char SUM[] = "sum";
inline constexpr char TDIGEST[] = "tdigest";
inline constexpr char VARIANCE[] = "variance";

// Type aliases for specific aggregation functions
using AllAggregation = AggregationScalar<ALL>;
using AnyAggregation = AggregationScalar<ANY>;
using ApproximateMedianAggregation = AggregationScalar<APPROXIMATE_MEDIAN>;
using CountAggregation = AggregationScalar<COUNT>;
using CountAllAggregation = AggregationScalar<COUNT_ALL>;
using CountDistinctAggregation = AggregationScalar<COUNT_DISTINCT>;
using FirstAggregation = AggregationScalar<FIRST>;
using KurtosisAggregation = AggregationScalar<KURTOSIS>;
using LastAggregation = AggregationScalar<LAST>;
using MaxAggregation = AggregationScalar<MAX>;
using MeanAggregation = AggregationScalar<MEAN>;
using MinAggregation = AggregationScalar<MIN>;
using ProductAggregation = AggregationScalar<PRODUCT>;
using QuantileAggregation = AggregationScalar<QUANTILE>;
using SkewAggregation = AggregationScalar<SKEW>;
using StddevAggregation = AggregationScalar<STDDEV>;
using SumAggregation = AggregationScalar<SUM>;
using TDigestAggregation = AggregationScalar<TDIGEST>;
using VarianceAggregation = AggregationScalar<VARIANCE>;

} // namespace epoch_metadata::transform
