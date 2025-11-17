//
// Created by dewe on 9/11/24.
//

#pragma once

#include "metadata.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace epoch_script::transforms {
// Define transforms that are intraday-only
// These transforms default to 1Min timeframe when no explicit timeframe is provided
static const std::unordered_set<std::string> kIntradayOnlyIds = {
    "session_time_window"};

void RegisterStrategyMetaData(const std::string &name,
                              const TransformsMetaDataCreator &metaData);

void RegisterTransformMetadata(FileLoaderInterface const &loader);
} // namespace epoch_script::transforms

namespace epoch_script::transform {
void InitializeTransforms(
    std::function<YAML::Node(std::string const &)> const &,
    std::vector<std::string> const &, std::vector<std::string> const &);
} // namespace epoch_script::transform