#pragma once

#include "base_card_report.h"
#include "epoch_core/enum_wrapper.h"

namespace epoch_script::reports {

class QuantileCardReport : public BaseCardReport {
public:
  explicit QuantileCardReport(epoch_script::transform::TransformConfiguration config)
      : BaseCardReport(std::move(config)) {}

protected:
  std::string GetAggregation() const override {
    return "quantile";  // Always use quantile aggregation
  }

  // Get the quantile value (0.0 to 1.0)
  double GetQuantileValue() const {
    auto options = m_config.GetOptions();
    if (options.contains("quantile") && (options["quantile"].IsType(epoch_core::MetaDataOptionType::Decimal) || options["quantile"].IsType(epoch_core::MetaDataOptionType::Integer))) {
      double value = options["quantile"].GetDecimal();
      // Ensure it's in valid range [0.0, 1.0]
      if (value < 0.0) value = 0.0;
      if (value > 1.0) value = 1.0;
      return value;
    }
    return 0.5;  // Default to median
  }

  // Get interpolation method
  std::string GetInterpolationMethod() const {
    auto options = m_config.GetOptions();
    if (options.contains("interpolation") && options["interpolation"].IsType(epoch_core::MetaDataOptionType::Select)) {
      return options["interpolation"].GetSelectOption();
    }
    return "linear";  // Default interpolation
  }
};

// Template specialization for QuantileCardReport metadata
template <> struct ReportMetadata<QuantileCardReport> {
  constexpr static const char *kReportId = "quantile_cards_report";

  static epoch_script::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Quantile Cards Report",
      .options = {
        {.id = "quantile",
         .name = "Quantile",
         .type = epoch_core::MetaDataOptionType::Decimal,
         .defaultValue = epoch_script::MetaDataOptionDefinition{0.5},
         .isRequired = true,
         .min = 0.0,
         .max = 1.0,
         .desc = "Quantile value between 0.0 and 1.0 (0.5 = median, 0.25 = Q1, 0.75 = Q3)"},
        {.id = "interpolation",
         .name = "Interpolation Method",
         .type = epoch_core::MetaDataOptionType::Select,
         .defaultValue = epoch_script::MetaDataOptionDefinition{"linear"},
         .isRequired = false,
         .selectOption = {
                          {"Linear", "linear"},
                          {"Lower", "lower"},
                          {"Higher", "higher"},
                          {"Midpoint", "midpoint"},
                          {"Nearest", "nearest"}
                        },
         .desc = "Interpolation method for calculating quantiles"},
        {.id = "category",
         .name = "Category",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Category name for the card group"},
        {.id = "title",
         .name = "Card Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "Custom title for the card. If empty, uses 'Quantile {value}' format"}
      },
      .isCrossSectional = false,
      .desc = "Calculate a specific quantile (percentile) of the input numeric series.",
      .inputs = {
        {epoch_core::IODataType::Number, epoch_script::ARG}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "cards", "quantile", "percentile", "statistics"},
      .requiresTimeFrame = false,
      .allowNullInputs = true
    };
  }
};

} // namespace epoch_script::reports