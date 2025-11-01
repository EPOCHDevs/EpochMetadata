//
// Created by adesola on 8/7/24.
//

#pragma once
#include "epoch_core/common_utils.h"
#include "itransform_manager.h"
#include <epochflow/transforms/core/transform_configuration.h>
#include <epochflow/transforms/core/itransform.h>

namespace epoch_flow::runtime {
  struct TimeframeResolutionCache {
    std::unordered_map<std::string, std::optional<epochflow::TimeFrame>>
        nodeTimeframes;

    std::optional<epochflow::TimeFrame> ResolveTimeframe(
        const std::string &nodeId, const std::vector<std::string> &inputIds,
        const std::optional<epochflow::TimeFrame> &baseTimeframe);
  };

  struct TransformManagerOptions {
    epochflow::strategy::PythonSource source;
    bool strict = true;
    bool timeframeIsBase = false;
    std::optional<epochflow::TimeFrame> timeframe;
  };

  class TransformManager : public ITransformManager {

  public:
    TransformManager() = default;
    explicit TransformManager(TransformManagerOptions const&);
    ~TransformManager() override = default;

    const epochflow::transform::TransformConfiguration *
    Insert(TransformConfigurationPtr info);

    const epochflow::transform::TransformConfiguration *
    Insert(const std::string &name, TransformConfigurationPtr info);

    template <typename T = epochflow::transform::TransformConfiguration>
    const epochflow::transform::TransformConfiguration *
    Insert(epochflow::TransformDefinition const &config) {
      return Insert(config.GetId(), std::make_unique<T>(config));
    }

    template <typename T = epochflow::transform::TransformConfiguration>
    const epochflow::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epochflow::TransformDefinitionData const &definitionData) {
      return Insert(key.value_or(definitionData.id),
                    std::make_unique<T>(
                        epochflow::TransformDefinition{definitionData}));
    }

    template <typename T = epochflow::transform::TransformConfiguration>
    const epochflow::transform::TransformConfiguration *
    Insert(epochflow::TransformDefinitionData const &definitionData) {
      return Insert(definitionData.id,
                    std::make_unique<T>(
                        epochflow::TransformDefinition{definitionData}));
    }

    const epochflow::transform::TransformConfiguration *Insert(
        epochflow::transform::TransformConfiguration const &configuration) {
      return Insert(
          configuration.GetId(),
          std::make_unique<epochflow::transform::TransformConfiguration>(
              configuration));
    }

    template <typename T = epochflow::transform::TransformConfiguration>
    const epochflow::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epochflow::TransformDefinition const &config) {
      return Insert(key.value_or(config.GetId()), std::make_unique<T>(config));
    }

    const epochflow::transform::TransformConfiguration *
    GetTransformConfigurationById(const std::string &key) const final {
      return epoch_core::lookup(
          m_configurationsById, key,
          "Failed to find transform configuration for output " + key);
    }

    void Merge(const ITransformManager *transformManager);

    const epochflow::transform::TransformConfigurationPtrList *
    GetTransforms() const final {
      return &m_configurations;
    }

    const epochflow::transform::TransformConfiguration*
    GetExecutor() const final {
      AssertFromStream(m_executorId.has_value(), "No executor is set.");
      return epoch_core::lookup(
          m_configurationsById, *m_executorId,
          "Failed to find a valid executor for the strategy.");
    }

    [[nodiscard]] std::vector<std::unique_ptr<epochflow::transform::ITransformBase>>
    BuildTransforms() const final;

  private:
    epochflow::transform::TransformConfigurationPtrList m_configurations;
    std::unordered_map<std::string,
                       const epochflow::transform::TransformConfiguration *>
        m_configurationsById;
    std::unordered_map<std::string,
                       const epochflow::transform::TransformConfiguration *>
        m_configurationsByOutput;
    std::optional<std::string> m_executorId;

    void BuildTransformManager(
    std::vector<epochflow::strategy::AlgorithmNode> &algorithms,
    const std::optional<epochflow::TimeFrame> &baseTimeframe);
  };
} // namespace epoch_flow::runtime
