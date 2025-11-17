#pragma once
#include "storage_types.h"
#include "iintermediate_storage.h"
#include <vector>
#include <shared_mutex>

namespace epoch_script::runtime {
    class IntermediateResultStorage : public IIntermediateStorage {
    public:
        epoch_frame::DataFrame
        GatherInputs(const AssetID &asset_id,
                     const epoch_script::transform::ITransformBase &transformer) const override;

        // Validate that all inputs are available for an asset before gathering
        // Returns true if all inputs exist, false if any are missing
        bool ValidateInputsAvailable(
            const AssetID &asset_id,
            const epoch_script::transform::ITransformBase &transformer) const override;

        void InitializeBaseData(TimeFrameAssetDataFrameMap data, const std::unordered_set<AssetID> &allowed_asset_ids) override;

        // Additional method to convert cache back to DataFrame format
        TimeFrameAssetDataFrameMap BuildFinalOutput() override;

        void RegisterTransform(const epoch_script::transform::ITransformBase &transform) override;

        void StoreTransformOutput(const AssetID &asset_id,
                                  const epoch_script::transform::ITransformBase &transformer,
                                  const epoch_frame::DataFrame &data) override;

        std::vector<AssetID> GetAssetIDs() const final {
            std::shared_lock lock(m_assetIDsMutex);
            return m_asset_ids;
        }

    private:
        TimeFrameCache m_cache;
        TimeFrameAssetDataFrameMap m_baseData;
        // Map from output ID to transform pointer for metadata queries
        std::unordered_map<std::string, const epoch_script::transform::ITransformBase*> m_ioIdToTransform;
        std::vector<AssetID> m_asset_ids;

        // Scalar optimization: Global scalar cache (no timeframe/asset dimensions)
        ScalarCache m_scalarCache;                     // outputId -> scalar value
        std::unordered_set<std::string> m_scalarOutputs; // Track which outputs are scalars

        // Thread-safety: Separate mutexes for different data structures to minimize contention
        mutable std::shared_mutex m_cacheMutex;        // Protects m_cache (hot path)
        mutable std::shared_mutex m_baseDataMutex;     // Protects m_baseData
        mutable std::shared_mutex m_transformMapMutex; // Protects m_ioIdToTransform
        mutable std::shared_mutex m_assetIDsMutex;     // Protects m_asset_ids
        mutable std::shared_mutex m_scalarCacheMutex;  // Protects m_scalarCache and m_scalarOutputs
    };
} // namespace epoch_script::runtime