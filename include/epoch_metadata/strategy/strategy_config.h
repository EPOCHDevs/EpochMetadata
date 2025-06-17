//
// Created by adesola on 1/27/25.
//

#pragma once
#include "data_options.h"
#include "epoch_metadata/strategy/date_period_config.h"
#include "generic_function.h"
#include <string>

namespace epoch_metadata::strategy {
struct StrategyConfigMetaData {
  std::optional<DatePeriodConfig> allowedEODDateRange;
  std::optional<DatePeriodConfig> allowedIntradayDateRange;

  std::optional<epoch_core::BaseDataTimeFrame> baseDataTimeFrame{};
  bool operator==(const StrategyConfigMetaData &) const = default;
};

struct StrategyConfig {
  std::string name;
  std::string description;
  DataOption data;
  GenericFunction trade_signal;
  std::optional<GenericFunction> position_sizer;
  std::optional<GenericFunction> take_profit;
  std::optional<GenericFunction> stop_loss;
  std::optional<StrategyConfigMetaData> metadata{};
};

struct StrategyTemplate {
  std::string id;
  StrategyConfig strategy;
  epoch_core::TradeSignalType category;
};
} // namespace epoch_metadata::strategy