#pragma once
//
// Created by adesola on 1/20/25.
//
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class Lag : public ITransform {
public:
  explicit Lag(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  int m_period;
};

} // namespace epoch_script::transform