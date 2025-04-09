//
// Created by dewe on 9/11/24.
//

#pragma once

#include "../doc_deserialization_helper.h"
#include "epoch_metadata/transforms/metadata.h"

#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator)   \
  const int REGISTER_STRATEGY_METADATA##FactoryMetaData =                      \
      RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace epoch_metadata::transforms {
    void RegisterStrategyMetaData(const std::string &name,
                                        const TransformsMetaDataCreator &metaData);

    void RegisterTransformMetadata(FileLoaderInterface const &loader);
} // namespace epoch_metadata::transforms