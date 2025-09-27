#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_dashboard/tearsheet/card_builder.h>
#include <epoch_dashboard/tearsheet/scalar_converter.h>
#include <epoch_frame/dataframe.h>
#include "epoch_core/enum_wrapper.h"

// Arrow aggregate functions enum
CREATE_ENUM(ArrowAggregateFunction,
  all,                 // all
  any,                 // any
  approximate_median,  // approximate_median
  count,               // count
  count_all,           // count_all
  count_distinct,      // count_distinct
  first,               // first
  first_last,          // first_last
  index,               // index
  kurtosis,            // kurtosis
  last,                // last
  max,                 // max
  mean,                // mean
  min,                 // min
  min_max,             // min_max
  mode,                // mode
  product,             // product
  quantile,            // quantile
  skew,                // skew
  stddev,              // stddev
  sum,                 // sum
  tdigest,             // tdigest
  variance             // variance
);
namespace epoch_metadata::reports {

class CardsReport : public IReporter {
public:
  explicit CardsReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config)) {}

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  void generateDashboard(const epoch_frame::DataFrame &normalizedDf) const override {
    // Dashboard generation uses the same implementation as tearsheet
    generateTearsheet(normalizedDf);
  }

private:
  // Helper methods - simplified
  std::string GetAggregation() const;
  std::string GetCategory() const;
  std::string GetTitle() const;
  uint32_t GetGroup() const;
  uint32_t GetGroupSize() const;
  epoch_proto::EpochFolioDashboardWidget GetWidgetType() const;
};

// Template specialization for CardsReport metadata
template <> struct ReportMetadata<CardsReport> {
  constexpr static const char *kReportId = "cards_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Cards Report",
      .options = {
        {.id = "agg",
         .name = "Aggregation",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"last"},
         .isRequired = false,
         .selectOption = {
                          {"All", "all"},
                          {"Any", "any"},
                          {"Approximate Median", "approximate_median"},
                          {"Count", "count"},
                          {"Count All", "count_all"},
                          {"Count Distinct", "count_distinct"},
                          {"First", "first"},
                          {"Last", "last"},
                          {"Max", "max"},
                          {"Mean", "mean"},
                          {"Min", "min"},
                          {"Mode", "mode"},
                          {"Product", "product"},
                          {"Quantile", "quantile"},
                          {"Skew", "skew"},
                          {"StdDev", "stddev"},
                          {"Sum", "sum"},
                          {"Variance", "variance"}
                        },
         .desc = "Arrow aggregate function to apply to the input series"},
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
      .desc = "Generate a single summary card by applying an Arrow aggregate function to the input column.",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "aggregation", "summary"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports