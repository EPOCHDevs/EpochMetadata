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
  if (!function.type) {
    return issues;
  }

  // Validate type
  auto options =
      ValidateGenericFunctionType(function.type.value(), type, issues);
  if (!options) {
    return issues;
  }

  // Validate args
  ValidateGenericFunctionArgs(
      function.args.value_or(MetaDataArgDefinitionMapping{}), options.value(),
      function.type.value(), issues);

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
  case epoch_core::GenericFunctionType::FuturesContinuation: {
    auto metaData =
        futures_continuation::Registry::GetInstance().GetMetaData(type);
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

// ============================================================================
// Optimization Functions Implementation
// ============================================================================

GenericFunction OptimizeGenericFunction(const GenericFunction &function,
                                        epoch_core::GenericFunctionType type) {
  GenericFunction optimizedFunction = function;

  if (!function.type.has_value()) {
    return optimizedFunction; // Cannot optimize without type
  }

  ValidationIssues
      issues; // We don't use this for optimization, just for type validation
  auto options =
      ValidateGenericFunctionType(function.type.value(), type, issues);
  if (!options.has_value()) {
    return optimizedFunction; // Cannot optimize unknown types
  }

  // Apply optimization phases
  ApplyDefaultGenericFunctionOptions(optimizedFunction, options.value());
  ClampGenericFunctionOptionValues(optimizedFunction, options.value());

  // If the function has data, optimize it too
  if (optimizedFunction.data.has_value()) {
    optimizedFunction.data = OptimizeUIData(optimizedFunction.data.value());
  }

  return optimizedFunction;
}

void ApplyDefaultGenericFunctionOptions(GenericFunction &function,
                                        const MetaDataOptionList &options) {
  if (!function.args.has_value()) {
    function.args = MetaDataArgDefinitionMapping{};
  }

  auto &args = function.args.value();

  // Apply defaults for missing required options
  for (const auto &option : options) {
    if (option.isRequired && args.find(option.id) == args.end() &&
        option.defaultValue.has_value()) {

      // Add the missing option with default value
      args[option.id] = option.defaultValue.value();
    }
  }
}

void ClampGenericFunctionOptionValues(GenericFunction &function,
                                      const MetaDataOptionList &options) {
  if (!function.args.has_value()) {
    return; // No args to clamp
  }

  auto &args = function.args.value();

  // Create a map of options for quick lookup
  std::unordered_map<std::string, MetaDataOption> optionMap;
  for (const auto &option : options) {
    optionMap[option.id] = option;
  }

  // Clamp argument values to their allowed ranges
  for (auto &[argId, argValue] : args) {
    if (optionMap.find(argId) == optionMap.end()) {
      continue; // Skip unknown options
    }

    const auto &option = optionMap[argId];

    // Only clamp numeric types (Integer and Decimal)
    if (option.type == epoch_core::MetaDataOptionType::Integer ||
        option.type == epoch_core::MetaDataOptionType::Decimal) {

      double numericValue = argValue.GetNumericValue();

      // Clamp to min/max range
      double clampedValue =
          std::max(option.min, std::min(option.max, numericValue));

      if (clampedValue != numericValue) {
        argValue = MetaDataOptionDefinition(clampedValue);
      }
    }
  }
}

} // namespace epoch_metadata::strategy
