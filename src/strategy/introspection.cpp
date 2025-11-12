//
// Created by Claude Code on 1/11/25.
//
#include "epoch_script/strategy/introspection.h"

namespace epoch_script::strategy {

namespace {
/**
 * Internal helper to get base timeframe from a GenericFunction.
 * Checks explicit timeframe first, then PythonSource compilation result.
 */
std::optional<epoch_core::BaseDataTimeFrame> _GetBaseTimeFrame(
    GenericFunction const &fn) {
  // Check explicit timeframe if provided
  if (fn.timeframe) {
    return fn.timeframe->IsIntraDay() ? epoch_core::BaseDataTimeFrame::Minute
                                      : epoch_core::BaseDataTimeFrame::EOD;
  }

  // Check PythonSource compilation result
  if (!fn.source) {
    return std::nullopt;
  }
  return fn.source->GetBaseTimeframe();
}

/**
 * Helper to check if a BaseDataTimeFrame is intraday.
 */
bool IsIntraday(std::optional<epoch_core::BaseDataTimeFrame> const &baseTF) {
  return baseTF && baseTF.value() == epoch_core::BaseDataTimeFrame::Minute;
}
} // anonymous namespace

std::optional<epoch_core::BaseDataTimeFrame> GetBaseTimeFrame(
    std::optional<GenericFunction> const &fn) {
  return fn.has_value() ? _GetBaseTimeFrame(fn.value()) : std::nullopt;
}

bool IsIntradayCampaign(StrategyConfig const &config) {
  // Check all strategy components - if ANY requires intraday, entire campaign is intraday
  // This is conservative but correct - can't mix minute and daily data without resampling
  return IsIntraday(GetBaseTimeFrame(config.trade_signal)) ||
         IsIntraday(GetBaseTimeFrame(config.position_sizer)) ||
         IsIntraday(GetBaseTimeFrame(config.take_profit)) ||
         IsIntraday(GetBaseTimeFrame(config.stop_loss));
}

} // namespace epoch_script::strategy
