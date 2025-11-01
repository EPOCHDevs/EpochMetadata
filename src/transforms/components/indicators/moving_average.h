#pragma once
//
// Created by dewe on 4/14/23.
//
#include "../tulip/tulip_model.h"
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class MovingAverage : public ITransform {
public:
  explicit MovingAverage(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final {
    return m_model.TransformData(bars).rename(
        {{m_model.GetOutputId(), GetOutputId()}});
  }

private:
  TulipModelImpl<true> m_model;
};

} // namespace epoch_script::transform
