//
// Created by adesola on 9/26/24.
//

#pragma once
#include <epoch_core/common_utils.h>
#include <epoch_frame/datetime.h>
#include <epoch_metadata/strategy/metadata.h>
#include <epoch_metadata/transforms/metadata.h>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata {
using epoch_metadata::strategy::InputMapping;

struct TransformDefinitionData {
  std::string type;
  std::string id{};
  epoch_metadata::MetaDataArgDefinitionMapping options{};
  std::optional<epoch_metadata::TimeFrame> timeframe;
  InputMapping inputs{};
  epoch_metadata::transforms::TransformsMetaData metaData{};
  std::optional<epoch_frame::SessionRange> sessionRange{};
};

struct TransformDefinition {
public:
  explicit TransformDefinition(TransformDefinitionData data)
      : m_data(std::move(data)) {}
  explicit TransformDefinition(YAML::Node const &node);
  TransformDefinition(epoch_metadata::strategy::AlgorithmNode const &algorithm,
                      std::optional<epoch_metadata::TimeFrame> timeframe);

  TransformDefinition &
  SetOption(std::string const &key,
            epoch_metadata::MetaDataOptionDefinition const &value) {
    m_data.options.emplace(key, value.GetVariant());
    return *this;
  }

  TransformDefinition &SetPeriod(int64_t value) {
    return SetOption("period", epoch_metadata::MetaDataOptionDefinition{
                                   static_cast<double>(value)});
  }

  TransformDefinition &SetPeriods(int64_t value) {
    return SetOption("periods", epoch_metadata::MetaDataOptionDefinition{
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
               epoch_metadata::MetaDataOptionDefinition{fallback})
        .GetDecimal();
  }

  double GetOptionAsDouble(std::string const &key) const {
    return epoch_metadata::MetaDataOptionDefinition{
        epoch_core::lookup(m_data.options, key)}
        .GetDecimal();
  }

  std::string GetType() const { return m_data.type; }

  epoch_metadata::TimeFrame GetTimeframe() const {
    AssertFromStream(m_data.timeframe.has_value(), "Timeframe is not set");
    return m_data.timeframe.value();
  }

  std::string GetId() const { return m_data.id; }

  InputMapping GetInputs() const { return m_data.inputs; }

  epoch_metadata::MetaDataArgDefinitionMapping GetOptions() const {
    return m_data.options;
  }

  epoch_metadata::transforms::TransformsMetaData GetMetadata() const {
    return m_data.metaData;
  }

  std::optional<epoch_frame::SessionRange> GetSessionRange() const {
    return m_data.sessionRange;
  }

  TransformDefinitionData GetData() const { return m_data; }

private:
  TransformDefinitionData m_data;
};
} // namespace epoch_metadata

// namespace YAML {
// template <> struct convert<epoch_metadata::TransformDefinition> {
//   static bool decode(Node const &node,
//                      epoch_metadata::TransformDefinition &def) {
//     try {
//       def = epoch_metadata::TransformDefinition{node};
//       return true;
//     } catch (...) {
//       return false;
//     }
//   }
// };
// }