#include "../../include/epoch_metadata/strategy/generic_function_validator.h"
#include "../../include/epoch_metadata/strategy/registry.h"
#include "epoch_metadata/strategy/algorithm_validator.h"
#include "epoch_metadata/strategy/enums.h"
#include <algorithm>
#include <cctype>
#include <regex>

namespace epoch_metadata::strategy {

ValidationIssues ValidateGenericFunction(const GenericFunction &function,
                                         epoch_core::GenericFunctionType type) {
  ValidationIssues issues;

  // Validate type
  auto options = ValidateGenericFunctionType(function.type, type, issues);
  if (!options) {
    return issues;
  }

  // Validate args
  ValidateGenericFunctionArgs(function.args, options.value(), function.type,
                              issues);

  if (function.data) {
    auto data_issues = ValidateUIData(function.data.value());
    if (!data_issues.has_value()) {
      auto data_issues_copy = data_issues.error();
      issues.insert(issues.end(), data_issues_copy.begin(),
                    data_issues_copy.end());
    }
  }

  return issues;
}

std::optional<MetaDataOptionList>
ValidateGenericFunctionType(const std::string &type,
                            epoch_core::GenericFunctionType functionType,
                            ValidationIssues &issues) {

  // Check if type is empty
  if (type.empty()) {
    issues.push_back({ValidationCode::MissingRequiredInput, std::monostate{},
                      "GenericFunction type cannot be empty",
                      "Provide a valid function type identifier"});
    return std::nullopt;
  }

  std::string functionTypeStr =
      epoch_core::GenericFunctionTypeWrapper::ToString(functionType);

  switch (functionType) {
  case epoch_core::GenericFunctionType::TradeSignal: {
    auto metaData = trade_signal::Registry::GetInstance().GetMetaData(type);
    if (!metaData) {
      issues.push_back({ValidationCode::UnknownNodeType, functionTypeStr,
                        "Unknown GenericFunction type: " + type,
                        "Provide a valid GenericFunction type"});
      return std::nullopt;
    }

    return metaData.value().get().options;
  }
  case epoch_core::GenericFunctionType::PositionSizer: {
    auto metaData = position_sizer::Registry::GetInstance().GetMetaData(type);
    if (!metaData) {
      issues.push_back({ValidationCode::UnknownNodeType, functionTypeStr,
                        "Unknown GenericFunction type: " + type,
                        "Provide a valid GenericFunction type"});
      return std::nullopt;
    }
    return metaData.value().get().options;
  }
  case epoch_core::GenericFunctionType::TakeProfit: {
    auto metaData = take_profit::Registry::GetInstance().GetMetaData(type);
    if (!metaData) {
      issues.push_back({ValidationCode::UnknownNodeType, functionTypeStr,
                        "Unknown GenericFunction type: " + type,
                        "Provide a valid GenericFunction type"});
      return std::nullopt;
    }
    return metaData.value().get().options;
  }
  case epoch_core::GenericFunctionType::StopLoss: {
    auto metaData = stop_loss::Registry::GetInstance().GetMetaData(type);
    if (!metaData) {
      issues.push_back({ValidationCode::UnknownNodeType, functionTypeStr,
                        "Unknown GenericFunction type: " + type,
                        "Provide a valid GenericFunction type"});
      return std::nullopt;
    }
    return metaData.value().get().options;
  }
  default:
    issues.push_back({ValidationCode::UnknownNodeType, functionTypeStr,
                      "Unknown GenericFunction type: " + type,
                      "Provide a valid GenericFunction type"});
    break;
  }
  return std::nullopt;
}

void ValidateGenericFunctionArgs(MetaDataArgDefinitionMapping args,
                                 const MetaDataOptionList &options,
                                 std::string const &functionType,
                                 ValidationIssues &issues) {

  for (auto const &option : options) {
    if (!args.contains(option.id) && option.isRequired) {
      issues.push_back({ValidationCode::InvalidOptionReference, option,
                        "Option '" + option.id + "' is not defined",
                        "Provide a valid option"});
    } else if (args.contains(option.id)) {
      auto arg = args.at(option.id);
      if (!arg.IsType(option.type)) {
        issues.push_back(
            {ValidationCode::InvalidOptionReference, option,
             std::format(
                 "Option '{}' has invalid type for function '{}'. Expected "
                 "type: "
                 "{} but got: {}. This will cause runtime errors.",
                 option.id, functionType,
                 epoch_core::MetaDataOptionTypeWrapper::ToString(option.type),
                 arg.ToString()),
             std::format(
                 "Change option '{}' value in function '{}' to type {}. "
                 "Example: "
                 "{}",
                 option.id, functionType,
                 epoch_core::MetaDataOptionTypeWrapper::ToString(option.type),
                 option.defaultValue ? option.defaultValue->ToString()
                                     : "valid_value")});
      }

      // Validate numeric range for Integer and Decimal types
      if (option.type == epoch_core::MetaDataOptionType::Integer ||
          option.type == epoch_core::MetaDataOptionType::Decimal) {
        double numericValue = arg.GetNumericValue();
        if (numericValue < option.min || numericValue > option.max) {
          issues.push_back(
              {ValidationCode::OptionValueOutOfRange, option,
               std::format(
                   "Option '{}' value {} is out of range for function '{}'. "
                   "Expected range: [{}, {}]. This may cause unexpected "
                   "behavior.",
                   option.id, numericValue, functionType, option.min,
                   option.max),
               std::format(
                   "Set option '{}' in function '{}' to a value between {} and "
                   "{}. "
                   "Suggested value: {}",
                   option.id, functionType, option.min, option.max,
                   option.defaultValue
                       ? option.defaultValue->ToString()
                       : std::to_string((option.min + option.max) / 2))});
        }
      }

      args.erase(option.id);
    }
  }

  for (auto const &arg : args) {
    issues.push_back({ValidationCode::InvalidOptionReference, functionType,
                      "Argument '" + arg.first + "' is not defined",
                      "Provide a valid argument"});
  }
}

} // namespace epoch_metadata::strategy
