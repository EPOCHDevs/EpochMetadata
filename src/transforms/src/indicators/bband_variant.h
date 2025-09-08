#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_metadata/bar_attribute.h>

namespace epoch_metadata::transform {
struct BollingerBandsPercent : ITransform {
  explicit BollingerBandsPercent(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final {
    const auto lower = bars[GetInputId("bbands_lower")];
    const auto upper = bars[GetInputId("bbands_upper")];
    return ((bars[epoch_metadata::EpochStratifyXConstants::instance().CLOSE()] -
             lower) /
            (upper - lower))
        .to_frame(GetOutputId());
  }
};

struct BollingerBandsWidth : ITransform {
  explicit BollingerBandsWidth(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final {
    const auto lower = bars[GetInputId("bbands_lower")];
    const auto middle = bars[GetInputId("bbands_middle")];
    const auto upper = bars[GetInputId("bbands_upper")];
    return ((upper - lower) / middle).to_frame(GetOutputId());
  }
};
} // namespace epoch_metadata::transform
