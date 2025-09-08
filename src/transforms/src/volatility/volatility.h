#pragma once
#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {
class PriceDiffVolatility : public ITransform {
public:
  explicit PriceDiffVolatility(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    auto price =
        bars[epoch_metadata::EpochStratifyXConstants::instance().CLOSE()];
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
        bars[epoch_metadata::EpochStratifyXConstants::instance().CLOSE()];
    return price.pct_change()
        .rolling_agg({.window_size = m_window})
        .stddev()
        .to_frame(GetOutputId());
  }

private:
  int m_window;
};
} // namespace epoch_metadata::transform
