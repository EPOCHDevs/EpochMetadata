//
// Created by adesola on 9/1/24.
//

#pragma once
#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {
struct GapReturns : ITransform {
  GapReturns(const TransformConfiguration &config) : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final;
};
} // namespace epoch_metadata::transform