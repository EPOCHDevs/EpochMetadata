#pragma once
#include <epoch_script/strategy/strategy_config.h>
#include <epoch_script/strategy/enums.h>

namespace epoch_script::strategy {

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
