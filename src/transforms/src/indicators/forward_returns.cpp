//
// Created by adesola on 1/28/25.
//
#include "forward_returns.h"
#include <cmath>

namespace epoch_metadata::transform {

ForwardReturns::ForwardReturns(const TransformConfiguration &config)
    : ITransform(config),
      m_period(config.GetOptionValue("period").GetInteger()),
      m_return_type(config.GetOptionValue("return_type").GetSelectOption<epoch_core::ReturnType>()) {}

[[nodiscard]] epoch_frame::DataFrame
ForwardReturns::TransformData(epoch_frame::DataFrame const &bars) const {
  epoch_frame::Series price = bars[GetInputId()];
  epoch_frame::Series future_price = price.shift(-m_period);

  epoch_frame::Series result;

  if (m_return_type == epoch_core::ReturnType::log) {
    // Log returns: log(future_price / price) = ln(future_price) - ln(price)
    result = future_price.ln() - price.ln();
  } else {
    // Simple returns: (future_price - price) / price
    result = (future_price - price) / price;
  }

  return MakeResult(result);
}

} // namespace epoch_metadata::transform
