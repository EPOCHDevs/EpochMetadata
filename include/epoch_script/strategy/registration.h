//
// Created by dewe on 9/11/24.
//

#pragma once
#include "epoch_script/core/constants.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace epoch_script::strategy {
void RegisterStrategyMetadata(
    FileLoaderInterface const &loader,
    std::vector<std::string> const &aiGeneratedAlgorithms,
    std::vector<std::string> const &aiGeneratedStrategies);

} // namespace epoch_script::strategy