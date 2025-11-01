#pragma once
#include "storage_types.h"
#include "iintermediate_storage.h"
#include <vector>
#include <shared_mutex>

namespace epoch_flow::runtime {
    class IntermediateResultStorage : public IIntermediateStorage {
    public:
        epoch_frame::DataFrame
        GatherInputs(const AssetID &asset_id,
                     const epoch_metadata::transform::ITransformBase &transformer) const override;

        void InitializeBaseData(TimeFrameAssetDataFrameMap data, const std::unordered_set<AssetID> &allowed_asset_ids) override;

        // Additional method to convert cache back to DataFrame format
        TimeFrameAssetDataFrameMap BuildFinalOutput() override;

        void RegisterTransform(const epoch_metadata::transform::ITransformBase &transform) override;

        void StoreTransformOutput(const AssetID &asset_id,
                                  const epoch_metadata::transform::ITransformBase &transformer,
                                  const epoch_frame::DataFrame &data) override;

        std::vector<AssetID> GetAssetIDs() const final {
            std::shared_lock lock(m_assetIDsMutex);
            return m_asset_ids;
        }

    private:
        TimeFrameCache m_cache;
        TimeFrameAssetDataFrameMap m_baseData;
        // Map from output ID to transform pointer for metadata queries
        std::unordered_map<std::string, const epoch_metadata::transform::ITransformBase*> m_ioIdToTransform;
        std::vector<AssetID> m_asset_ids;

        // Thread-safety: Separate mutexes for different data structures to minimize contention
        mutable std::shared_mutex m_cacheMutex;        // Protects m_cache (hot path)
        mutable std::shared_mutex m_baseDataMutex;     // Protects m_baseData
        mutable std::shared_mutex m_transformMapMutex; // Protects m_ioIdToTransform
        mutable std::shared_mutex m_assetIDsMutex;     // Protects m_asset_ids
    };
} // namespace epoch_flow::runtime