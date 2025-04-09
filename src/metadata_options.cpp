//
// Created by dewe on 9/12/24.
//

#include "epoch_metadata/metadata_options.h"
#include "doc_deserialization_helper.h"
#include <epoch_core/ranges_to.h>
#include <unordered_set>

namespace epoch_metadata {
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
  case epoch_core::MetaDataOptionType::Null:
    throw std::runtime_error("Null value not allowed.");
  }
}

double MetaDataOptionDefinition::GetNumericValue() const {
  if (std::holds_alternative<double>(m_optionsVariant)) {
    return GetDecimal();
  }
  if (std::holds_alternative<bool>(m_optionsVariant)) {
    return static_cast<double>(GetBoolean());
  }

  throw std::runtime_error("Invalid Numeric MetaDataOptionType Type");
}

size_t MetaDataOptionDefinition::GetHash() const {
  return std::visit(
      [this](auto &&arg) {
        using K = std::decay_t<decltype(arg)>;
        if constexpr (std::same_as<K, MetaDataArgRef>) {
          return std::hash<std::string>{}(GetRef());
        } else {
          return std::hash<K>{}(arg);
        }
      },
      m_optionsVariant);
}

MetaDataOptionDefinition
CreateMetaDataArgDefinition(YAML::Node const &node, MetaDataOption const &arg) {
  AssertFromStream(node.IsScalar(), "invalid transform option type: "
                                        << node << ", expected a scalar for "
                                        << arg.id << ".");
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
        .min = 0,
        .max = 1000}}};

  if (element.IsScalar()) {
    *this = epoch_core::lookup(PLACEHOLDER_MAP, element.as<std::string>());
    return;
  }

  id = element["id"].as<std::string>();
  name = element["name"].as<std::string>();
  type = epoch_core::MetaDataOptionTypeWrapper::FromString(
      element["type"].as<std::string>());

  selectOption = element["selectOption"].as<std::vector<SelectOption>>(
      std::vector<SelectOption>{});

  if (element["default"]) {
    defaultValue =
        CreateMetaDataArgDefinition(element["default"], *this).GetVariant();
  }

  isRequired = element["required"].as<bool>(true);
}
} // namespace epoch_metadata