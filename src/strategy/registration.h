//
// Created by dewe on 9/11/24.
//

#pragma once

#include "../doc_deserialization_helper.h"
#include "../transforms/registration.h"
#include "epoch_metadata/strategy/metadata.h"
#include "registry.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace metadata::strategy {

const int RegisterMetadataListExitCode = [] {
  // TODO ADD FILTERS/SCREENER

  futures_continuation::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>("futures_continuation"));
  commission::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>("commission"));
  slippage::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>("slippage"));
  position_sizer::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>("position_sizer"));
  stop_loss::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>("exit_signals"));
  take_profit::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>("exit_signals"));
  trade_signal::Registry::GetInstance().Register(
      LoadFromFile<TradeSignalMetaData>("trade_signals"));
  return 0;
}();
} // namespace metadata::strategy