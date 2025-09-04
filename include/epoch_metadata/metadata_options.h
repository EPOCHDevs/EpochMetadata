//
// Created by dewe on 9/12/24.
//

#pragma once
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <epoch_core/enum_wrapper.h>
#include <glaze/glaze.hpp>
#include <limits>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>
#include <yaml-cpp/yaml.h>

CREATE_ENUM(MetaDataOptionType, Integer, Decimal, Boolean, Select, NumericList,
            StringList, Time);

namespace epoch_metadata {
struct TimeObject {
  int hour{0};
  int minute{0};
  int second{0};

  [[nodiscard]] int ToSeconds() const {
    return hour * 3600 + minute * 60 + second;
  }
};
struct MetaDataArgRef {
  std::string refName{};
  bool operator==(const MetaDataArgRef &) const = default;
};

using SequenceItem = std::variant<double, std::string>;
using Sequence = std::vector<SequenceItem>;

class MetaDataOptionDefinition {
public:
  using T = std::variant<Sequence, MetaDataArgRef, std::string, bool, double>;

  explicit MetaDataOptionDefinition() = default;

  // Overloads for directly passing our variant type T
  explicit MetaDataOptionDefinition(const T &value) {
    if (std::holds_alternative<std::string>(value)) {
      m_optionsVariant = ParseStringOverride(std::get<std::string>(value));
    } else {
      m_optionsVariant = value;
    }
  }

  explicit MetaDataOptionDefinition(T &&value) {
    if (std::holds_alternative<std::string>(value)) {
      m_optionsVariant =
          ParseStringOverride(std::move(std::get<std::string>(value)));
    } else {
      m_optionsVariant = std::move(value);
    }
  }

  // Convenience constructors for vector types
  explicit MetaDataOptionDefinition(const std::vector<double> &values) {
    Sequence seq;
    seq.reserve(values.size());
    for (const auto &val : values) {
      seq.emplace_back(val);
    }
    m_optionsVariant = std::move(seq);
  }

  explicit MetaDataOptionDefinition(std::vector<double> &&values) {
    Sequence seq;
    seq.reserve(values.size());
    for (auto &&val : values) {
      seq.emplace_back(std::move(val));
    }
    m_optionsVariant = std::move(seq);
  }

  explicit MetaDataOptionDefinition(const std::vector<std::string> &values) {
    Sequence seq;
    seq.reserve(values.size());
    for (const auto &val : values) {
      seq.emplace_back(val);
    }
    m_optionsVariant = std::move(seq);
  }

  explicit MetaDataOptionDefinition(std::vector<std::string> &&values) {
    Sequence seq;
    seq.reserve(values.size());
    for (auto &&val : values) {
      seq.emplace_back(std::move(val));
    }
    m_optionsVariant = std::move(seq);
  }

  // Overload for string-like types
  template <typename StringLike>
    requires(std::is_convertible_v<std::decay_t<StringLike>, std::string_view>)
  explicit MetaDataOptionDefinition(StringLike &&value) {
    std::string materialized{std::forward<StringLike>(value)};
    m_optionsVariant = ParseStringOverride(std::move(materialized));
  }

  // Overload for types directly constructible into the variant
  template <typename K>
    requires(!std::is_convertible_v<std::decay_t<K>, std::string_view> &&
             std::is_constructible_v<T, K> &&
             !std::is_same_v<std::decay_t<K>, T>)
  explicit MetaDataOptionDefinition(K &&value)
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

  [[nodiscard]] TimeObject GetTime() const;

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

  T m_optionsVariant{0.0};

private:
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

  static T ParseStringOverride(std::string input) {
    // trim leading/trailing whitespace
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    input.erase(input.begin(),
                std::find_if(input.begin(), input.end(),
                             [&](unsigned char ch) { return !is_space(ch); }));
    input.erase(std::find_if(input.rbegin(), input.rend(),
                             [&](unsigned char ch) { return !is_space(ch); })
                    .base(),
                input.end());

    // Disallow empty string after trimming
    if (input.empty()) {
      throw std::runtime_error(
          "Empty string is not a valid MetaDataOptionDefinition value");
    }

    // list parsing: [a,b,c] or [1,2,3]
    if (!input.empty() && input.front() == '[' && input.back() == ']') {
      std::string content = input.substr(1, input.size() - 2);

      auto trim = [&](std::string &s) {
        s.erase(s.begin(),
                std::find_if(s.begin(), s.end(),
                             [&](unsigned char ch) { return !is_space(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(),
                             [&](unsigned char ch) { return !is_space(ch); })
                    .base(),
                s.end());
      };

      std::vector<std::string> tokens;
      {
        std::string current;
        for (char c : content) {
          if (c == ',') {
            trim(current);
            if (!current.empty()) {
              // strip surrounding quotes if present
              if ((current.front() == '"' && current.back() == '"') ||
                  (current.front() == '\'' && current.back() == '\'')) {
                if (current.size() >= 2) {
                  current = current.substr(1, current.size() - 2);
                }
              }
              tokens.push_back(current);
            } else {
              tokens.emplace_back("");
            }
            current.clear();
          } else {
            current.push_back(c);
          }
        }
        trim(current);
        if (!current.empty()) {
          if ((current.front() == '"' && current.back() == '"') ||
              (current.front() == '\'' && current.back() == '\'')) {
            if (current.size() >= 2) {
              current = current.substr(1, current.size() - 2);
            }
          }
          tokens.push_back(current);
        }
      }

      if (tokens.empty()) {
        return T{Sequence{}};
      }

      bool any_numeric = false;
      bool any_non_numeric = false;
      Sequence sequence;
      sequence.reserve(tokens.size());

      for (auto const &tok : tokens) {
        if (tok.empty()) {
          sequence.emplace_back(std::string{});
          any_non_numeric = true;
          continue;
        }
        char *end_ptr = nullptr;
        errno = 0;
        double parsed = std::strtod(tok.c_str(), &end_ptr);
        if (end_ptr != nullptr && *end_ptr == '\0' && errno == 0 &&
            std::isfinite(parsed)) {
          sequence.emplace_back(parsed);
          any_numeric = true;
        } else {
          sequence.emplace_back(tok);
          any_non_numeric = true;
        }
      }

      if (any_numeric && any_non_numeric) {
        throw std::runtime_error("Mixed types in list literal are not allowed");
      }

      return T{std::move(sequence)};
    }

    // lowercase copy for boolean check
    std::string lowered = input;
    std::transform(
        lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lowered == "true") {
      return T{true};
    }
    if (lowered == "false") {
      return T{false};
    }

    // Check for special numeric values
    if (lowered == "nan") {
      return T{std::nan("")};
    }
    if (lowered == "inf" || lowered == "infinity") {
      return T{std::numeric_limits<double>::infinity()};
    }
    if (lowered == "-inf" || lowered == "-infinity") {
      return T{-std::numeric_limits<double>::infinity()};
    }

    // Check for explicit invalid numeric strings that should remain as strings
    if (lowered == "not_a_number") {
      // This should remain as a string, not be parsed as a number
      return T{std::move(input)};
    }

    // numeric check using strtod; accepts +/-, decimals, and scientific
    // notation
    if (!input.empty()) {
      char *end_ptr = nullptr;
      errno = 0;
      double parsed = std::strtod(input.c_str(), &end_ptr);
      if (end_ptr != nullptr && *end_ptr == '\0' && errno == 0 &&
          std::isfinite(parsed)) {
        return T{parsed};
      }
    }

    // fallback to original string
    return T{std::move(input)};
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
  double min{-std::numeric_limits<double>::max()};
  double max{std::numeric_limits<double>::max()};
  double step_size{0.000001};
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
template <> struct meta<epoch_metadata::MetaDataArgRef> {
  using T = epoch_metadata::MetaDataArgRef;
  static constexpr auto value = object("refName", &T::refName);
};

// Sequence (vector<variant<double, string>>) will be handled automatically by
// glaze

template <> struct meta<epoch_metadata::MetaDataOptionDefinition> {
  using T = epoch_metadata::MetaDataOptionDefinition;
  static constexpr auto value = &T::m_optionsVariant;
};

// Custom JSON (de)serialization: always use string IO for
// MetaDataOptionDefinition
template <> struct to<JSON, epoch_metadata::MetaDataOptionDefinition> {
  template <auto Opts>
  static void op(const epoch_metadata::MetaDataOptionDefinition &x,
                 auto &&...args) noexcept {
    const std::string out = x.ToString();
    serialize<JSON>::op<Opts>(out, args...);
  }
};

template <> struct from<JSON, epoch_metadata::MetaDataOptionDefinition> {
  template <auto Opts>
  static void op(epoch_metadata::MetaDataOptionDefinition &value,
                 auto &&...args) {
    std::string in;
    parse<JSON>::op<Opts>(in, args...);
    // Special prefix handling for encoded MetaDataArgRef
    if (in.rfind("$ref:", 0) == 0) {
      value = epoch_metadata::MetaDataOptionDefinition{
          epoch_metadata::MetaDataArgRef{in.substr(5)}};
    } else {
      value = epoch_metadata::MetaDataOptionDefinition{in};
    }
  }
};
} // namespace glz