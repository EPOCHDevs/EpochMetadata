//
// Created by adesola on 8/7/24.
//

#pragma once
#include "epoch_core/common_utils.h"
#include "itransform_manager.h"
#include <epoch_metadata/transforms/transform_configuration.h>
#include <epoch_metadata/transforms/itransform.h>

namespace epoch_flow::runtime {
  struct TimeframeResolutionCache {
    std::unordered_map<std::string, std::optional<epoch_metadata::TimeFrame>>
        nodeTimeframes;

    std::optional<epoch_metadata::TimeFrame> ResolveTimeframe(
        const std::string &nodeId, const std::vector<std::string> &inputIds,
        const std::optional<epoch_metadata::TimeFrame> &baseTimeframe);
  };

  struct TransformManagerOptions {
    epoch_metadata::strategy::PythonSource source;
    bool strict = true;
    bool timeframeIsBase = false;
    std::optional<epoch_metadata::TimeFrame> timeframe;
  };

  class TransformManager : public ITransformManager {

  public:
    TransformManager() = default;
    explicit TransformManager(TransformManagerOptions const&);
    ~TransformManager() override = default;

    const epoch_metadata::transform::TransformConfiguration *
    Insert(TransformConfigurationPtr info);

    const epoch_metadata::transform::TransformConfiguration *
    Insert(const std::string &name, TransformConfigurationPtr info);

    template <typename T = epoch_metadata::transform::TransformConfiguration>
    const epoch_metadata::transform::TransformConfiguration *
    Insert(epoch_metadata::TransformDefinition const &config) {
      return Insert(config.GetId(), std::make_unique<T>(config));
    }

    template <typename T = epoch_metadata::transform::TransformConfiguration>
    const epoch_metadata::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epoch_metadata::TransformDefinitionData const &definitionData) {
      return Insert(key.value_or(definitionData.id),
                    std::make_unique<T>(
                        epoch_metadata::TransformDefinition{definitionData}));
    }

    template <typename T = epoch_metadata::transform::TransformConfiguration>
    const epoch_metadata::transform::TransformConfiguration *
    Insert(epoch_metadata::TransformDefinitionData const &definitionData) {
      return Insert(definitionData.id,
                    std::make_unique<T>(
                        epoch_metadata::TransformDefinition{definitionData}));
    }

    const epoch_metadata::transform::TransformConfiguration *Insert(
        epoch_metadata::transform::TransformConfiguration const &configuration) {
      return Insert(
          configuration.GetId(),
          std::make_unique<epoch_metadata::transform::TransformConfiguration>(
              configuration));
    }

    template <typename T = epoch_metadata::transform::TransformConfiguration>
    const epoch_metadata::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epoch_metadata::TransformDefinition const &config) {
      return Insert(key.value_or(config.GetId()), std::make_unique<T>(config));
    }

    const epoch_metadata::transform::TransformConfiguration *
    GetTransformConfigurationById(const std::string &key) const final {
      return epoch_core::lookup(
          m_configurationsById, key,
          "Failed to find transform configuration for output " + key);
    }

    void Merge(const ITransformManager *transformManager);

    const epoch_metadata::transform::TransformConfigurationPtrList *
    GetTransforms() const final {
      return &m_configurations;
    }

    const epoch_metadata::transform::TransformConfiguration*
    GetExecutor() const final {
      AssertFromStream(m_executorId.has_value(), "No executor is set.");
      return epoch_core::lookup(
          m_configurationsById, *m_executorId,
          "Failed to find a valid executor for the strategy.");
    }

    [[nodiscard]] std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>>
    BuildTransforms() const final;

  private:
    epoch_metadata::transform::TransformConfigurationPtrList m_configurations;
    std::unordered_map<std::string,
                       const epoch_metadata::transform::TransformConfiguration *>
        m_configurationsById;
    std::unordered_map<std::string,
                       const epoch_metadata::transform::TransformConfiguration *>
        m_configurationsByOutput;
    std::optional<std::string> m_executorId;

    void BuildTransformManager(
    std::vector<epoch_metadata::strategy::AlgorithmNode> &algorithms,
    const std::optional<epoch_metadata::TimeFrame> &baseTimeframe);
  };
} // namespace epoch_flow::runtime
