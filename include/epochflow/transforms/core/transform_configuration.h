#pragma once
//
// Created by dewe on 4/6/23.
//
#include "transform_definition.h"
#include "string"
#include <epoch_core/common_utils.h>
#include <epoch_frame/datetime.h>
#include <epochflow/core/glaze_custom_types.h>
#include <epochflow/core/id_sequence.h>

namespace epochflow::transform {
class TransformConfiguration {

public:
  explicit TransformConfiguration(TransformDefinition def)
      : m_transformDefinition(std::move(def)) {
    for (auto const &output : m_transformDefinition.GetMetadata().outputs) {
      m_globalOutputMapping[output.id] =
          std::format("{}#{}", GetId(), output.id);
    }
  }

  std::string GetId() const { return m_transformDefinition.GetId(); }

  std::string GetTransformName() const {
    return m_transformDefinition.GetType();
  }

  epochflow::TimeFrame GetTimeframe() const {
    return m_transformDefinition.GetTimeframe();
  }

  std::vector<epochflow::transforms::IOMetaData> GetOutputs() const {
    return m_transformDefinition.GetMetadata().outputs;
  }

  InputMapping GetInputs() const { return m_transformDefinition.GetInputs(); }

  std::string GetInput() const {
    AssertFromStream(m_transformDefinition.GetInputs().size() == 1,
                     "Expected only one input\n"
                         << ToString());
    AssertFromStream(m_transformDefinition.GetInputs().begin()->second.size() ==
                         1,
                     "Expected only one input\n"
                         << ToString());
    return m_transformDefinition.GetInputs().begin()->second.front();
  }

  std::string GetInput(std::string const &parameter) const {
    auto inputs = epoch_core::lookup(GetInputs(), parameter);
    AssertFromStream(inputs.size() == 1, "Expected only one input\n"
                                             << ToString());
    return inputs.front();
  }

  std::vector<std::string> GetInputs(std::string const &parameter) const {
    auto inputs = GetInputs();
    if (auto iter = inputs.find(parameter); iter != inputs.end()) {
      return iter->second;
    }
    return {};
  }

  epochflow::MetaDataOptionDefinition
  GetOptionValue(const std::string &key) const {
    return epochflow::MetaDataOptionDefinition{
        epoch_core::lookup(GetOptions(), key)};
  }

  epochflow::MetaDataOptionDefinition GetOptionValue(
      const std::string &key,
      epochflow::MetaDataOptionDefinition const &defaultValue) const {
    return epochflow::MetaDataOptionDefinition{
        epoch_core::lookupDefault(GetOptions(), key, defaultValue)};
  }

  epochflow::MetaDataArgDefinitionMapping GetOptions() const {
    return m_transformDefinition.GetOptions();
  }

  bool IsCrossSectional() const {
    return m_transformDefinition.GetMetadata().isCrossSectional;
  }

  std::string GetOutputId() const {
    AssertFromStream(m_globalOutputMapping.size() == 1,
                     "Expected only one output\n"
                         << ToString());
    return m_globalOutputMapping.cbegin()->second;
  }

  std::string GetOutputId(std::string const &transformOutputId) const {
    return epoch_core::lookup(m_globalOutputMapping, transformOutputId);
  }

  bool ContainsOutputId(std::string const &transformOutputId) const {
    return m_globalOutputMapping.contains(transformOutputId);
  }

  auto GetOutputIds() const {
    return m_globalOutputMapping | std::views::values;
  }

  TransformDefinition GetTransformDefinition() const {
    return m_transformDefinition;
  }

  std::optional<epoch_frame::SessionRange> GetSessionRange() const {
    return m_transformDefinition.GetSessionRange();
  }

  std::string ToString() const {
    return glz::prettify("TransformConfiguration",
                         m_transformDefinition.GetData());
  }

  ~TransformConfiguration() = default;

private:
  TransformDefinition m_transformDefinition;
  std::unordered_map<std::string, std::string> m_globalOutputMapping;
};

using TransformConfigurationPtrList =
    std::vector<std::unique_ptr<transform::TransformConfiguration>>;
using TransformConfigurationList =
    std::vector<transform::TransformConfiguration>;

} // namespace epochflow::transform