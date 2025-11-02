//
// Created by adesola on 8/7/24.
//

#pragma once
#include "epoch_core/common_utils.h"
#include <epoch_script/transforms/runtime/transform_manager/itransform_manager.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::runtime {
  struct TimeframeResolutionCache {
    std::unordered_map<std::string, std::optional<epoch_script::TimeFrame>>
        nodeTimeframes;

    std::optional<epoch_script::TimeFrame> ResolveTimeframe(
        const std::string &nodeId, const std::vector<std::string> &inputIds,
        const std::optional<epoch_script::TimeFrame> &baseTimeframe);
  };

  struct TransformManagerOptions {
    epoch_script::strategy::PythonSource source;
    bool strict = true;
    bool timeframeIsBase = false;
    std::optional<epoch_script::TimeFrame> timeframe;
  };

  class TransformManager : public ITransformManager {

  public:
    TransformManager() = default;
    explicit TransformManager(TransformManagerOptions const&);
    ~TransformManager() override = default;

    const epoch_script::transform::TransformConfiguration *
    Insert(TransformConfigurationPtr info);

    const epoch_script::transform::TransformConfiguration *
    Insert(const std::string &name, TransformConfigurationPtr info);

    template <typename T = epoch_script::transform::TransformConfiguration>
    const epoch_script::transform::TransformConfiguration *
    Insert(epoch_script::TransformDefinition const &config) {
      return Insert(config.GetId(), std::make_unique<T>(config));
    }

    template <typename T = epoch_script::transform::TransformConfiguration>
    const epoch_script::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epoch_script::TransformDefinitionData const &definitionData) {
      return Insert(key.value_or(definitionData.id),
                    std::make_unique<T>(
                        epoch_script::TransformDefinition{definitionData}));
    }

    template <typename T = epoch_script::transform::TransformConfiguration>
    const epoch_script::transform::TransformConfiguration *
    Insert(epoch_script::TransformDefinitionData const &definitionData) {
      return Insert(definitionData.id,
                    std::make_unique<T>(
                        epoch_script::TransformDefinition{definitionData}));
    }

    const epoch_script::transform::TransformConfiguration *Insert(
        epoch_script::transform::TransformConfiguration const &configuration) override {
      return Insert(
          configuration.GetId(),
          std::make_unique<epoch_script::transform::TransformConfiguration>(
              configuration));
    }

    template <typename T = epoch_script::transform::TransformConfiguration>
    const epoch_script::transform::TransformConfiguration *
    Insert(const std::optional<std::string> &key,
           epoch_script::TransformDefinition const &config) {
      return Insert(key.value_or(config.GetId()), std::make_unique<T>(config));
    }

    const epoch_script::transform::TransformConfiguration *
    GetTransformConfigurationById(const std::string &key) const final {
      return epoch_core::lookup(
          m_configurationsById, key,
          "Failed to find transform configuration for output " + key);
    }

    void Merge(const ITransformManager *transformManager);

    const epoch_script::transform::TransformConfigurationPtrList *
    GetTransforms() const final {
      return &m_configurations;
    }

    const epoch_script::transform::TransformConfiguration*
    GetExecutor() const final {
      AssertFromStream(m_executorId.has_value(), "No executor is set.");
      return epoch_core::lookup(
          m_configurationsById, *m_executorId,
          "Failed to find a valid executor for the strategy.");
    }

    [[nodiscard]] std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>>
    BuildTransforms() const final;

  private:
    epoch_script::transform::TransformConfigurationPtrList m_configurations;
    std::unordered_map<std::string,
                       const epoch_script::transform::TransformConfiguration *>
        m_configurationsById;
    std::unordered_map<std::string,
                       const epoch_script::transform::TransformConfiguration *>
        m_configurationsByOutput;
    std::optional<std::string> m_executorId;

    void BuildTransformManager(
    std::vector<epoch_script::strategy::AlgorithmNode> &algorithms,
    const std::optional<epoch_script::TimeFrame> &baseTimeframe);
  };
} // namespace epoch_script::runtime
