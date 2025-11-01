#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

// Numeric-specific Arrow aggregate functions
CREATE_ENUM(NumericArrowAggregateFunction,
  approximate_median,  // approximate_median
  count_all,           // count_all
  count_distinct,      // count_distinct
  kurtosis,            // kurtosis
  max,                 // max
  mean,                // mean
  min,                 // min
  product,             // product
  skew,                // skew
  stddev,              // stddev
  sum,                 // sum
  tdigest,             // tdigest
  variance             // variance
);

namespace epochflow::reports {

class NumericCardReport : public BaseCardReport {
public:
  explicit NumericCardReport(epochflow::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override;
};

// Template specialization for NumericCardReport metadata
template <> struct ReportMetadata<NumericCardReport> {
  constexpr static const char *kReportId = "numeric_cards_report";

  static epochflow::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Numeric Cards Report",
      .options = {
        {.id = "agg",
         .name = "Aggregation",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epochflow::MetaDataOptionDefinition{"mean"},
         .isRequired = false,
         .selectOption = {
                          {"Approximate Median", "approximate_median"},
                          {"Count All", "count_all"},
                          {"Count Distinct", "count_distinct"},
                          {"Kurtosis", "kurtosis"},
                          {"Max", "max"},
                          {"Mean", "mean"},
                          {"Min", "min"},
                          {"Product", "product"},
                          {"Skew", "skew"},
                          {"StdDev", "stddev"},
                          {"Sum", "sum"},
                          {"TDigest", "tdigest"},
                          {"Variance", "variance"}
                        },
         .desc = "Numeric aggregate function to apply to the input series"},
        {.id = "category",
         .name = "Category",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Category name for the card group"},
        {.id = "title",
         .name = "Card Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Custom title for the card. If empty, uses 'aggregation(column)' format"},
        {.id = "group",
         .name = "Group",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = true,
         .min = 0,
         .max = 100,
         .desc = "Group number for this card (0-based indexing)"},
        {.id = "group_size",
         .name = "Group Size",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = true,
         .min = 1,
         .max = 10,
         .desc = "Number of cards in this group (1-indexed)"}
      },
      .isCrossSectional = false,
      .desc = "Generate a single summary card by applying a numeric Arrow aggregate function to the input column.",
      .inputs = {
        {epoch_core::IODataType::Number, ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "aggregation", "summary", "numeric"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epochflow::reports