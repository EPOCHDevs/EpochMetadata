//
// Created by adesola on 9/26/24.
//

#pragma once
#include <epoch_core/common_utils.h>
#include <epoch_frame/datetime.h>
#include <epochflow/strategy/metadata.h>
#include <epochflow/transforms/core/metadata.h>
#include <epochflow/transforms/core/registry.h>
#include <yaml-cpp/yaml.h>

namespace epochflow {
using epochflow::strategy::InputMapping;

struct TransformDefinitionData {
  std::string type;
  std::string id{};
  epochflow::MetaDataArgDefinitionMapping options{};
  std::optional<epochflow::TimeFrame> timeframe;
  InputMapping inputs{};
  epochflow::transforms::TransformsMetaData metaData{};
  std::optional<epoch_frame::SessionRange> sessionRange{};
};

struct TransformDefinition {
public:
  explicit TransformDefinition(TransformDefinitionData data)
      : m_data(std::move(data)) {
    // Auto-fill metadata from registry if not provided
    if (m_data.metaData.id.empty()) {
      auto metaDataPtr = epochflow::transforms::ITransformRegistry::GetInstance().GetMetaData(m_data.type);
      AssertFromStream(metaDataPtr, "Invalid Transform: " << m_data.type);
      m_data.metaData = *metaDataPtr;
    }
  }
  explicit TransformDefinition(YAML::Node const &node);
  TransformDefinition(epochflow::strategy::AlgorithmNode const &algorithm,
                      std::optional<epochflow::TimeFrame> timeframe);

  TransformDefinition &
  SetOption(std::string const &key,
            epochflow::MetaDataOptionDefinition const &value) {
    m_data.options.emplace(key, value.GetVariant());
    return *this;
  }

  TransformDefinition &SetPeriod(int64_t value) {
    return SetOption("period", epochflow::MetaDataOptionDefinition{
                                   static_cast<double>(value)});
  }

  TransformDefinition &SetPeriods(int64_t value) {
    return SetOption("periods", epochflow::MetaDataOptionDefinition{
                                    static_cast<double>(value)});
  }

  TransformDefinition &SetType(std::string const &value) {
    m_data.type = value;
    return *this;
  }

  TransformDefinition SetTypeCopy(std::string const &new_type) const noexcept {
    auto clone = *this;
    return clone.SetType(new_type);
  }

  TransformDefinition &SetTypeIfEmpty(std::string const &value) {
    m_data.type = m_data.type.empty() ? value : m_data.type;
    return *this;
  }

  TransformDefinition SetInput(InputMapping const &newInputs) const noexcept {
    auto clone = *this;
    clone.m_data.inputs = newInputs;
    return clone;
  }

  double GetOptionAsDouble(std::string const &key, double fallback) const {
    return epoch_core::lookupDefault(
               m_data.options, key,
               epochflow::MetaDataOptionDefinition{fallback})
        .GetDecimal();
  }

  double GetOptionAsDouble(std::string const &key) const {
    return epochflow::MetaDataOptionDefinition{
        epoch_core::lookup(m_data.options, key)}
        .GetDecimal();
  }

  std::string GetType() const { return m_data.type; }

  epochflow::TimeFrame GetTimeframe() const {
    AssertFromStream(m_data.timeframe.has_value(), "Timeframe is not set");
    return m_data.timeframe.value();
  }

  std::string GetId() const { return m_data.id; }

  InputMapping GetInputs() const { return m_data.inputs; }

  epochflow::MetaDataArgDefinitionMapping GetOptions() const {
    return m_data.options;
  }

  epochflow::transforms::TransformsMetaData GetMetadata() const {
    return m_data.metaData;
  }

  std::optional<epoch_frame::SessionRange> GetSessionRange() const {
    return m_data.sessionRange;
  }

  TransformDefinitionData GetData() const { return m_data; }

private:
  TransformDefinitionData m_data;
};
} // namespace epochflow

// namespace YAML {
// template <> struct convert<epochflow::TransformDefinition> {
//   static bool decode(Node const &node,
//                      epochflow::TransformDefinition &def) {
//     try {
//       def = epochflow::TransformDefinition{node};
//       return true;
//     } catch (...) {
//       return false;
//     }
//   }
// };
// }