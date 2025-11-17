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