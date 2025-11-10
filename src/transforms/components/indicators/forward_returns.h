#pragma once
//
// Created by adesola on 1/28/25.
//
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/metadata.h>
#include <epoch_core/enum_wrapper.h>

CREATE_ENUM(ReturnType, simple, log);

namespace epoch_script::transform {

class ForwardReturns : public ITransform {
public:
  explicit ForwardReturns(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  int m_period;
  epoch_core::ReturnType m_return_type;
};

// Metadata function
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeForwardReturnsMetaData() {
  using namespace epoch_script::transforms;
  std::vector<TransformsMetaData> metadataList;

  metadataList.emplace_back(TransformsMetaData{
      .id = "forward_returns",
      .category = epoch_core::TransformCategory::Trend,
      .plotKind = epoch_core::TransformPlotKind::panel_line,
      .name = "Forward Returns",
      .options = {
          MetaDataOption{
              .id = "period",
              .name = "Period",
              .type = epoch_core::MetaDataOptionType::Integer,
              .defaultValue = MetaDataOptionDefinition(static_cast<double>(1)),
              .min = 1,
              .desc = "Number of periods to look forward",
              .tuningGuidance = "Period 1 for next bar returns. Larger periods for longer-term forward returns. Common: 1 (next bar), 5 (next week on daily), 20 (next month). Use as target variable for machine learning predictions."
          },
          MetaDataOption{
              .id = "return_type",
              .name = "Return Type",
              .type = epoch_core::MetaDataOptionType::Select,
              .defaultValue = MetaDataOptionDefinition(std::string("simple")),
              .selectOption = {{"Simple", "simple"}, {"Log", "log"}},
              .desc = "Type of return calculation",
              .tuningGuidance = "Simple returns: (future_price - price) / price. Log returns: log(future_price / price). Log returns are more symmetric and better for ML models. Simple returns are more intuitive for interpretation."
          }
      },
      .desc = "Calculates future returns by looking ahead N periods. For each bar, computes the return from current price to future price. Essential for creating target variables in predictive models.",
      .inputs = {IOMetaDataConstants::DECIMAL_INPUT_METADATA},
      .outputs = {IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
      .tags = {"forward-looking", "returns", "prediction", "target", "machine-learning"},
      .requiresTimeFrame = false,
      .allowNullInputs = true,
      .strategyTypes = {"machine-learning", "prediction", "feature-engineering"},
      .relatedTransforms = {"lag", "return_vol", "pct_change"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Create target variables for machine learning models predicting future returns. Use as labels for supervised learning. Combine with current features to build predictive models. Common in factor research and alpha discovery.",
      .limitations = "Forward-looking transform - last N bars will be null/undefined since future data is not available. Cannot be used in live trading for the periods being predicted. Use only for backtesting and model training."
  });

  return metadataList;
}

} // namespace epoch_script::transform
