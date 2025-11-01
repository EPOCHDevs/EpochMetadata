#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

namespace epochflow::reports {

class IndexCardReport : public BaseCardReport {
public:
  explicit IndexCardReport(epochflow::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override {
    return "index";  // Always use index aggregation
  }

  // Get the target value to search for
  epoch_frame::Scalar GetTargetValue() const {
    auto options = m_config.GetOptions();
    if (options.contains("target_value") && options["target_value"].IsType(epoch_core::MetaDataOptionType::String)) {
      std::string valueStr = options["target_value"].GetString();
      // Try to parse as different types
      try {
        // Try integer first
        int64_t intValue = std::stoll(valueStr);
        return epoch_frame::Scalar(intValue);
      } catch (...) {
        try {
          // Try double
          double doubleValue = std::stod(valueStr);
          return epoch_frame::Scalar(doubleValue);
        } catch (...) {
          // Default to string
          return epoch_frame::Scalar(valueStr);
        }
      }
    }
    // Default to searching for 0
    return epoch_frame::Scalar(static_cast<int64_t>(0));
  }
};

// Template specialization for IndexCardReport metadata
template <> struct ReportMetadata<IndexCardReport> {
  constexpr static const char *kReportId = "index_cards_report";

  static epochflow::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Index Cards Report",
      .options = {
        {.id = "target_value",
         .name = "Target Value",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epochflow::MetaDataOptionDefinition{"0"},
         .isRequired = true,
         .desc = "Value to search for in the series (will return the index position)"},
        {.id = "category",
         .name = "Category",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Category name for the card group"},
        {.id = "title",
         .name = "Card Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Custom title for the card. If empty, uses 'Index of {target_value}' format"},
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
      .desc = "Find the index position of a specific value in the input series.",
      .inputs = {
        {epoch_core::IODataType::Any, epochflow::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "index", "search", "position"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epochflow::reports