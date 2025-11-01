#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

// Boolean-specific Arrow aggregate functions
CREATE_ENUM(BooleanArrowAggregateFunction,
  all,                 // all
  any                  // any
);

namespace epochflow::reports {

class BooleanCardReport : public BaseCardReport {
public:
  explicit BooleanCardReport(epochflow::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override;
};

// Template specialization for BooleanCardReport metadata
template <> struct ReportMetadata<BooleanCardReport> {
  constexpr static const char *kReportId = "boolean_cards_report";

  static epochflow::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Boolean Cards Report",
      .options = {
        {.id = "agg",
         .name = "Aggregation",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epochflow::MetaDataOptionDefinition{"any"},
         .isRequired = false,
         .selectOption = {
                          {"All", "all"},
                          {"Any", "any"}
                        },
         .desc = "Boolean aggregate function to apply to the input series"},
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
      .desc = "Generate a single summary card by applying a boolean Arrow aggregate function to the input column.",
      .inputs = {
        {epoch_core::IODataType::Boolean, epochflow::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "aggregation", "summary", "boolean"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epochflow::reports