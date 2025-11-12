#pragma once
#include <epoch_script/strategy/strategy_config.h>
#include <epoch_script/strategy/enums.h>

namespace epoch_script::strategy {

/**
 * Get base timeframe from a generic function configuration.
 *
 * Checks both explicit timeframe and PythonSource compilation result
 * to determine if the function requires intraday or daily data.
 *
 * @param fn Optional GenericFunction to inspect
 * @return BaseDataTimeFrame::Minute for intraday, EOD for daily, nullopt if unknown
 */
std::optional<epoch_core::BaseDataTimeFrame> GetBaseTimeFrame(
    std::optional<GenericFunction> const &fn);

/**
 * Determine if a strategy configuration requires intraday data.
 *
 * Checks ALL strategy components (trade_signal, position_sizer, take_profit, stop_loss)
 * and returns true if ANY component needs intraday data (< 1D timeframe).
 *
 * This is used to determine whether to load MinuteBars or DailyBars from the data source.
 *
 * @param config Strategy configuration to inspect
 * @return true if ANY component requires intraday data, false otherwise
 */
bool IsIntradayCampaign(StrategyConfig const &config);

} // namespace epoch_script::strategy
