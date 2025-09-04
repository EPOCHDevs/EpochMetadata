//
// Created by dewe on 9/10/24.
//

#pragma once
#include "../metadata_options.h"
#include "enums.h"
#include "ui_data.h"
#include <glaze/json/json_t.hpp>
#include <variant>

// including here ensure all transforms have been serialized
namespace epoch_metadata::strategy {
using SessionVariant =
    std::variant<epoch_frame::SessionRange, epoch_core::SessionType>;

inline bool operator==(const std::optional<SessionVariant> &lhs,
                       const std::optional<SessionVariant> &rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }
  if (!lhs.has_value() && !rhs.has_value()) {
    return true;
  }

  return std::visit(
      [&]<typename T1, typename T2>(const T1 &lhs_arg, const T2 &rhs_arg) {
        if constexpr (std::is_same_v<T1, T2>) {
          return lhs_arg == rhs_arg;
        } else {
          return false;
        }
      },
      *lhs, *rhs);
}

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
  std::optional<SessionVariant> session{};
  bool operator==(const AlgorithmNode &other) const {
    return type == other.type && id == other.id && options == other.options &&
           inputs == other.inputs && timeframe == other.timeframe &&
           (session == other.session);
  }
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
template <> struct convert<epoch_metadata::strategy::SessionVariant> {
  static bool decode(const Node &node,
                     epoch_metadata::strategy::SessionVariant &out);
};

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

epoch_metadata::strategy::TradeSignalMetaData decode(glz::json_t const &);

glz::json_t encode(epoch_metadata::strategy::TradeSignalMetaData const &);

} // namespace YAML