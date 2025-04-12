//
// Created by adesola on 4/9/25.
//
#include "registration.h"
#include "epoch_metadata/transforms/registry.h"


namespace epoch_metadata::transforms {
     void RegisterStrategyMetaData(const std::string &name,
                                        const TransformsMetaDataCreator &metaData) {
        ITransformRegistry::GetInstance().Register(metaData(name));
    }

    void RegisterTransformMetadata(FileLoaderInterface const &loader) {
        std::vector<std::vector<TransformsMetaData>> metaDataList{
            LoadFromFile<TransformsMetaData>(loader, "transforms")};

        metaDataList.emplace_back(MakeDataSource());
        metaDataList.emplace_back(MakeMathMetaData());
        metaDataList.emplace_back(MakeComparativeMetaData());
        metaDataList.emplace_back(MakeTulipIndicators());
        // metaDataList.emplace_back(MakeTulipCandles());
        metaDataList.emplace_back(MakeTradeSignalExecutor());

        for (auto const &indicator : std::views::join(metaDataList)) {
            ITransformRegistry::GetInstance().Register(indicator);
        }
    }
}
