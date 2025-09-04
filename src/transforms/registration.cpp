//
// Created by adesola on 4/9/25.
//
#include "epoch_metadata/transforms/registration.h"
#include "../doc_deserialization_helper.h"
#include "epoch_metadata/transforms/registry.h"
#include <unordered_map>

namespace epoch_metadata::transforms {
void RegisterStrategyMetaData(const std::string &name,
                              const TransformsMetaDataCreator &metaData) {
  ITransformRegistry::GetInstance().Register(metaData(name));
}

void RegisterTransformMetadata(FileLoaderInterface const &loader) {
  std::vector<std::vector<TransformsMetaData>> metaDataList{
      LoadFromFile<TransformsMetaData>(loader, "transforms")};

  metaDataList.emplace_back(MakeDataSource());
  metaDataList.emplace_back(MakeComparativeMetaData());
  metaDataList.emplace_back(MakeTulipIndicators());
  metaDataList.emplace_back(MakeTulipCandles());
  metaDataList.emplace_back(MakeTradeSignalExecutor());
  metaDataList.emplace_back(MakeScalarMetaData());
  // Aggregation nodes are loaded from the transforms.yaml file

  // Define transforms that are intraday-only (e.g., gap-related nodes)
  static const std::unordered_set<std::string> kIntradayOnlyIds = {
      "gap_returns", "gap_classify"};

  for (auto &&indicator : std::views::join(metaDataList)) {
    if (!indicator.requiredDataSources.empty()) {
      indicator.requiresTimeFrame = true;
    }

    if (kIntradayOnlyIds.contains(indicator.id)) {
      indicator.intradayOnly = true;
    }

    ITransformRegistry::GetInstance().Register(indicator);
  }
}
} // namespace epoch_metadata::transforms
