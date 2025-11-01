//
// Created by adesola on 4/9/25.
//
#include <epochflow/transforms/core/registration.h>
#include "../core/doc_deserialization_helper.h"
#include <epochflow/transforms/core/registry.h>
#include "components/sql/sql_query_metadata.h"
#include "components/data_sources/polygon_metadata.h"
#include "components/data_sources/polygon_indices_metadata.h"
#include "components/data_sources/fred_metadata.h"
#include "components/data_sources/sec_metadata.h"
#include "components/indicators/forward_returns.h"

namespace epochflow::transforms {
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
  metaDataList.emplace_back(MakeLagMetaData());
  metaDataList.emplace_back(epochflow::transform::MakeForwardReturnsMetaData());
  metaDataList.emplace_back(MakeChartFormationMetaData());
  metaDataList.emplace_back(MakeCalendarEffectMetaData());
  metaDataList.emplace_back(MakeStringTransformMetaData());
  // metaDataList.emplace_back(epochflow::transform::MakeSQLQueryMetaData()); // DISABLED
  metaDataList.emplace_back(epochflow::transform::MakePolygonDataSources());
  metaDataList.emplace_back(epochflow::transform::MakePolygonIndicesDataSources());
  metaDataList.emplace_back(epochflow::transform::MakeFREDDataSource());
  metaDataList.emplace_back(epochflow::transform::MakeSECDataSources());
  // Aggregation nodes are loaded from the transforms.yaml file

  for (auto &&indicator : std::views::join(metaDataList)) {
    if (!indicator.requiredDataSources.empty()) {
      indicator.requiresTimeFrame = true;
    }

    if (kIntradayOnlyIds.contains(indicator.id)) {
      indicator.intradayOnly = true;
    }

    if (indicator.category == epoch_core::TransformCategory::Executor) {
      indicator.allowNullInputs = true;
    }

    ITransformRegistry::GetInstance().Register(indicator);
  }
}
} // namespace epochflow::transforms
