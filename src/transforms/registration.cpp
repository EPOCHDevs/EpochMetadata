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

  std::unordered_set allowTimeFrameResample{
      epoch_core::TransformCategory::Trend,
      epoch_core::TransformCategory::Momentum,
      epoch_core::TransformCategory::Volatility,
      epoch_core::TransformCategory::Volume,
      epoch_core::TransformCategory::PriceAction,
      epoch_core::TransformCategory::Statistical,
      epoch_core::TransformCategory::Factor};
  for (auto &&indicator : std::views::join(metaDataList)) {
    if (allowTimeFrameResample.contains(indicator.category)) {
      indicator.requiresTimeFrame = true;
    }

    ITransformRegistry::GetInstance().Register(indicator);
  }
}
} // namespace epoch_metadata::transforms
