#pragma once
#include <epoch_script/core/constants.h>
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {
class PriceDiffVolatility : public ITransform {
public:
  explicit PriceDiffVolatility(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    auto price =
        bars[epoch_script::EpochStratifyXConstants::instance().CLOSE()];
    return price.diff()
        .rolling_agg({.window_size = m_window})
        .stddev()
        .to_frame(GetOutputId());
  }

private:
  int m_window;
};

class ReturnVolatility : public ITransform {
public:
  explicit ReturnVolatility(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    auto price =
        bars[epoch_script::EpochStratifyXConstants::instance().CLOSE()];
    return price.pct_change()
        .rolling_agg({.window_size = m_window})
        .stddev()
        .to_frame(GetOutputId());
  }

private:
  int m_window;
};
} // namespace epoch_script::transform
