//
// Created by Claude Code on 1/11/25.
//
#include "epoch_script/strategy/introspection.h"

namespace epoch_script::strategy {
    /**
     * Internal helper to get base timeframe from a GenericFunction.
     * Checks explicit timeframe first, then PythonSource compilation result.
     */
    bool IsIntraday(
        std::optional<GenericFunction> const &fn) {
        return fn.has_value() && fn->timeframe.IsIntraDay();
    }

    bool IsIntradayCampaign(StrategyConfig const &config) {
        // Check all strategy components - if ANY requires intraday, entire campaign is intraday
        // This is conservative but correct - can't mix minute and daily data without resampling
        return (config.trade_signal.source && config.trade_signal.source->IsIntraday()) ||
               IsIntraday(config.position_sizer) ||
               IsIntraday(config.take_profit) ||
               IsIntraday(config.stop_loss);
    }
} // namespace epoch_script::strategy
