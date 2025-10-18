//
// Created by dewe on 9/10/24.
//

#pragma once
#include "../metadata_options.h"
#include "../time_frame.h"
#include "enums.h"
#include "session_variant.h"
#include <glaze/json/generic.hpp>
#include <variant>

// including here ensure all transforms have been serialized
namespace epoch_metadata::strategy
{
  // Type alias for Python source code (EpochFlow)
  using PythonCode = std::string;

  struct AlgorithmBaseMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    std::vector<std::string> tags{};
  };

  struct AlgorithmMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    bool isGroup{false};
    bool requiresTimeframe{true};
    std::vector<std::string> tags{};
  };

  using InputMapping = std::unordered_map<std::string, std::vector<std::string>>;
  struct AlgorithmNode
  {
    std::string type;
    std::string id{};
    epoch_metadata::MetaDataArgDefinitionMapping options{};
    InputMapping inputs{};
    std::optional<TimeFrame> timeframe{};
    std::optional<SessionVariant> session{};
    bool operator==(const AlgorithmNode &other) const
    {
      return type == other.type && id == other.id && options == other.options &&
             inputs == other.inputs && timeframe == other.timeframe &&
             (session == other.session);
    }
  };

  struct TradeSignalMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    bool isGroup{false};
    bool requiresTimeframe{true};
    epoch_core::TradeSignalType type{epoch_core::TradeSignalType::Null};
    PythonCode source;
    std::vector<std::string> tags{};
  };

  struct PartialTradeSignalMetaData
  {
    MetaDataOptionList options;
    std::vector<AlgorithmNode> algorithm;
    AlgorithmNode executor;
  };

  // Copy member variables to support glaze serialization form decomposition
} // namespace epoch_metadata::strategy

namespace YAML
{
  template <>
  struct convert<epoch_metadata::strategy::SessionVariant>
  {
    static bool decode(const Node &node,
                       epoch_metadata::strategy::SessionVariant &out);
  };

  template <>
  struct convert<epoch_metadata::strategy::AlgorithmNode>
  {
    static bool decode(YAML::Node const &,
                       epoch_metadata::strategy::AlgorithmNode &);
  };

  template <>
  struct convert<epoch_metadata::strategy::AlgorithmBaseMetaData>
  {
    static bool decode(YAML::Node const &,
                       epoch_metadata::strategy::AlgorithmBaseMetaData &);
  };

  template <>
  struct convert<epoch_metadata::strategy::AlgorithmMetaData>
  {
    static bool decode(YAML::Node const &,
                       epoch_metadata::strategy::AlgorithmMetaData &);
  };

  epoch_metadata::strategy::TradeSignalMetaData decode(glz::generic const &);

  glz::generic encode(epoch_metadata::strategy::TradeSignalMetaData const &);

} // namespace YAML