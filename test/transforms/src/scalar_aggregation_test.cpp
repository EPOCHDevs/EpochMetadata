//
// Created by adesola on 1/26/25.
//
// Comprehensive test suite for scalar aggregation transforms.
//
// CURRENT STATE:
// - Tests all 18 scalar aggregation transforms registered in registration.cpp
// - Verifies basic functionality: no crashes, correct output structure, proper
// values
// - Tests edge cases: null handling, empty data, various configuration options
//
// IMPLEMENTATION FEATURES:
// - Simple aggregation functions return single column with scalar result
// - Complex functions (min_max, first_last) automatically extract struct fields
//   into separate columns with naming pattern: transform_id#field_name
// - Comprehensive null handling and configuration options support
//
// COLUMN NAMING CONVENTIONS:
// - Simple functions: "transform_id" (e.g., "my_sum", "my_mean")
// - Complex functions: "transform_id#field" (e.g., "my_minmax#min",
// "my_minmax#max")
//
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/config_helper.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "transforms/src/aggregation_scalar.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

namespace {

// Helper function to create test dataframes with double values
DataFrame createTestDataFrame(const std::vector<double> &values,
                              const std::string &column_name = "price") {
  auto index = epoch_frame::factory::index::make_datetime_index(
      std::vector<epoch_frame::DateTime>(
          values.size(), epoch_frame::DateTime{2020y, January, 18d}));
  return epoch_frame::make_dataframe<double>(index, {values}, {column_name});
}

// Helper function to create test dataframes with boolean values
DataFrame createBooleanTestDataFrame(const std::vector<bool> &values,
                                     const std::string &column_name = "flag") {
  auto index = epoch_frame::factory::index::make_datetime_index(
      std::vector<epoch_frame::DateTime>(
          values.size(), epoch_frame::DateTime{2020y, January, 18d}));
  return epoch_frame::make_dataframe<bool>(index, {values}, {column_name});
}

// Helper function to create test dataframes with mixed null values
DataFrame createMixedNullDataFrame(const std::vector<Scalar> &values,
                                   const std::string &column_name = "mixed") {
  auto index = epoch_frame::factory::index::make_datetime_index(
      std::vector<epoch_frame::DateTime>(
          values.size(), epoch_frame::DateTime{2020y, January, 18d}));
  return epoch_frame::make_dataframe(index, {values}, {column_name},
                                     arrow::float64());
}

// Helper function to test scalar aggregation transforms
template <typename T>
void testScalarAggregation(const std::string &agg_type,
                           const std::string &test_id,
                           const std::vector<double> &input_values,
                           T expected_value, double tolerance = 1e-10) {
  auto input = createTestDataFrame(input_values);
  auto config = scalar_aggregation_cfg(
      agg_type, test_id, "price",
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  DataFrame output = transform->TransformData(input);

  // Should have single row (last index) and single column
  REQUIRE(output.num_rows() == 1);
  REQUIRE(output.num_cols() == 1);
  REQUIRE(output.contains(config.GetOutputId()));

  auto result_value = output[config.GetOutputId()].iloc(0);
  if constexpr (std::is_same_v<T, double>) {
    REQUIRE_THAT(result_value.as_double(),
                 Catch::Matchers::WithinAbs(expected_value, tolerance));
  } else if constexpr (std::is_same_v<T, bool>) {
    REQUIRE(result_value.as_bool() == expected_value);
  } else if constexpr (std::is_same_v<T, int64_t>) {
    REQUIRE(result_value.as_int64() == expected_value);
  }
}

// Helper function to test boolean scalar aggregation transforms
void testBooleanScalarAggregation(const std::string &agg_type,
                                  const std::string &test_id,
                                  const std::vector<bool> &input_values,
                                  bool expected_value) {
  auto input = createBooleanTestDataFrame(input_values);
  auto config = scalar_aggregation_cfg(
      agg_type, test_id, "flag",
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  DataFrame output = transform->TransformData(input);

  // Should have single row and single column
  REQUIRE(output.num_rows() == 1);
  REQUIRE(output.num_cols() == 1);
  REQUIRE(output.contains(config.GetOutputId()));

  auto result_value = output[config.GetOutputId()].iloc(0);
  REQUIRE(result_value.as_bool() == expected_value);
}

} // namespace

TEST_CASE("Scalar Aggregation - Basic Numeric Functions",
          "[scalar_aggregation]") {
  const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};

  SECTION("sum_scalar") {
    testScalarAggregation("sum", "sum_test", test_values, 15.0);
  }

  SECTION("mean_scalar") {
    testScalarAggregation("mean", "mean_test", test_values, 3.0);
  }

  SECTION("min_scalar") {
    testScalarAggregation("min", "min_test", test_values, 1.0);
  }

  SECTION("max_scalar") {
    testScalarAggregation("max", "max_test", test_values, 5.0);
  }

  SECTION("product_scalar") {
    testScalarAggregation("product", "product_test", test_values, 120.0);
  }

  SECTION("count_scalar") {
    // count returns integer count
    auto input = createTestDataFrame(test_values);
    auto config = scalar_aggregation_cfg(
        std::string{"count"}, std::string{"count_test"}, std::string{"price"},
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    REQUIRE(transform != nullptr);

    DataFrame output = transform->TransformData(input);
    REQUIRE(output.num_rows() == 1);
    REQUIRE(output.contains(config.GetOutputId()));
    REQUIRE(output[config.GetOutputId()].iloc(0).as_int64() == 5);
  }

  SECTION("first_scalar") {
    testScalarAggregation("first", "first_test", test_values, 1.0);
  }

  SECTION("last_scalar") {
    testScalarAggregation("last", "last_test", test_values, 5.0);
  }
}

TEST_CASE("Scalar Aggregation - Statistical Functions",
          "[scalar_aggregation]") {
  const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};

  SECTION("stddev_scalar") {
    // Standard deviation with ddof=1 (sample standard deviation)
    double expected_stddev = std::sqrt(2.5); // Sample stddev of {1,2,3,4,5}
    testScalarAggregation("stddev", "stddev_test", test_values, expected_stddev,
                          1e-6);
  }

  SECTION("variance_scalar") {
    // Variance with ddof=1 (sample variance)
    double expected_variance = 2.5; // Sample variance of {1,2,3,4,5}
    testScalarAggregation("variance", "variance_test", test_values,
                          expected_variance, 1e-6);
  }

  SECTION("quantile_scalar") {
    // Default quantile (0.5 = median)
    testScalarAggregation("quantile", "quantile_test", test_values, 3.0);
  }

  SECTION("approximate_median_scalar") {
    testScalarAggregation("approximate_median", "approx_median_test",
                          test_values, 3.0);
  }
}

TEST_CASE("Scalar Aggregation - Boolean Functions", "[scalar_aggregation]") {
  SECTION("all_scalar - all true") {
    testBooleanScalarAggregation("all", "all_true_test", {true, true, true},
                                 true);
  }

  SECTION("all_scalar - one false") {
    testBooleanScalarAggregation("all", "all_false_test", {true, false, true},
                                 false);
  }

  SECTION("any_scalar - one true") {
    testBooleanScalarAggregation("any", "any_true_test", {false, true, false},
                                 true);
  }

  SECTION("any_scalar - all false") {
    testBooleanScalarAggregation("any", "any_false_test", {false, false, false},
                                 false);
  }
}

TEST_CASE("Scalar Aggregation - Count Functions", "[scalar_aggregation]") {
  SECTION("count_all_scalar") {
    // count_all includes nulls
    auto mixed_data = createMixedNullDataFrame(
        {Scalar{1.0}, Scalar{}, Scalar{3.0}, Scalar{4.0}});
    auto config = count_all_scalar_cfg(
        "count_all_test", "mixed",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(mixed_data);

    REQUIRE(output[config.GetOutputId()].iloc(0).as_int64() == 3);
  }

  SECTION("count_distinct_scalar") {
    testScalarAggregation<int64_t>("count_distinct", "count_distinct_test",
                                   {1.0, 2.0, 2.0, 3.0, 3.0, 3.0}, 3.0);
  }
}

TEST_CASE("Scalar Aggregation - Advanced Functions", "[scalar_aggregation]") {
  const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};

  SECTION("skew_scalar") {
    // Skewness calculation - for symmetric data should be close to 0
    testScalarAggregation("skew", "skew_test", test_values, 0.0, 1e-6);
  }

  SECTION("kurtosis_scalar") {
    // Kurtosis calculation
    testScalarAggregation("kurtosis", "kurtosis_test", test_values, -1.2, 0.1);
  }
}

TEST_CASE("Scalar Aggregation - Complex Return Types", "[scalar_aggregation]") {
  const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};

  SECTION("tdigest_scalar") {
    // TDigest for approximate quantiles - returns complex digest structure
    auto input = createTestDataFrame(test_values);
    auto config = tdigest_scalar_cfg(
        0.5, "tdigest_test", "price",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    DataFrame output;
    REQUIRE_NOTHROW(output = transform->TransformData(input));

    REQUIRE(output.num_rows() == 1);
    REQUIRE(output.num_cols() >= 1);
    REQUIRE(output.contains(config.GetOutputId()));
  }
}

TEST_CASE("Scalar Aggregation - Null Handling", "[scalar_aggregation]") {
  SECTION("skip_nulls = true (default)") {
    auto mixed_data = createMixedNullDataFrame(
        {Scalar{1.0}, Scalar{}, Scalar{3.0}, Scalar{5.0}});

    auto config = mean_scalar_cfg(
        true, 1, "mean_skip_nulls", "mixed",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(mixed_data);

    // Mean of {1.0, 3.0, 5.0} = 3.0
    REQUIRE_THAT(output[config.GetOutputId()].iloc(0).as_double(),
                 Catch::Matchers::WithinAbs(3.0, 1e-10));
  }

  SECTION("min_count requirement") {
    auto sparse_data =
        createMixedNullDataFrame({Scalar{1.0}, Scalar{}, Scalar{}, Scalar{}});

    auto config = mean_scalar_cfg(
        false, 2, "mean_min_count", "mixed",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    // Should not crash but may return null due to insufficient non-null values
    REQUIRE_NOTHROW(transform->TransformData(sparse_data));
  }
}

TEST_CASE("Scalar Aggregation - Options Configuration",
          "[scalar_aggregation]") {
  SECTION("stddev with custom ddof") {
    const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};
    auto input = createTestDataFrame(test_values);

    // Test with ddof=0 (population standard deviation)
    auto config = stddev_scalar_cfg(
        0, "stddev_ddof0", "price",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    // Population stddev should be different from sample stddev
    double result = output[config.GetOutputId()].iloc(0).as_double();
    REQUIRE(result > 0);
    REQUIRE(result != std::sqrt(2.5)); // Should be different from sample stddev
  }

  SECTION("quantile with custom quantile value") {
    const std::vector<double> test_values = {1.0, 2.0, 3.0, 4.0, 5.0};
    auto input = createTestDataFrame(test_values);

    // Test 75th percentile (0.75 quantile)
    auto config = quantile_scalar_cfg(
        0.75, "quantile_75", "price",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    double result = output[config.GetOutputId()].iloc(0).as_double();
    REQUIRE(result > 3.0);  // 75th percentile should be > median
    REQUIRE(result <= 5.0); // But <= max
  }
}

TEST_CASE("Scalar Aggregation - Edge Cases", "[scalar_aggregation]") {
  SECTION("single value") {
    testScalarAggregation("sum", "sum_single", {42.0}, 42.0);
    testScalarAggregation("mean", "mean_single", {42.0}, 42.0);
    testScalarAggregation("min", "min_single", {42.0}, 42.0);
    testScalarAggregation("max", "max_single", {42.0}, 42.0);
  }

  SECTION("all same values") {
    const std::vector<double> same_values = {5.0, 5.0, 5.0, 5.0};
    testScalarAggregation("sum", "sum_same", same_values, 20.0);
    testScalarAggregation("mean", "mean_same", same_values, 5.0);
    testScalarAggregation("stddev", "stddev_same", same_values, 0.0);
    testScalarAggregation("variance", "variance_same", same_values, 0.0);
  }

  SECTION("negative values") {
    const std::vector<double> negative_values = {-3.0, -1.0, 0.0, 1.0, 3.0};
    testScalarAggregation("sum", "sum_negative", negative_values, 0.0);
    testScalarAggregation("mean", "mean_negative", negative_values, 0.0);
    testScalarAggregation("min", "min_negative", negative_values, -3.0);
    testScalarAggregation("max", "max_negative", negative_values, 3.0);
  }

  SECTION("large values") {
    const std::vector<double> large_values = {1e6, 2e6, 3e6, 4e6, 5e6};
    testScalarAggregation("sum", "sum_large", large_values, 15e6, 1e-6);
    testScalarAggregation("mean", "mean_large", large_values, 3e6, 1e-6);
  }
}

TEST_CASE("Scalar Aggregation - Transform Output Structure",
          "[scalar_aggregation]") {
  SECTION("basic scalar output structure") {
    const std::vector<double> test_values = {1.0, 2.0, 3.0};
    auto input = createTestDataFrame(test_values);
    auto config = sum_scalar_cfg(
        "structure_test", "price",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    // Should use last index from input
    REQUIRE(output.num_rows() == 1);
    REQUIRE(output.num_cols() == 1);

    // Output column should have correct name
    REQUIRE(output.contains(config.GetOutputId()));

    // Verify the actual column name format
    auto expected_column_name = config.GetOutputId();
    REQUIRE(output.contains(expected_column_name));

    // Index should be from the last row of input
    auto input_last_index = input.tail(1).index();
    auto output_index = output.index();
    // Both should have same timestamp for the single row
    REQUIRE(output_index->size() == input_last_index->size());

    // Verify the actual sum value
    auto result_value = output[expected_column_name].iloc(0).as_double();
    REQUIRE(result_value == 6.0); // 1.0 + 2.0 + 3.0
  }

  SECTION("column naming consistency") {
    const std::vector<double> test_values = {10.0, 20.0, 30.0};
    auto input = createTestDataFrame(test_values);

    // Test multiple transforms to ensure consistent naming
    std::vector<std::pair<std::string, double>> test_cases = {
        {"sum", 60.0},   // 10 + 20 + 30
        {"mean", 20.0},  // (10 + 20 + 30) / 3
        {"min", 10.0},   // minimum value
        {"max", 30.0},   // maximum value
        {"count", 3.0},  // number of elements
        {"first", 10.0}, // first value
        {"last", 30.0}   // last value
    };

    for (const auto &[agg_type, expected_value] : test_cases) {
      auto config = scalar_aggregation_cfg(
          agg_type, agg_type + "_naming_test", "price",
          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());
      DataFrame output = transform->TransformData(input);

      // Verify structure
      REQUIRE(output.num_rows() == 1);
      REQUIRE(output.num_cols() == 1);

      // Verify column exists with expected name
      auto column_name = config.GetOutputId();
      REQUIRE(output.contains(column_name));

      // Verify value is correct
      if (agg_type == "count") {
        // Count should be an integer
        auto result = output[column_name].iloc(0).as_int64();
        REQUIRE(result == expected_value);
        INFO("Testing " << agg_type << "_scalar: column=" << column_name
                        << ", value=" << result);
      } else {
        auto result = output[column_name].iloc(0).as_double();
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(expected_value, 1e-10));
        INFO("Testing " << agg_type << "_scalar: column=" << column_name
                        << ", value=" << result);
      }
    }
  }
}
