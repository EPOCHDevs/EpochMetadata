//
// Created by dewe on 9/10/24.
//

#pragma once
#include "../metadata_options.h"
#include <epoch_core/enum_wrapper.h>
#include "epoch_metadata/transforms/metadata.h"

// including here ensure all transforms have been serialized

CREATE_ENUM(AlgorithmType, TakeProfit, StopLoss, Sizer, Commission, Slippage,
            FuturesContinuation);
CREATE_ENUM(TradeSignalType, TrendFollowing, MeanReverting, CandleStickPattern,
            Momentum, EventDriven);

namespace metadata::strategy {
struct AlgorithmBaseMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
};

struct AlgorithmMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
  bool isGroup{false};
  bool requiresTimeframe{true};
};

using InputMapping = std::unordered_map<std::string, std::string>;
struct AlgorithmNode {
  std::string type;
  std::string id{};
  metadata::MetaDataArgDefinitionMapping options{};
  InputMapping inputs{};
  metadata::transforms::TransformsMetaData metaData;
};

struct TradeSignalMetaData {
  std::string id;
  std::string name;
  MetaDataOptionList options{};
  std::string desc{};
  bool isGroup{false};
  bool requiresTimeframe{true};
  epoch_core::TradeSignalType type{epoch_core::TradeSignalType::Null};
  std::vector<AlgorithmNode> algorithm;
  AlgorithmNode executor;
};

// Copy member variables to support glaze serialization form decomposition
} // namespace metadata::strategy

namespace YAML {
template <> struct convert<metadata::strategy::AlgorithmNode> {
  static bool decode(YAML::Node const &, metadata::strategy::AlgorithmNode &);
};

template <> struct convert<metadata::strategy::AlgorithmBaseMetaData> {
  static bool decode(YAML::Node const &,
                     metadata::strategy::AlgorithmBaseMetaData &);
};

template <> struct convert<metadata::strategy::AlgorithmMetaData> {
  static bool decode(YAML::Node const &,
                     metadata::strategy::AlgorithmMetaData &);
};

template <> struct convert<metadata::strategy::TradeSignalMetaData> {
  static bool decode(YAML::Node const &,
                     metadata::strategy::TradeSignalMetaData &);
};
} // namespace YAML