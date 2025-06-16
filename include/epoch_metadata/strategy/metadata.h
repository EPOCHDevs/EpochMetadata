//
// Created by dewe on 9/10/24.
//

#pragma once
#include "../metadata_options.h"
#include "enums.h"
#include "epoch_metadata/transforms/metadata.h"
#include "ui_data.h"

// including here ensure all transforms have been serialized
namespace epoch_metadata::strategy {
struct AlgorithmBaseMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
  std::vector<std::string> tags{};
};

struct AlgorithmMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
  bool isGroup{false};
  bool requiresTimeframe{true};
  std::vector<std::string> tags{};
};

using InputMapping = std::unordered_map<std::string, std::vector<std::string>>;
struct AlgorithmNode {
  std::string type;
  std::string id{};
  epoch_metadata::MetaDataArgDefinitionMapping options{};
  InputMapping inputs{};
  std::optional<TimeFrame> timeframe{};

  bool operator==(const AlgorithmNode &other) const = default;
};

struct TradeSignalMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
  bool isGroup{false};
  bool requiresTimeframe{true};
  epoch_core::TradeSignalType type{epoch_core::TradeSignalType::Null};
  UIData data;
  std::vector<std::string> tags{};
};

struct PartialTradeSignalMetaData {
  MetaDataOptionList options;
  std::vector<AlgorithmNode> algorithm;
  AlgorithmNode executor;
};

// Copy member variables to support glaze serialization form decomposition
} // namespace epoch_metadata::strategy

namespace YAML {
template <> struct convert<epoch_metadata::strategy::AlgorithmNode> {
  static bool decode(YAML::Node const &,
                     epoch_metadata::strategy::AlgorithmNode &);
};

template <> struct convert<epoch_metadata::strategy::AlgorithmBaseMetaData> {
  static bool decode(YAML::Node const &,
                     epoch_metadata::strategy::AlgorithmBaseMetaData &);
};

template <> struct convert<epoch_metadata::strategy::AlgorithmMetaData> {
  static bool decode(YAML::Node const &,
                     epoch_metadata::strategy::AlgorithmMetaData &);
};

template <> struct convert<epoch_metadata::strategy::TradeSignalMetaData> {
  static bool decode(YAML::Node const &,
                     epoch_metadata::strategy::TradeSignalMetaData &);
};
} // namespace YAML