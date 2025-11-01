//
// Created by adesola on 8/7/24.
//

#include "transform_manager.h"
#include <epochflow/transforms/core/transform_registry.h>

namespace epoch_flow::runtime {
  std::optional<epochflow::TimeFrame> TimeframeResolutionCache::ResolveTimeframe(
      const std::string &nodeId, const std::vector<std::string> &inputIds,
      const std::optional<epochflow::TimeFrame> &baseTimeframe) {
    // Check cache first
    if (nodeTimeframes.contains(nodeId)) {
      return nodeTimeframes[nodeId];
    }

    std::optional<epochflow::TimeFrame> resolvedTimeframe;

    // If we have inputs, resolve from them
    if (!inputIds.empty()) {
      std::vector<epochflow::TimeFrame> inputTimeframes;
      inputTimeframes.reserve(inputIds.size());

      for (const auto &handleId : inputIds) {
        auto handleNodeId = handleId.substr(0, handleId.find("#"));
        if (nodeTimeframes.contains(handleNodeId) &&
            nodeTimeframes[handleNodeId]) {
          inputTimeframes.push_back(nodeTimeframes[handleNodeId].value());
            }
      }

      // Find the lowest resolution (highest timeframe value) using operator<
      if (!inputTimeframes.empty()) {
        auto maxTimeframe =
            *std::max_element(inputTimeframes.begin(), inputTimeframes.end());
        resolvedTimeframe = maxTimeframe;
      }
    }

    // Fall back to base timeframe if no inputs or no input timeframes found
    if (!resolvedTimeframe) {
      resolvedTimeframe = baseTimeframe;
    }

    // Cache the result
    nodeTimeframes[nodeId] = resolvedTimeframe;
    return resolvedTimeframe;
  };

  std::optional<epochflow::TimeFrame> ResolveNodeTimeframe(
    const epochflow::strategy::AlgorithmNode &node,
    const std::optional<epochflow::TimeFrame> &baseTimeframe,
    TimeframeResolutionCache &cache) {

    // If node has explicit timeframe, use it
    if (node.timeframe) {
      cache.nodeTimeframes[node.id] = node.timeframe;
      return node.timeframe;
    }

    // Extract input IDs from the node
    std::vector<std::string> inputIds;
    for (const auto &[key, values] : node.inputs) {
      inputIds.insert(inputIds.end(), values.begin(), values.end());
    }

    // Use cache to resolve timeframe
    return cache.ResolveTimeframe(node.id, inputIds, baseTimeframe);
  }

  void TransformManager::BuildTransformManager(
    std::vector<epochflow::strategy::AlgorithmNode> &algorithms,
    const std::optional<epochflow::TimeFrame> &baseTimeframe) {
    TimeframeResolutionCache cache;

    // Process algorithms with timeframe resolution
    for (auto &algorithm : algorithms) {
      auto resolvedTimeframe =
          ResolveNodeTimeframe(algorithm, baseTimeframe, cache);
      this->Insert(
          epochflow::TransformDefinition{algorithm, resolvedTimeframe});
      if (algorithm.type == epochflow::transforms::TRADE_SIGNAL_EXECUTOR_ID) {
        m_executorId = algorithm.id;
      }
    }
  }

  TransformManager::TransformManager(
    TransformManagerOptions const& options) {
    auto algorithms = options.source.GetCompilationResult();

    // Only reset node timeframes if strict mode AND a timeframe is set AND not base
    if (options.strict && options.timeframe && !options.timeframeIsBase) {
      for (auto &node : algorithms) {
        if (node.timeframe.has_value()) {
          node.timeframe = {};
        }
      }
    }

    BuildTransformManager(
        algorithms, options.timeframe);
  }

  const epochflow::transform::TransformConfiguration *
  TransformManager::Insert(TransformConfigurationPtr info) {
    const auto name = info->GetId();
    auto ptr =
        m_configurationsById
            .emplace(name, m_configurations.emplace_back(std::move(info)).get())
            .first->second;

    for (auto const &outputMetadata : ptr->GetOutputs()) {
      m_configurationsByOutput.emplace(ptr->GetOutputId(outputMetadata.id), ptr);
    }
    return ptr;
  }

  const epochflow::transform::TransformConfiguration *
  TransformManager::Insert(const std::string &name,
                           TransformConfigurationPtr info) {
    AssertFromStream(!m_configurationsById.contains(name),
                     "Transform is already registered as " << name << ".");
    //        info->Rename(name);
    return Insert(std::move(info));
  }

  void TransformManager::Merge(const ITransformManager *transformManager) {
    if (transformManager) {
      for (auto const &transformInfo : *transformManager->GetTransforms()) {
        Insert(
            std::make_unique<epochflow::transform::TransformConfiguration>(
                *transformInfo));
      }
    }
  }

  std::vector<std::unique_ptr<epochflow::transform::ITransformBase>>
  TransformManager::BuildTransforms() const {
    std::vector<std::unique_ptr<epochflow::transform::ITransformBase>> transforms;
    transforms.reserve(m_configurations.size());

    for (const auto& config : m_configurations) {
      transforms.push_back(MAKE_TRANSFORM(*config));
    }

    return transforms;
  }
} // namespace epoch_flow::runtime