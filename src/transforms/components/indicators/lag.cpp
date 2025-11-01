//
// Created by adesola on 1/20/25.
//
#include "lag.h"

namespace epochflow::transform {

Lag::Lag(const TransformConfiguration &config)
    : ITransform(config),
      m_period(config.GetOptionValue("period").GetInteger()) {}

[[nodiscard]] epoch_frame::DataFrame
Lag::TransformData(epoch_frame::DataFrame const &bars) const {
  epoch_frame::Series input = bars[GetInputId()];
  epoch_frame::Series result = input.shift(m_period);
  return MakeResult(result);
}

} // namespace epochflow::transform