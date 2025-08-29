//
// Created by dewe on 9/12/24.
//

#include "epoch_metadata/metadata_options.h"
#include "doc_deserialization_helper.h"
#include <epoch_core/ranges_to.h>
#include <unordered_set>

namespace epoch_metadata {
using SequenceItem = std::variant<double, std::string>;
using Sequence = std::vector<SequenceItem>;
void MetaDataOptionDefinition::AssertType(
    epoch_core::MetaDataOptionType const &argType,
    std::unordered_set<std::string> const &selections) const {
  switch (argType) {
  case epoch_core::MetaDataOptionType::Integer:
  case epoch_core::MetaDataOptionType::Decimal:
    AssertType<double>();
    break;
  case epoch_core::MetaDataOptionType::Boolean:
    AssertType<bool>();
    break;
  case epoch_core::MetaDataOptionType::Select: {
    auto option = GetValueByType<std::string>();
    AssertFromStream(selections.contains(option),
                     "Invalid select member: "
                         << option << ", Expected one of "
                         << epoch_core::toString(selections));
    break;
  }
  case epoch_core::MetaDataOptionType::Time: {
    AssertType<std::string>();
    auto const &val = GetValueByType<std::string>();
    auto count_colon =
        static_cast<int>(std::count(val.begin(), val.end(), ':'));
    AssertFromStream(count_colon == 1 || count_colon == 2,
                     "Time must be HH:MM or HH:MM:SS, got: " << val);
    auto parse_component = [](std::string const &s, size_t &pos) -> int {
      size_t next = s.find(':', pos);
      std::string token = s.substr(
          pos, next == std::string::npos ? std::string::npos : next - pos);
      AssertFromStream(!token.empty() && std::ranges::all_of(token, ::isdigit),
                       "Invalid time component: " << token);
      int v = std::stoi(token);
      pos = (next == std::string::npos) ? std::string::npos : next + 1;
      return v;
    };
    size_t pos = 0;
    int hh = parse_component(val, pos);
    int mm = parse_component(val, pos);
    int ss = 0;
    if (pos != std::string::npos) {
      ss = parse_component(val, pos);
    }
    AssertFromStream(0 <= hh && hh < 24, "Hour out of range: " << hh);
    AssertFromStream(0 <= mm && mm < 60, "Minute out of range: " << mm);
    AssertFromStream(0 <= ss && ss < 60, "Second out of range: " << ss);
    break;
  }
  case epoch_core::MetaDataOptionType::NumericList: {
    AssertType<Sequence>();
    // Additional validation: ensure all items are numeric
    const auto &seq = GetValueByType<Sequence>();
    for (const auto &item : seq) {
      if (!std::holds_alternative<double>(item)) {
        throw std::runtime_error("NumericList contains non-numeric values");
      }
    }
    break;
  }
  case epoch_core::MetaDataOptionType::StringList: {
    AssertType<Sequence>();
    // Additional validation: ensure all items are strings
    const auto &seq = GetValueByType<Sequence>();
    for (const auto &item : seq) {
      if (!std::holds_alternative<std::string>(item)) {
        throw std::runtime_error("StringList contains non-string values");
      }
    }
    break;
  }
  case epoch_core::MetaDataOptionType::Null:
    throw std::runtime_error("Null value not allowed.");
  }
}

bool MetaDataOptionDefinition::IsType(
    epoch_core::MetaDataOptionType const &argType) const {
  switch (argType) {
  case epoch_core::MetaDataOptionType::Integer:
  case epoch_core::MetaDataOptionType::Decimal:
    return std::holds_alternative<double>(m_optionsVariant);
  case epoch_core::MetaDataOptionType::Boolean:
    return std::holds_alternative<bool>(m_optionsVariant);
  case epoch_core::MetaDataOptionType::Select:
    return std::holds_alternative<std::string>(m_optionsVariant);
  case epoch_core::MetaDataOptionType::Time:
    return std::holds_alternative<std::string>(m_optionsVariant);
  case epoch_core::MetaDataOptionType::NumericList:
  case epoch_core::MetaDataOptionType::StringList:
    return std::holds_alternative<Sequence>(m_optionsVariant);
  case epoch_core::MetaDataOptionType::Null:
    return false;
  }
  std::unreachable();
}

double MetaDataOptionDefinition::GetNumericValue() const {
  if (std::holds_alternative<double>(m_optionsVariant)) {
    return GetDecimal();
  }
  if (std::holds_alternative<bool>(m_optionsVariant)) {
    return static_cast<double>(GetBoolean());
  }
  std::stringstream ss;
  ss << "Invalid Numeric MetaDataOptionType Type\n";
  ss << "Got: " << typeid(m_optionsVariant).name() << "\n";
  throw std::runtime_error(ss.str());
}

TimeObject MetaDataOptionDefinition::GetTime() const {
  AssertFromStream(std::holds_alternative<std::string>(m_optionsVariant),
                   "GetTime expects a string Time option");
  const auto &val = std::get<std::string>(m_optionsVariant);
  auto parse_component = [](std::string const &s, size_t &pos) -> int {
    size_t next = s.find(':', pos);
    std::string token = s.substr(
        pos, next == std::string::npos ? std::string::npos : next - pos);
    AssertFromStream(!token.empty() && std::ranges::all_of(token, ::isdigit),
                     "Invalid time component: " << token);
    int v = std::stoi(token);
    pos = (next == std::string::npos) ? std::string::npos : next + 1;
    return v;
  };

  size_t pos = 0;
  int hh = parse_component(val, pos);
  int mm = parse_component(val, pos);
  int ss = 0;
  if (pos != std::string::npos) {
    ss = parse_component(val, pos);
  }
  AssertFromStream(0 <= hh && hh < 24, "Hour out of range: " << hh);
  AssertFromStream(0 <= mm && mm < 60, "Minute out of range: " << mm);
  AssertFromStream(0 <= ss && ss < 60, "Second out of range: " << ss);
  return TimeObject{hh, mm, ss};
}

size_t MetaDataOptionDefinition::GetHash() const {
  return std::visit(
      [this](auto &&arg) {
        using K = std::decay_t<decltype(arg)>;
        if constexpr (std::same_as<K, MetaDataArgRef>) {
          return std::hash<std::string>{}(GetRef());
        } else if constexpr (std::same_as<K, Sequence>) {
          size_t seed = 0;
          for (const auto &item : arg) {
            size_t h = std::visit(
                [](const auto &v) {
                  return std::hash<std::decay_t<decltype(v)>>{}(v);
                },
                item);
            seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
          }
          return seed;
        } else {
          return std::hash<K>{}(arg);
        }
      },
      m_optionsVariant);
}

std::string MetaDataOptionDefinition::ToString() const {
  return std::visit(
      [this](auto &&arg) {
        using K = std::decay_t<decltype(arg)>;
        if constexpr (std::same_as<K, MetaDataArgRef>) {
          return std::string(GetRef());
        } else if constexpr (std::same_as<K, std::string>) {
          return arg;
        } else if constexpr (std::same_as<K, Sequence>) {
          std::string out = "[";
          for (size_t i = 0; i < arg.size(); ++i) {
            std::visit(
                [&out](const auto &v) {
                  if constexpr (std::same_as<std::decay_t<decltype(v)>,
                                             double>) {
                    out += std::to_string(v);
                  } else {
                    out += v;
                  }
                },
                arg[i]);
            if (i + 1 < arg.size())
              out += ",";
          }
          out += "]";
          return out;
        } else {
          return std::to_string(arg);
        }
      },
      m_optionsVariant);
}

MetaDataOptionDefinition
CreateMetaDataArgDefinition(YAML::Node const &node, MetaDataOption const &arg) {
  if (arg.type == epoch_core::MetaDataOptionType::NumericList ||
      arg.type == epoch_core::MetaDataOptionType::StringList) {
    AssertFromStream(node.IsSequence() || node.IsScalar(),
                     "invalid transform option type: "
                         << node
                         << ", expected a sequence or bracketed string for "
                         << arg.id << ".");
  } else {
    AssertFromStream(node.IsScalar(), "invalid transform option type: "
                                          << node << ", expected a scalar for "
                                          << arg.id << ".");
  }
  switch (arg.type) {
  case epoch_core::MetaDataOptionType::Integer:
  case epoch_core::MetaDataOptionType::Decimal:
    return MetaDataOptionDefinition{node.as<double>()};
  case epoch_core::MetaDataOptionType::Boolean: {
    return MetaDataOptionDefinition{node.as<bool>()};
  }
  case epoch_core::MetaDataOptionType::Select: {
    return MetaDataOptionDefinition{node.as<std::string>()};
  }
  case epoch_core::MetaDataOptionType::Time: {
    return MetaDataOptionDefinition{node.as<std::string>()};
  }
  case epoch_core::MetaDataOptionType::NumericList: {
    if (node.IsSequence()) {
      return MetaDataOptionDefinition{node.as<std::vector<double>>()};
    }
    return MetaDataOptionDefinition{node.as<std::string>()};
  }
  case epoch_core::MetaDataOptionType::StringList: {
    if (node.IsSequence()) {
      return MetaDataOptionDefinition{node.as<std::vector<std::string>>()};
    }
    return MetaDataOptionDefinition{node.as<std::string>()};
  }
  case epoch_core::MetaDataOptionType::Null:
    break;
  }
  throw std::runtime_error("Invalid MetaDataOptionType Type");
}

void MetaDataOption::decode(const YAML::Node &element) {
  static std::unordered_map<std::string, MetaDataOption> PLACEHOLDER_MAP{
      {"PERIOD",
       {.id = "period",
        .name = "Period",
        .type = epoch_core::MetaDataOptionType::Integer,
        .min = 1, // Period must be at least 1
        .max = 1000}}};

  if (element.IsScalar()) {
    *this = epoch_core::lookup(PLACEHOLDER_MAP, element.as<std::string>());
    return;
  }

  id = element["id"].as<std::string>();
  name = element["name"].as<std::string>();
  {
    auto rawType = element["type"].as<std::string>();
    std::string lowered = rawType;
    std::transform(
        lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lowered == "numeric_list") {
      type = epoch_core::MetaDataOptionType::NumericList;
    } else if (lowered == "string_list") {
      type = epoch_core::MetaDataOptionType::StringList;
    } else {
      type = epoch_core::MetaDataOptionTypeWrapper::FromString(rawType);
    }
  }

  selectOption = element["selectOption"].as<std::vector<SelectOption>>(
      std::vector<SelectOption>{});

  if (element["default"]) {
    defaultValue =
        CreateMetaDataArgDefinition(element["default"], *this).GetVariant();
  }
  min = element["min"].as<double>(0);
  max = element["max"].as<double>(10000);
  step_size = element["step_size"].as<double>(0.000001);

  isRequired = element["required"].as<bool>(true);
}
} // namespace epoch_metadata