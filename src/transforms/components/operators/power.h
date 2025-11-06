//
// Created by adesola
//

#pragma once
#include "epoch_frame/factory/dataframe_factory.h"
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class PowerTransform : public ITransform {
public:
  explicit PowerTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    using namespace epoch_frame;
    Series base = bars[GetInputId(epoch_script::ARG0)];
    Series exponent = bars[GetInputId(epoch_script::ARG1)];

    // Use Arrow's built-in power function
    return make_dataframe(
        base.index(),
        {arrow_utils::call_compute_array({base.array(), exponent.array()}, "power")},
        {GetOutputId()});
  }
};

} // namespace epoch_script::transform
