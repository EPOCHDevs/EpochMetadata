//
// Created by adesola on 5/16/25.
//

#pragma once

#include <epoch_script/transforms/core/itransform.h>

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>

namespace epoch_script::transform {
using namespace epoch_frame;

// Donchian Channel: upper = rolling max(high), lower = rolling min(low), middle
// = (upper + lower)/2
class DonchianChannel final : public ITransform {
public:
  explicit DonchianChannel(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;
    const auto &C = epoch_script::EpochStratifyXConstants::instance();

    const auto highArr = df[C.HIGH()].contiguous_array();
    const auto lowArr = df[C.LOW()].contiguous_array();

    const auto upperSeries = Series{highArr.value()}
                                 .to_frame()
                                 .rolling_agg({.window_size = m_window})
                                 .max()
                                 .to_series();

    const auto lowerSeries = Series{lowArr.value()}
                                 .to_frame()
                                 .rolling_agg({.window_size = m_window})
                                 .min()
                                 .to_series();

    const auto middleCA = (upperSeries + lowerSeries) * Scalar{0.5};

    return make_dataframe(
        df.index(),
        std::vector{upperSeries.array(), lowerSeries.array(), middleCA.array()},
        {GetOutputId("bbands_upper"), GetOutputId("bbands_lower"),
         GetOutputId("bbands_middle")});
  }

private:
  int64_t m_window;
};

} // namespace epoch_script::transform
