#pragma once
//
// Intraday Returns Transform
//
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/metadata.h>
#include <epoch_core/enum_wrapper.h>

CREATE_ENUM(IntradayReturnType, simple, log);

namespace epoch_script::transform {

class IntradayReturns : public ITransform {
public:
  explicit IntradayReturns(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::IntradayReturnType m_return_type;
};

// Metadata function
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeIntradayReturnsMetaData() {
  using namespace epoch_script::transforms;
  std::vector<TransformsMetaData> metadataList;

  metadataList.emplace_back(TransformsMetaData{
      .id = "intraday_returns",
      .category = epoch_core::TransformCategory::Trend,
      .plotKind = epoch_core::TransformPlotKind::line,
      .name = "Intraday Returns",
      .options = {
          MetaDataOption{
              .id = "return_type",
              .name = "Return Type",
              .type = epoch_core::MetaDataOptionType::Select,
              .defaultValue = MetaDataOptionDefinition(std::string("simple")),
              .selectOption = {{"Simple", "simple"}, {"Log", "log"}},
              .desc = "Type of return calculation",
              .tuningGuidance = "Simple returns: (close - open) / open. Log returns: log(close / open). Log returns are symmetric and better for statistical analysis. Simple returns are more intuitive for interpretation and sum across assets."
          }
      },
      .desc = "Calculates the return from open to close within each bar. Measures intraday price movement, capturing the performance within each time period. Essential for analyzing intrabar volatility and directional bias.",
      .inputs = {},
      .outputs = {IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
      .tags = {"intraday", "returns", "volatility", "directional", "open-close"},
      .requiresTimeFrame = false,
      .requiredDataSources = {"o", "c"},
      .allowNullInputs = true,
      .strategyTypes = {"volatility", "mean-reversion", "trend", "statistical-analysis"},
      .relatedTransforms = {"forward_returns", "pct_change", "return_vol", "bar_gap"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Measure intraday price movement and directional bias. Analyze gap vs. intraday follow-through. Identify high volatility periods. Create features for intraday trading strategies. Compare open-to-close returns across timeframes or assets.",
      .limitations = "Requires OHLC data with distinct open and close prices. Will return zero for data where open equals close. Does not account for high/low extremes within the bar."
  });

  return metadataList;
}

} // namespace epoch_script::transform
