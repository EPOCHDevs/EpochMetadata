#include "intermediate_storage.h"
#include "common/asserts.h"
#include "epoch_frame/common.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_metadata/transforms/metadata.h"
#include "storage_types.h"
#include <algorithm>
#include <arrow/table.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <ranges>
#include <vector>

namespace epoch_flow::runtime {
epoch_frame::DataFrame IntermediateResultStorage::GatherInputs(
    const AssetID &asset_id,
    const epoch_metadata::transform::ITransformBase &transformer) const {
  const auto targetTimeframe = transformer.GetTimeframe().ToString();
  const auto dataSources = transformer.GetConfiguration()
                               .GetTransformDefinition()
                               .GetMetadata()
                               .requiredDataSources;

  const auto transformInputs = transformer.GetInputIds();

  if (transformInputs.empty()) {
    SPDLOG_DEBUG(
        "Gathering base data for asset: {}, timeframe {}, transform: {}.",
        asset_id, targetTimeframe, transformer.GetId());
    std::shared_lock lock(m_baseDataMutex);
    return epoch_core::lookup(epoch_core::lookup(m_baseData, targetTimeframe), asset_id);
  }

  // Acquire read locks for both cache and baseData
  std::shared_lock cacheLock(m_cacheMutex);
  std::shared_lock baseDataLock(m_baseDataMutex);
  std::shared_lock transformMapLock(m_transformMapMutex);

  auto targetIndex =
      epoch_core::lookup(epoch_core::lookup(m_baseData, targetTimeframe,
                    "Failed to find target timeframe in basedata"),
             asset_id, "failed to find asset for target timeframe")
          .index();

  std::vector<std::string> columns;
  std::vector<arrow::ChunkedArrayPtr> arrayList;

  std::unordered_set<std::string> columIdSet;
  for (const auto &inputId : transformInputs) {
    if (columIdSet.contains(inputId)) {
      continue;
    }

    auto transform = m_ioIdToTransform.find(inputId);
    if (transform == m_ioIdToTransform.end()) {
      throw std::runtime_error("Cannot find transform for input: " +
                               inputId);
    }
    auto tf = transform->second->GetTimeframe().ToString();
    SPDLOG_DEBUG(
        "Gathering input {} for transform {}, asset: {}, timeframe {}. from {}",
        inputId, transform->second->GetId(), asset_id, tf,
        transformer.GetId());
    auto result = m_cache.at(tf).at(asset_id).at(inputId);
    arrayList.emplace_back(tf == targetTimeframe
                               ? result.array()
                               : result.reindex(targetIndex).array());
    columns.emplace_back(inputId);
    columIdSet.emplace(inputId);
  }

  for (const auto &dataSource : dataSources) {
    if (columIdSet.contains(dataSource)) {
      continue;
    }
    auto column = m_baseData.at(targetTimeframe).at(asset_id)[dataSource];
    arrayList.emplace_back(column.array());
    columns.emplace_back(dataSource);
  }

  return epoch_frame::make_dataframe(targetIndex, arrayList, columns);
}

void IntermediateResultStorage::InitializeBaseData(
    TimeFrameAssetDataFrameMap data, const std::unordered_set<AssetID> &allowed_asset_ids) {
  // Store base data with empty transformepoch_metadata::ID
  // Acquire exclusive locks for initialization
  std::unique_lock baseDataLock(m_baseDataMutex);
  std::unique_lock cacheLock(m_cacheMutex);
  std::unique_lock assetsLock(m_assetIDsMutex);

  m_baseData = std::move(data);
  std::unordered_set<AssetID> asset_id_set;

  // Update metadata for base data columns
  for (const auto &[timeframe, assetMap] : m_baseData) {
    for (const auto &[asset_id, dataFrame] : assetMap) {
      asset_id_set.insert(asset_id);

      if (!allowed_asset_ids.contains(asset_id)) {
        SPDLOG_DEBUG("Asset {} not found in required assets list", asset_id);
        continue;
      }
      SPDLOG_DEBUG("Initializing base data for asset: {}, timeframe {}",
                   asset_id, timeframe);
      for (const auto &colName : dataFrame.column_names()) {
        m_cache[timeframe][asset_id][colName] = dataFrame[colName];
      }
    }
  }
  m_asset_ids.assign(asset_id_set.begin(), asset_id_set.end());
}

void IntermediateResultStorage::RegisterTransform(
    const epoch_metadata::transform::ITransformBase &transform) {
  std::unique_lock lock(m_transformMapMutex);

  // Register each output of this transform
  for (const auto& output : transform.GetOutputMetaData()) {
    const std::string outputId = transform.GetOutputId(output.id);
    m_ioIdToTransform.insert_or_assign(outputId, &transform);
  }
}

TimeFrameAssetDataFrameMap IntermediateResultStorage::BuildFinalOutput() {
  // Acquire read locks to safely copy data
  std::shared_lock cacheLock(m_cacheMutex);
  std::shared_lock baseDataLock(m_baseDataMutex);
  std::shared_lock transformMapLock(m_transformMapMutex);
  std::shared_lock assetsLock(m_assetIDsMutex);

  TimeFrameAssetDataFrameMap result = m_baseData; // Copy instead of move for thread safety

  std::unordered_map<
      std::string, std::unordered_map<AssetID, std::vector<epoch_frame::FrameOrSeries>>>
      concatFrames;

  for (const auto &asset_id : m_asset_ids) {
    for (const auto &[ioId, transform] : m_ioIdToTransform) {
      if (transform->GetConfiguration().GetTransformDefinition().GetMetadata().category ==
          epoch_core::TransformCategory::DataSource) {
        continue;
      }
      const auto targetTimeframe = transform->GetTimeframe().ToString();
      const auto &assetBucket = m_cache.at(targetTimeframe);
      auto bucket = assetBucket.find(asset_id);
      if (bucket == assetBucket.end()) {
        continue;
      }
      // we only export cseries with plot kinds
      auto target = bucket->second.find(ioId);
      if (target == bucket->second.end()) {
        continue;
      }

      concatFrames[targetTimeframe][asset_id].emplace_back(target->second);
    }
  }

  // Release locks before expensive concat operations
  cacheLock.unlock();
  baseDataLock.unlock();
  transformMapLock.unlock();
  assetsLock.unlock();

  for (const auto &[timeframe, assetMap] : result) {
    for (const auto &[asset_id, dataFrame] : assetMap) {
      auto &entry = concatFrames[timeframe][asset_id];
      if (entry.empty()) {
        continue;
      }
      entry.emplace_back(dataFrame);
      auto concated =
          epoch_frame::concat({.frames = entry,
                               .joinType = epoch_frame::JoinType::Outer,
                               .axis = epoch_frame::AxisType::Column});
      result[timeframe][asset_id] = std::move(concated);
    }
  }

  return result;
}

std::shared_ptr<arrow::DataType>
GetArrowTypeFromIODataType(epoch_core::IODataType dataType) {
  using epoch_core::IODataType;
  switch (dataType) {
  case IODataType::Integer:
    return arrow::int64();
  case IODataType::Boolean:
    return arrow::boolean();
  case IODataType::Decimal:
  case IODataType::Number:
    return arrow::float64();
  case IODataType::String:
    return arrow::binary();
  default:
    break;
  }
  SPDLOG_WARN("Invalid IODataType: {}. using null scalar",
              epoch_core::IODataTypeWrapper::ToString(dataType));
  return arrow::null();
}

void IntermediateResultStorage::StoreTransformOutput(
    const AssetID &asset_id,
    const epoch_metadata::transform::ITransformBase &transformer,
    const epoch_frame::DataFrame &data) {
  const auto timeframe = transformer.GetTimeframe().ToString();

  // Read lock for baseData, write lock for cache (this is the hot path)
  std::shared_lock baseDataLock(m_baseDataMutex);
  std::unique_lock cacheLock(m_cacheMutex);

  for (const auto &outputMetaData : transformer.GetOutputMetaData()) {
    auto outputId = transformer.GetOutputId(outputMetaData.id);
    auto index = m_baseData[timeframe][asset_id].index();

    if (data.contains(outputId)) {
      SPDLOG_DEBUG("Storing output {} for asset: {}, timeframe {}", outputId,
                   asset_id, timeframe);
      // TODO: Save guide for duplicate index(Futures)
      m_cache[timeframe][asset_id][outputId] = data[outputId].reindex(index);
      continue;
    }
    SPDLOG_DEBUG("Storing NULL   output {} for asset: {}, timeframe {}",
                 outputId, asset_id, timeframe);

    m_cache[timeframe][asset_id][outputId] = epoch_frame::Series(
        arrow::MakeNullScalar(GetArrowTypeFromIODataType(outputMetaData.type)),
        index, outputId);
  }
}
} // namespace epoch_flow::runtime