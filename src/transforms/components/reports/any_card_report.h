#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

// Any-type Arrow aggregate functions
CREATE_ENUM(AnyArrowAggregateFunction,
  first,               // first
  last                 // last
);

namespace epoch_script::reports {

class AnyCardReport : public BaseCardReport {
public:
  explicit AnyCardReport(epoch_script::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override;
};

// Template specialization for AnyCardReport metadata
template <> struct ReportMetadata<AnyCardReport> {
  constexpr static const char *kReportId = "any_cards_report";

  static epoch_script::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Any Type Cards Report",
      .options = {
        {.id = "agg",
         .name = "Aggregation",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epoch_script::MetaDataOptionDefinition{"last"},
         .isRequired = false,
         .selectOption = {
                          {"First", "first"},
                          {"Last", "last"}
                        },
         .desc = "Generic aggregate function to apply to the input series"},
        {.id = "category",
         .name = "Category",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Category name for the card group"},
        {.id = "title",
         .name = "Card Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Custom title for the card. If empty, uses 'aggregation(column)' format"}
      },
      .isCrossSectional = false,
      .desc = "Generate a single summary card by applying a generic Arrow aggregate function to the input column.",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_script::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "aggregation", "summary", "generic"},
      .requiresTimeFrame = false,
      .allowNullInputs = true
    };
  }
};

} // namespace epoch_script::reports