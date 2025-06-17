//
// Created by dewe on 9/12/24.
//

#pragma once
#include <epoch_core/enum_wrapper.h>
#include <glaze/glaze.hpp>
#include <unordered_set>
#include <variant>
#include <yaml-cpp/yaml.h>

CREATE_ENUM(MetaDataOptionType, Integer, Decimal, Boolean, Select);

namespace epoch_metadata {
struct MetaDataArgRef {
  std::string refName{};
  bool operator==(const MetaDataArgRef &) const = default;
};

class MetaDataOptionDefinition {
public:
  using T = std::variant<double, bool, std::string, MetaDataArgRef>;

  MetaDataOptionDefinition() = default;

  template <typename K>
    requires std::is_constructible_v<T, K>
  MetaDataOptionDefinition(K &&value)
      : m_optionsVariant(std::forward<K>(value)) {}

  [[nodiscard]] auto GetVariant() const { return m_optionsVariant; }

  template <class K> [[nodiscard]] bool IsType() const {
    return std::holds_alternative<K>(m_optionsVariant);
  }

  [[nodiscard]] auto GetDecimal() const { return GetValueByType<double>(); }

  [[nodiscard]] double GetNumericValue() const;

  [[nodiscard]] auto GetInteger() const {
    return static_cast<int64_t>(GetValueByType<double>());
  }

  [[nodiscard]] auto GetBoolean() const { return GetValueByType<bool>(); }

  std::string GetRef() const {
    return GetValueByType<MetaDataArgRef>().refName;
  }

  template <class T> T GetSelectOption() const {
    return epoch_core::EnumWrapper<T>::type::FromString(
        GetValueByType<std::string>());
  }

  std::string GetSelectOption() const { return GetValueByType<std::string>(); }

  size_t GetHash() const;

  void AssertType(epoch_core::MetaDataOptionType const &argType,
                  std::unordered_set<std::string> const &selections = {}) const;

  bool IsType(epoch_core::MetaDataOptionType const &argType) const;

  template <class T> void AssertType() const {
    AssertFromStream(std::holds_alternative<T>(m_optionsVariant),
                     "Wrong type! Expected: "
                         << typeid(T).name()
                         << ", but got: " << typeid(m_optionsVariant).name());
  }

  bool operator==(const MetaDataOptionDefinition &other) const = default;

  std::string ToString() const;

private:
  T m_optionsVariant{};

  template <class T> T GetValueByType() const {
    std::stringstream errorStreamer;
    try {
      return std::get<T>(m_optionsVariant);
    } catch (std::bad_variant_access const &) {
      errorStreamer << "Error: Bad variant access.\n"
                    << "Expected type: " << typeid(T).name() << std::endl;
    } catch (...) {
      errorStreamer << "Error: An unknown error occurred." << std::endl;
    }
    throw std::runtime_error(errorStreamer.str());
  }
};

using MetaDataArgDefinitionMapping =
    std::unordered_map<std::string, MetaDataOptionDefinition>;

struct SelectOption {
  std::string name{};
  std::string value{};

  void decode(YAML::Node const &node) {
    name = node["name"].as<std::string>();
    value = node["value"].as<std::string>();
  }
};

struct MetaDataOption {
  std::string id;
  std::string name;
  epoch_core::MetaDataOptionType type;
  std::optional<MetaDataOptionDefinition> defaultValue{std::nullopt};
  bool isRequired{false};
  std::vector<SelectOption> selectOption{};
  double min{-1000};
  double max{1000};
  double step_size{0.01};
  std::string desc{};

  void decode(YAML::Node const &);

  YAML::Node encode() const { return {}; }
};

using MetaDataOptionList = std::vector<MetaDataOption>;

MetaDataOptionDefinition CreateMetaDataArgDefinition(YAML::Node const &,
                                                     MetaDataOption const &);
} // namespace epoch_metadata

namespace YAML {
template <> struct convert<epoch_metadata::MetaDataOption> {
  static bool decode(const Node &node, epoch_metadata::MetaDataOption &t) {
    t.decode(node);
    return true;
  }
};

template <> struct convert<epoch_metadata::SelectOption> {
  static bool decode(const Node &node, epoch_metadata::SelectOption &t) {
    t.decode(node);
    return true;
  }
};

} // namespace YAML

namespace glz {
template <> struct meta<epoch_metadata::MetaDataOptionDefinition> {
  static constexpr auto read =
      [](epoch_metadata::MetaDataOptionDefinition &x,
         const epoch_metadata::MetaDataOptionDefinition::T &input) {
        x = epoch_metadata::MetaDataOptionDefinition{input};
      };

  static constexpr auto write =
      [](const epoch_metadata::MetaDataOptionDefinition &x) -> auto {
    return x.GetVariant();
  };

  static constexpr auto value = glz::custom<read, write>;
};
} // namespace glz