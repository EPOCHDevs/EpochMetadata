#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {

struct CumProdOperation : ITransform {
  explicit CumProdOperation(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    return bars[GetInputId()].cumulative_prod().to_frame(GetOutputId());
  }
};

} // namespace epoch_metadata::transform
