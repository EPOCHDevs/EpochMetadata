//
// Created by dewe on 9/11/24.
//

#pragma once
#include "epoch_metadata/constants.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace epoch_metadata::strategy {
void RegisterStrategyMetadata(
    FileLoaderInterface const &loader,
    std::vector<std::string> const &aiGeneratedAlgorithms,
    std::vector<std::string> const &aiGeneratedStrategies);

} // namespace epoch_metadata::strategy