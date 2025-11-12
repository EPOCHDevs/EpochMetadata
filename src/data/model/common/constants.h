#pragma once
//
// Created by dewe on 8/7/22.
//

#include <epoch_data_sdk/model/builder/asset_builder.hpp>
#include "epoch_frame/factory/date_offset_factory.h"
#include "epoch_script/core/bar_attribute.h"
#include "epoch_script/core/time_frame.h"
#include <epoch_core/macros.h>
#include <epoch_data_sdk/model/asset/asset_specification.hpp>
#include <epoch_data_sdk/model/asset/exchanges.hpp>

namespace epoch_script {

// Import SDK asset types
using data_sdk::asset::Asset;
using data_sdk::asset::AssetSpecification;
using data_sdk::asset::AssetHashMap;
using epoch_core::AssetClass;
using epoch_core::Exchange;

constexpr auto AI_GENERATED_ALGORITHMS_DIR = "ai_generated_algorithms";
constexpr auto AI_GENERATED_STRATEGIES_DIR = "ai_generated_strategies";

using Prices = AssetHashMap<decimal::Decimal>;
using Quantities = AssetHashMap<decimal::Decimal>;
constexpr size_t DAYS_IN_A_YEAR{365};

struct EpochScriptAssetConstants {
  static const EpochScriptAssetConstants &instance() {
    static EpochScriptAssetConstants instance;
    return instance;
  }
  const Asset AAPL{data_sdk::asset::MakeAsset("AAPL", AssetClass::Stocks)};
  const Asset MSFT{data_sdk::asset::MakeAsset("MSFT", AssetClass::Stocks)};
  const Asset TSLA{data_sdk::asset::MakeAsset("TSLA", AssetClass::Stocks)};
  const Asset IBM{data_sdk::asset::MakeAsset("IBM", AssetClass::Stocks)};
  const Asset AMZN{data_sdk::asset::MakeAsset("AMZN", AssetClass::Stocks)};
  const Asset GOOG{data_sdk::asset::MakeAsset("GOOG", AssetClass::Stocks)};
  const Asset AA{data_sdk::asset::MakeAsset("AA", AssetClass::Stocks)};
  const Asset SPY{data_sdk::asset::MakeAsset("SPY", AssetClass::Stocks)};
  const Asset SPX{data_sdk::asset::MakeAsset("^SPX", AssetClass::Indices)};
  const Asset DOGE_USD{
      data_sdk::asset::MakeAsset("^DOGEUSD", AssetClass::Crypto, Exchange::COINBASE)};
  const Asset BTC_USD{
      data_sdk::asset::MakeAsset("^BTCUSD", AssetClass::Crypto, Exchange::COINBASE)};
  const Asset ETH_BTC{
      data_sdk::asset::MakeAsset("^ETHBTC", AssetClass::Crypto, Exchange::COINBASE)};
  const Asset ETH_USD{
      data_sdk::asset::MakeAsset("^ETHUSD", AssetClass::Crypto, Exchange::COINBASE)};
  const Asset LTC_USD{
      data_sdk::asset::MakeAsset("^LTCUSD", AssetClass::Crypto, Exchange::COINBASE)};
  const Asset EUR_USD{
      data_sdk::asset::MakeAsset("^EURUSD", AssetClass::FX, Exchange::FX)};
  const Asset EUR_GBP{
      data_sdk::asset::MakeAsset("^EURGBP", AssetClass::FX, Exchange::FX)};
  const Asset USD_JPY{
      data_sdk::asset::MakeAsset("^USDJPY", AssetClass::FX, Exchange::FX)};
  const Asset GBP_USD{
      data_sdk::asset::MakeAsset("^GBPUSD", AssetClass::FX, Exchange::FX)};
  const Asset GC{
      data_sdk::asset::MakeAsset("GC", AssetClass::Futures, Exchange::COMEX)};
  const Asset ZC{
      data_sdk::asset::MakeAsset("ZC", AssetClass::Futures, Exchange::CBOT)};
  const Asset ES{
      data_sdk::asset::MakeAsset("ES", AssetClass::Futures, Exchange::GBLX)};
};

/**
 * Namespace for all position keys.
 */
struct PositionConstants {
  /**
   * List of all keys.
   */
  const std::vector<std::string> all{
      "avg_entry_price", "cost_basis",    "current_price",
      "market_value",    "qty",           "unrealized_pl",
      "unrealized_plpc", "lastday_price", "change_today"};
}; // namespace position_keys

/**
 * Namespace for all order keys.
 */
struct OrderConstants {
  /**
   * List of all keys.
   */
  const std::vector<std::string> all{
      "type",       "buy",         "filled_qty",  "filled_avg_price",
      "status",     "limit_price", "trail_price", "trail_percent",
      "stop_price", "hwm"};
}; // namespace order_keys

/**
 * Namespace for all account keys.
 */
struct AccountConstants {
  /**
   * List of all keys.
   */
  const std::vector<std::string> all{
      "equity",
      "cash",
      "buying_power",
      "long_market_value",
      "short_market_value",
      "maintenance_margin",
      "initial_margin",
      "available_funds",
      "prev_day_equity",
  };
}; // namespace account_keys

struct SymbolConstants {
  static const SymbolConstants &instance() {
    static SymbolConstants instance;
    return instance;
  }

  const data_sdk::Symbol AAPL{"AAPL"};
  const data_sdk::Symbol MSFT{"MSFT"};
  const data_sdk::Symbol TSLA{"TSLA"};
  const data_sdk::Symbol IBM{"IBM"};
  const data_sdk::Symbol AA{"AA"};
  const data_sdk::Symbol RTX{"RTX"};
  const data_sdk::Symbol SPY{"SPY"};
  const data_sdk::Symbol GOOG{"GOOG"};
  const data_sdk::Symbol DOGE_USD{"DOGE-USD"};
  const data_sdk::Symbol BTC_USD{"BTC-USD"};
  const data_sdk::Symbol BTC_AUD{"BTC-AUD"};
  const data_sdk::Symbol ETH_BTC{"ETH-BTC"};
  const data_sdk::Symbol BTC_ETH{"BTC-ETH"};
  const data_sdk::Symbol ETH_USD{"ETH-USD"};
  const data_sdk::Symbol EUR_USD{"EUR-USD"};
  const data_sdk::Symbol EUR_GBP{"EUR-GBP"};
  const data_sdk::Symbol GC{"GC"};
  const data_sdk::Symbol ZC{"ZC"};
  const data_sdk::Symbol ES{"ES"};
};

struct MLTags {
  static constexpr auto FEATURES_TAG = "@features/";
  static constexpr auto LABELS_TAG = "@labels/";
};
} // namespace epoch_script