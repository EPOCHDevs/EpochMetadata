//
// Created by dewe on 9/11/24.
//

#pragma once

#include "epoch_metadata/transforms/metadata.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace epoch_metadata::transforms {
// Define transforms that are intraday-only (e.g., gap-related nodes)
static const std::unordered_set<std::string> kIntradayOnlyIds = {
    "gap_returns", "gap_classify"};

void RegisterStrategyMetaData(const std::string &name,
                              const TransformsMetaDataCreator &metaData);

void RegisterTransformMetadata(FileLoaderInterface const &loader);
} // namespace epoch_metadata::transforms

namespace epoch_metadata::transform {
void InitializeTransforms(
    std::function<YAML::Node(std::string const &)> const &,
    std::vector<std::string> const &, std::vector<std::string> const &);
} // namespace epoch_metadata::transform