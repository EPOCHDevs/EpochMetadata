//
// Created by dewe on 9/11/24.
//

#pragma once

#include "registry.h"
#include "metadata.h"
#include "../metadata_options.h"
#include "../doc_deserialization_helper.h"


#define REGISTER_ALGORITHM_METADATA(FactoryMetaData, FactoryMetaDataCreator) \
const int REGISTER_STRATEGY_METADATA##FactoryMetaData = RegisterStrategyMetaData(#FactoryMetaData, FactoryMetaDataCreator)

namespace metadata::transforms {
    inline int RegisterStrategyMetaData(const std::string &name, const TransformsMetaDataCreator &metaData) {
        ITransformRegistry::GetInstance().Register(metaData(name));
        return 0;
    }

    inline int RegisterMetadataList() {
        std::vector<std::vector<TransformsMetaData>> metaDataList{LoadFromFile<TransformsMetaData>("transforms")};

        metaDataList.emplace_back(MakeDataSource());
        metaDataList.emplace_back(MakeMathMetaData());
        metaDataList.emplace_back(MakeComparativeMetaData());
        metaDataList.emplace_back(MakeTulipIndicators());
        metaDataList.emplace_back(MakeTulipCandles());
        metaDataList.emplace_back(MakeTradeSignalExecutor());

        for (auto const &indicator: std::views::join(metaDataList)) {
            ITransformRegistry::GetInstance().Register(indicator);
        }
        return 0;
    }

    const int REGISTER_METADATA_LIST = RegisterMetadataList();
}