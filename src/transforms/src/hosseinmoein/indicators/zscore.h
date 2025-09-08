//
// Created by adesola on 5/16/25.
//

#pragma once

#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/factory/dataframe_factory.h>

namespace epoch_metadata::transform {

// Z-Score using hmdf::ZScoreVisitor over a rolling window
class ZScore final : public ITransform {
public:
  explicit ZScore(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;
    const auto input = df[GetInputId()];
    const auto series =
        input.rolling_apply({.window_size = m_window})
            .apply([&](Series const &x) {
              return (x.iloc(-1) - x.mean()) /
                     x.stddev(arrow::compute::VarianceOptions(1));
            });
    return series.to_frame(GetOutputId("result"));
  }

private:
  int64_t m_window;
};

} // namespace epoch_metadata::transform
