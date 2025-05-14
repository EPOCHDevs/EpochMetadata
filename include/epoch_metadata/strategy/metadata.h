//
// Created by dewe on 9/10/24.
//

#pragma once
#include "../metadata_options.h"
#include "epoch_metadata/transforms/metadata.h"
#include <epoch_core/enum_wrapper.h>

// including here ensure all transforms have been serialized

CREATE_ENUM(AlgorithmType, TakeProfit, StopLoss, Sizer, Commission, Slippage,
            FuturesContinuation);
CREATE_ENUM(TradeSignalType, TrendFollowing, MeanReverting, CandleStickPattern,
            Momentum, EventDriven, PriceAction, TechnicalPattern);

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
  std::vector<std::string> tags{};
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