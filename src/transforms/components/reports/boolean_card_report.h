#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

// Boolean-specific Arrow aggregate functions
CREATE_ENUM(BooleanArrowAggregateFunction,
  all,                 // all
  any                  // any
);

namespace epoch_script::reports {

class BooleanCardReport : public BaseCardReport {
public:
  explicit BooleanCardReport(epoch_script::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override;
};

// Template specialization for BooleanCardReport metadata
template <> struct ReportMetadata<BooleanCardReport> {
  constexpr static const char *kReportId = "boolean_cards_report";

  static epoch_script::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Boolean Cards Report",
      .options = {
        {.id = "agg",
         .name = "Aggregation",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epoch_script::MetaDataOptionDefinition{"any"},
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
         .desc = "Card position index in the row (0-based). Use sequential values: 0 for first card, 1 for second, 2 for third, etc. Within the same category, each card must have a unique group value."},
        {.id = "group_size",
         .name = "Group Size",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = true,
         .min = 1,
         .max = 10,
         .desc = "Total number of cards to display in the row. All cards in the same category row must have the same group_size."}
      },
      .isCrossSectional = false,
      .desc = "Generate a single summary card by applying a boolean Arrow aggregate function to the input column.",
      .inputs = {
        {epoch_core::IODataType::Boolean, epoch_script::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "aggregation", "summary", "boolean"},
      .requiresTimeFrame = false,
      .allowNullInputs = true
    };
  }
};

} // namespace epoch_script::reports