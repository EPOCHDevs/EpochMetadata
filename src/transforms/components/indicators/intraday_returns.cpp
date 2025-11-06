//
// Intraday Returns Transform Implementation
//
#include "intraday_returns.h"
#include <cmath>

namespace epoch_script::transform {

IntradayReturns::IntradayReturns(const TransformConfiguration &config)
    : ITransform(config),
      m_return_type(config.GetOptionValue("return_type").GetSelectOption<epoch_core::IntradayReturnType>()) {}

[[nodiscard]] epoch_frame::DataFrame
IntradayReturns::TransformData(epoch_frame::DataFrame const &bars) const {
  // Get open and close prices from the bars DataFrame
  epoch_frame::Series open = bars["o"];
  epoch_frame::Series close = bars["c"];

  epoch_frame::Series result;

  if (m_return_type == epoch_core::IntradayReturnType::log) {
    // Log returns: log(close / open) = ln(close) - ln(open)
    result = close.ln() - open.ln();
  } else {
    // Simple returns: (close - open) / open
    result = (close - open) / open;
  }

  return MakeResult(result);
}

} // namespace epoch_script::transform
