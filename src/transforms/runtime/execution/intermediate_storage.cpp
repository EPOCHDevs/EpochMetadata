#include "intermediate_storage.h"
#include "common/asserts.h"
#include "epoch_frame/common.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include <epoch_script/transforms/core/metadata.h>
#include "storage_types.h"
#include <algorithm>
#include <arrow/table.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <ranges>
#include <vector>

namespace epoch_script::runtime {

// Helper function to broadcast a scalar value to a target index size (DRY principle)
// SRP: Responsible only for creating a broadcasted array from a scalar
static arrow::ChunkedArrayPtr BroadcastScalar(const epoch_frame::Scalar& scalar, size_t target_size) {
    auto broadcastedArray = arrow::MakeArrayFromScalar(*scalar.value(), target_size).ValueOrDie();
    return std::make_shared<arrow::ChunkedArray>(broadcastedArray);
}

epoch_frame::DataFrame IntermediateResultStorage::GatherInputs(
    const AssetID &asset_id,
    const epoch_script::transform::ITransformBase &transformer) const {
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

  // Acquire read locks for all relevant caches
  std::shared_lock cacheLock(m_cacheMutex);
  std::shared_lock baseDataLock(m_baseDataMutex);
  std::shared_lock transformMapLock(m_transformMapMutex);
  std::shared_lock scalarLock(m_scalarCacheMutex);  // Also lock scalar cache for reading

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

    // Check if this input is a scalar (SRP: decision point for scalar vs regular path)
    if (m_scalarOutputs.contains(inputId)) {
      // Scalar path: Broadcast from global scalar cache
      const auto& scalarValue = m_scalarCache.at(inputId);
      arrayList.emplace_back(BroadcastScalar(scalarValue, targetIndex->size()));
      columns.emplace_back(inputId);
      columIdSet.emplace(inputId);
      SPDLOG_DEBUG("Broadcasting scalar {} to {} rows for asset: {}, timeframe {}",
                   inputId, targetIndex->size(), asset_id, targetTimeframe);
      continue;
    }

    // Regular (non-scalar) path: Retrieve from timeframe-specific cache
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
  // Store base data with empty transformepoch_script::ID
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
    const epoch_script::transform::ITransformBase &transform) {
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
  std::shared_lock scalarLock(m_scalarCacheMutex);  // Also lock scalar cache

  TimeFrameAssetDataFrameMap result = m_baseData; // Copy instead of move for thread safety

  std::unordered_map<
      std::string, std::unordered_map<AssetID, std::vector<epoch_frame::FrameOrSeries>>>
      concatFrames;

  // Process regular (non-scalar) transforms
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
  scalarLock.unlock();

  // First, concatenate regular data
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

  // Second, broadcast scalars to all (timeframe, asset) combinations
  // SRP: Broadcasting scalars is a separate concern from regular data concatenation
  if (!m_scalarOutputs.empty()) {
    std::shared_lock scalarReadLock(m_scalarCacheMutex);  // Re-acquire for scalar broadcasting

    for (auto &[timeframe, assetMap] : result) {
      for (auto &[asset_id, dataFrame] : assetMap) {
        auto index = dataFrame.index();
        std::vector<epoch_frame::FrameOrSeries> scalarSeries;

        // Broadcast each scalar to this (timeframe, asset) combination
        for (const auto &scalarOutputId : m_scalarOutputs) {
          const auto& scalarValue = m_scalarCache.at(scalarOutputId);
          auto broadcastedArray = BroadcastScalar(scalarValue, index->size());

          epoch_frame::Series series(index, broadcastedArray, scalarOutputId);
          scalarSeries.emplace_back(std::move(series));
        }

        // Concatenate scalars with existing data
        if (!scalarSeries.empty()) {
          scalarSeries.emplace_back(dataFrame);
          result[timeframe][asset_id] = epoch_frame::concat({
              .frames = scalarSeries,
              .joinType = epoch_frame::JoinType::Outer,
              .axis = epoch_frame::AxisType::Column
          });
          SPDLOG_DEBUG("Broadcasted {} scalars to asset: {}, timeframe {}",
                       m_scalarOutputs.size(), asset_id, timeframe);
        }
      }
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
    const epoch_script::transform::ITransformBase &transformer,
    const epoch_frame::DataFrame &data) {
  const auto timeframe = transformer.GetTimeframe().ToString();

  // Check if this is a scalar transform
  const auto metadata = transformer.GetConfiguration().GetTransformDefinition().GetMetadata();
  const bool isScalar = (metadata.category == epoch_core::TransformCategory::Scalar);

  if (isScalar) {
    // Scalar optimization: Store once globally, not per (timeframe, asset)
    std::unique_lock scalarLock(m_scalarCacheMutex);

    for (const auto &outputMetaData : transformer.GetOutputMetaData()) {
      auto outputId = transformer.GetOutputId(outputMetaData.id);

      // Only store if not already cached (scalars are executed once)
      if (!m_scalarCache.contains(outputId)) {
        if (data.contains(outputId) && data[outputId].size() > 0) {
          // Extract scalar value from first element of the Series
          m_scalarCache[outputId] = epoch_frame::Scalar(data[outputId].array()->GetScalar(0).ValueOrDie());
          SPDLOG_DEBUG("Stored scalar {} globally (single copy, no timeframe/asset)", outputId);
        } else {
          // Store null scalar
          m_scalarCache[outputId] = epoch_frame::Scalar(
              arrow::MakeNullScalar(GetArrowTypeFromIODataType(outputMetaData.type)));
          SPDLOG_DEBUG("Stored NULL scalar {} globally", outputId);
        }
        m_scalarOutputs.insert(outputId);
      }
    }
    return; // Early exit - scalars don't use the regular cache
  }

  // Regular (non-scalar) storage path
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
} // namespace epoch_script::runtime