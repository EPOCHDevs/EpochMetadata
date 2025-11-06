//
// Created by adesola
//

#pragma once
#include "epoch_frame/factory/dataframe_factory.h"
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class ModuloTransform : public ITransform {
public:
  explicit ModuloTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    using namespace epoch_frame;
    Series dividend = bars[GetInputId(epoch_script::ARG0)];
    Series divisor = bars[GetInputId(epoch_script::ARG1)];

    // Implement modulo as: a % b = a - floor(a/b) * b
    // Note: This follows Python's modulo behavior for negative numbers
    auto quotient = arrow_utils::call_compute_array(
        {dividend.array(), divisor.array()}, "divide");
    auto floored = arrow_utils::call_compute_array({quotient}, "floor");
    auto product = arrow_utils::call_compute_array(
        {floored, divisor.array()}, "multiply");
    auto result = arrow_utils::call_compute_array(
        {dividend.array(), product}, "subtract");

    return make_dataframe(dividend.index(), {result}, {GetOutputId()});
  }
};

} // namespace epoch_script::transform
