#pragma once
#include "storage_types.h"
#include <epoch_frame/dataframe.h>
#include <epoch_metadata/transforms/itransform.h>

namespace epoch_flow::runtime {
class IIntermediateStorage {
public:
  virtual ~IIntermediateStorage() = default;

  // Gather inputs for a transform into a DataFrame
  [[nodiscard]] virtual epoch_frame::DataFrame
  GatherInputs(const AssetID &asset_id,
               const epoch_metadata::transform::ITransformBase &transformer) const = 0;

  virtual TimeFrameAssetDataFrameMap BuildFinalOutput() = 0;

  // Initialize base data (OHLCV)
  virtual void InitializeBaseData(TimeFrameAssetDataFrameMap data,
                                  const std::unordered_set<AssetID> &allowed_asset_ids) = 0;

  // Register a transform with the cache - stores transform metadata for later queries
  virtual void RegisterTransform(const epoch_metadata::transform::ITransformBase &transform) = 0;

  virtual void StoreTransformOutput(
      const AssetID &asset_id,
      const epoch_metadata::transform::ITransformBase &transformer,
      const epoch_frame::DataFrame &data) = 0;

  virtual std::vector<AssetID> GetAssetIDs() const = 0;
};

using IIntermediateStoragePtr = std::unique_ptr<IIntermediateStorage>;
} // namespace epoch_flow::runtime