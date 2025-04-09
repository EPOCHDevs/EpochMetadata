//
// Created by dewe on 9/11/24.
//
#include "registration.h"

#include <transforms/registration.h>

#include "../doc_deserialization_helper.h"
#include "epoch_metadata/strategy/metadata.h"
#include "registry.h"

namespace epoch_metadata::strategy {

void RegisterStrategyMetadata(FileLoaderInterface const &loader) {
    transforms::RegisterTransformMetadata(loader);
  // TODO ADD FILTERS/SCREENER

  futures_continuation::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "futures_continuation"));
  commission::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "commission"));
  slippage::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "slippage"));
  position_sizer::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "position_sizer"));
  stop_loss::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "exit_signals"));
  take_profit::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "exit_signals"));
  trade_signal::Registry::GetInstance().Register(
      LoadFromFile<TradeSignalMetaData>(loader, "trade_signals"));
}

} // namespace epoch_metadata::strategy