#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_data_sdk/dataloader/options.hpp>
#include <unordered_map>

namespace epoch_script::transform {

// Transform for FRED economic indicators
// Auto-derives date range from input DataFrame and returns publication events
// Requires external loader to provide FRED data via TransformData input parameter
class FREDTransform final : public ITransform {
public:
  explicit FREDTransform(const TransformConfiguration &config)
      : ITransform(config),
        m_indicator(config.GetOptionValue("category")
                        .GetSelectOption<epoch_core::MacroEconomicsIndicator>()) {
    // Build column rename mapping from FRED API field names to output IDs
    for (auto const &outputMetaData : config.GetOutputs()) {
      m_replacements[outputMetaData.id] = config.GetOutputId(outputMetaData.id);
    }
  }

  [[nodiscard]] epoch_core::MacroEconomicsIndicator GetIndicator() const {
    return m_indicator;
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &fred_data) const override {
    // External loader has already:
    // 1. Extracted backtest date range from market data
    // 2. Called FRED API with published_from/published_to = backtest range
    // 3. Returned DataFrame indexed by published_at with columns: observation_date, value
    // 4. Reindexed to match market data timeline (values only on publication dates)
    //
    // We just rename columns to match the node's output IDs
    return fred_data.rename(m_replacements);
  }

private:
  epoch_core::MacroEconomicsIndicator m_indicator;
  std::unordered_map<std::string, std::string> m_replacements;
};

// Category to FRED Series ID mapping for external loader reference
// External loader uses this to map user's category selection to FRED API series_id
inline const std::unordered_map<std::string, std::string> FRED_SERIES_MAP{
    // Inflation Indicators
    {"CPI", "CPIAUCSL"},
    {"CoreCPI", "CPILFESL"},
    {"PCE", "PCEPI"},
    {"CorePCE", "PCEPILFE"},

    // Interest Rates & Monetary Policy
    {"FedFunds", "DFF"},
    {"Treasury3M", "DTB3"},
    {"Treasury2Y", "DGS2"},
    {"Treasury5Y", "DGS5"},
    {"Treasury10Y", "DGS10"},
    {"Treasury30Y", "DGS30"},

    // Employment & Labor Market
    {"Unemployment", "UNRATE"},
    {"NonfarmPayrolls", "PAYEMS"},
    {"InitialClaims", "ICSA"},

    // Economic Growth & Production
    {"GDP", "GDPC1"},
    {"IndustrialProduction", "INDPRO"},
    {"RetailSales", "RSXFS"},
    {"HousingStarts", "HOUST"},

    // Market Sentiment & Money Supply
    {"ConsumerSentiment", "UMCSENT"},
    {"M2", "M2SL"},
    {"SP500", "SP500"},
    {"VIX", "VIXCLS"},
};

} // namespace epoch_script::transform
