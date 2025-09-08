//
// Created by adesola on 5/16/25.
//

#pragma once

#include "epoch_metadata/transforms/itransform.h"

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>

namespace epoch_metadata::transform {

// Ichimoku Cloud components with configurable periods
// tenkan = (rolling_max(high, p_tenkan) + rolling_min(low, p_tenkan))/2
// kijun  = (rolling_max(high, p_kijun)  + rolling_min(low, p_kijun))/2
// senkou_a = shift_fwd((tenkan + kijun)/2, p_kijun)
// senkou_b = shift_fwd((rolling_max(high, p_senkou_b) + rolling_min(low,
// p_senkou_b))/2, p_kijun) chikou = shift_back(close, p_kijun)
class Ichimoku final : public ITransform {
public:
  explicit Ichimoku(const TransformConfiguration &config)
      : ITransform(config),
        m_tenkan(config.GetOptionValue("p_tenkan").GetInteger()),
        m_kijun(config.GetOptionValue("p_kijun").GetInteger()),
        m_senkou_b(config.GetOptionValue("p_senkou_b").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();

    const epoch_frame::Series high = df[C.HIGH()];
    const epoch_frame::Series low = df[C.LOW()];
    const epoch_frame::Series close = df[C.CLOSE()];

    auto half = [](const epoch_frame::Series &a, const epoch_frame::Series &b) {
      return (a + b) * epoch_frame::Scalar{0.5};
    };

    auto roll_max = [&](const epoch_frame::Series &s, int64_t w) {
      return s.rolling_agg({.window_size = w}).max();
    };
    auto roll_min = [&](const epoch_frame::Series &s, int64_t w) {
      return s.rolling_agg({.window_size = w}).min();
    };

    const epoch_frame::Series tenkan =
        half(roll_max(high, m_tenkan), roll_min(low, m_tenkan));
    const epoch_frame::Series kijun =
        half(roll_max(high, m_kijun), roll_min(low, m_kijun));

    // Shift helpers
    const epoch_frame::Series senkou_a = half(tenkan, kijun).shift(-m_kijun);
    const epoch_frame::Series senkou_b =
        half(roll_max(high, m_senkou_b), roll_min(low, m_senkou_b))
            .shift(-m_kijun);
    const epoch_frame::Series chikou = close.shift(m_kijun);

    return make_dataframe(
        df.index(),
        std::vector{tenkan.array(), kijun.array(), senkou_a.array(),
                    senkou_b.array(), chikou.array()},
        {GetOutputId("tenkan"), GetOutputId("kijun"), GetOutputId("senkou_a"),
         GetOutputId("senkou_b"), GetOutputId("chikou")});
  }

private:
  int64_t m_tenkan;
  int64_t m_kijun;
  int64_t m_senkou_b;
};

} // namespace epoch_metadata::transform
