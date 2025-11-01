//
// Created by adesola on 12/28/24.
//

#pragma once
#include <epoch_script/transforms/runtime/iorchestrator.h>
#include "execution/execution_node.h"
#include "execution/execution_context.h"
#include <epoch_script/transforms/runtime/transform_manager/itransform_manager.h>
#include <epoch_script/transforms/core/registry.h>
#include <tbb/flow_graph.h>

namespace epoch_script::runtime {
    // TODO: Provide Stream Interface to Live trading
    class DataFlowRuntimeOrchestrator final : public IDataFlowOrchestrator {

    public:
        using HandleType = std::string;

        // Concurrency policy: serial for deterministic debugging, unlimited for performance
        // Controlled by EPOCH_ENABLE_PARALLEL_EXECUTION compile flag
        using TransformExecutionNode = tbb::flow::continue_node<tbb::flow::continue_msg>;
        using TransformNodePtr = std::unique_ptr<TransformExecutionNode>;

        DataFlowRuntimeOrchestrator(
            std::vector<std::string> asset_ids,
            ITransformManagerPtr transformManager,
            IIntermediateStoragePtr cacheManager = nullptr, ILoggerPtr logger = nullptr);

        void
        RegisterTransform(std::unique_ptr<epoch_script::transform::ITransformBase> transform);

        /**
         * @brief Execute the flow graph.
         *        Typically you'd push some initial ExecutionContext into "root" nodes.
         */
        TimeFrameAssetDataFrameMap ExecutePipeline(TimeFrameAssetDataFrameMap) override;

        AssetReportMap GetGeneratedReports() const override;

        AssetSelectorMap GetGeneratedSelectors() const override;

        // Public for testing
        static void MergeReportInPlace(epoch_proto::TearSheet& existing,
                                         const epoch_proto::TearSheet& newReport,
                                         const std::string& sourceTransformId);

    private:
        std::vector<std::string> m_asset_ids;
        tbb::flow::graph m_graph{};
        std::unordered_map<std::string, TransformExecutionNode *> m_outputHandleToNode;
        std::vector<TransformNodePtr> m_independentNodes;
        std::vector<TransformNodePtr> m_dependentNodes;
        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>>
            m_transforms;
        std::vector<std::function<void(execution_context_t)>> m_executionFunctions; // temporary
        ExecutionContext m_executionContext;

        // Report cache for reporter transforms (thread-safe with mutex)
        mutable AssetReportMap m_reportCache;
        mutable std::mutex m_reportCacheMutex;

        // Selector cache for selector transforms (thread-safe with mutex)
        mutable AssetSelectorMap m_selectorCache;
        mutable std::mutex m_selectorCacheMutex;

        std::function<void(execution_context_t)> CreateExecutionFunction(
            const epoch_script::transform::ITransformBase &transform);

        TransformNodePtr
        CreateTransformNode(epoch_script::transform::ITransformBase& transform);

        std::vector<DataFlowRuntimeOrchestrator::TransformExecutionNode *>
        ResolveInputDependencies(const epoch_script::strategy::InputMapping &inputs) const;

        // Helper to cache reports from reporter transforms
        void CacheReportFromTransform(const epoch_script::transform::ITransformBase& transform) const;

        // Helper to cache selectors from selector transforms
        void CacheSelectorFromTransform(const epoch_script::transform::ITransformBase& transform) const;
    };

    using DataFlowOrchestratorPtr = std::unique_ptr<DataFlowRuntimeOrchestrator>;
} // namespace epoch_script::runtime