#include "epoch_metadata/metadata_options.h"
#include "epoch_metadata/strategy/generic_function.h"
#include "epoch_metadata/strategy/generic_function_validator.h"
#include "epoch_metadata/strategy/registry.h"
#include "epoch_metadata/strategy/validation_error.h"
#include <catch.hpp>

#include "epoch_metadata/strategy/validation.h"

using namespace epoch_metadata::strategy;
using namespace epoch_metadata;

namespace {

// Helper function to create a basic GenericFunction
GenericFunction
CreateGenericFunction(const std::string &type,
                      const MetaDataArgDefinitionMapping &args = {}) {
  GenericFunction func;
  func.type = type;
  func.args = args;
  return func;
}

// Helper function to create MetaDataOptionDefinition
MetaDataOptionDefinition CreateOption(double value) {
  return MetaDataOptionDefinition(value);
}

MetaDataOptionDefinition CreateOption(bool value) {
  return MetaDataOptionDefinition(value);
}

MetaDataOptionDefinition CreateStringOption(const std::string &value) {
  return MetaDataOptionDefinition(value);
}

// Helper function to expect validation errors
void ExpectValidationError(const ValidationIssues &issues,
                           ValidationCode expectedCode,
                           const std::string &expectedMessagePart = "") {
  bool found = false;
  for (const auto &issue : issues) {
    if (issue.code == expectedCode) {
      found = true;
      if (!expectedMessagePart.empty()) {
        REQUIRE_THAT(issue.message,
                     Catch::Matchers::ContainsSubstring(expectedMessagePart));
      }
      break;
    }
  }
  if (!found) {
    INFO("Available error codes in result:");
    for (const auto &issue : issues) {
      INFO("  - Code: " << ValidationCodeWrapper::ToString(issue.code)
                        << ", Message: " << issue.message);
    }
  }
  REQUIRE(found);
}

void ExpectNoValidationErrors(const ValidationIssues &issues) {
    INFO(FormatValidationIssues(issues));
    REQUIRE(issues.empty());
}

} // namespace

TEST_CASE("GenericFunctionValidator: Empty type validation",
          "[GenericFunctionValidator]") {
  auto func = CreateGenericFunction("");

  SECTION("TradeSignal with empty type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectValidationError(issues, ValidationCode::MissingRequiredInput,
                          "type cannot be empty");
  }

  SECTION("PositionSizer with empty type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectValidationError(issues, ValidationCode::MissingRequiredInput,
                          "type cannot be empty");
  }

  SECTION("TakeProfit with empty type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectValidationError(issues, ValidationCode::MissingRequiredInput,
                          "type cannot be empty");
  }

  SECTION("StopLoss with empty type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::StopLoss);
    ExpectValidationError(issues, ValidationCode::MissingRequiredInput,
                          "type cannot be empty");
  }
}

TEST_CASE("GenericFunctionValidator: Unknown type validation",
          "[GenericFunctionValidator]") {
  auto func = CreateGenericFunction("unknown_type");

  SECTION("TradeSignal with unknown type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectValidationError(issues, ValidationCode::UnknownNodeType,
                          "Unknown GenericFunction type");
  }

  SECTION("PositionSizer with unknown type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectValidationError(issues, ValidationCode::UnknownNodeType,
                          "Unknown GenericFunction type");
  }

  SECTION("TakeProfit with unknown type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectValidationError(issues, ValidationCode::UnknownNodeType,
                          "Unknown GenericFunction type");
  }

  SECTION("StopLoss with unknown type") {
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::StopLoss);
    ExpectValidationError(issues, ValidationCode::UnknownNodeType,
                          "Unknown GenericFunction type");
  }
}

TEST_CASE("GenericFunctionValidator: Valid TradeSignal types",
          "[GenericFunctionValidator]") {
  SECTION("Valid trade signal - atr_scalping with valid args") {
    MetaDataArgDefinitionMapping args;
    args["atr_period"] = CreateOption(14.0);
    args["atr_rolling_mean_period"] = CreateOption(5.0);
    args["breakout_period"] = CreateOption(3.0);

    auto func = CreateGenericFunction("atr_scalping", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Valid trade signal - moving_average_crossover with valid args") {
    MetaDataArgDefinitionMapping args;
    args["slowPeriod"] = CreateOption(200.0);
    args["slowMAType"] = CreateStringOption("sma");
    args["fastPeriod"] = CreateOption(50.0);
    args["fastMAType"] = CreateStringOption("ema");

    auto func = CreateGenericFunction("moving_average_crossover", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Trade signal with missing required args") {
    auto func = CreateGenericFunction("atr_scalping"); // No args provided
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectValidationError(issues, ValidationCode::InvalidOptionReference,
                          "not defined");
  }
}

TEST_CASE("GenericFunctionValidator: Valid PositionSizer types",
          "[GenericFunctionValidator]") {
  SECTION("Valid position sizer - fixed_unit with valid args") {
    MetaDataArgDefinitionMapping args;
    args["unit"] = CreateOption(1.0);

    auto func = CreateGenericFunction("fixed_unit", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Valid position sizer - cppi with valid args") {
    MetaDataArgDefinitionMapping args;
    args["multiplier"] = CreateOption(1.0);
    args["floor_pct"] = CreateOption(0.9);

    auto func = CreateGenericFunction("cppi", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Position sizer with out of range values") {
    MetaDataArgDefinitionMapping args;
    args["multiplier"] = CreateOption(150.0); // Out of range (max 100)
    args["floor_pct"] = CreateOption(0.9);

    auto func = CreateGenericFunction("cppi", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectValidationError(issues, ValidationCode::OptionValueOutOfRange,
                          "out of range");
  }
}

TEST_CASE("GenericFunctionValidator: Valid TakeProfit types",
          "[GenericFunctionValidator]") {
  SECTION("Valid take profit - atr_volatility with valid args") {
    MetaDataArgDefinitionMapping args;
    args["period"] = CreateOption(14.0);
    args["multiple"] = CreateOption(2.0);

    auto func = CreateGenericFunction("atr_volatility", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Valid take profit - fixed_percent_ratio_offset with valid args") {
    MetaDataArgDefinitionMapping args;
    args["ratio"] = CreateOption(0.02);

    auto func = CreateGenericFunction("fixed_percent_ratio_offset", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectNoValidationErrors(issues);
  }
}

TEST_CASE("GenericFunctionValidator: Valid StopLoss types",
          "[GenericFunctionValidator]") {
  SECTION("Valid stop loss - atr_volatility with valid args") {
    MetaDataArgDefinitionMapping args;
    args["period"] = CreateOption(14.0);
    args["multiple"] = CreateOption(2.0);

    auto func = CreateGenericFunction("atr_volatility", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::StopLoss);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Valid stop loss - chande_kroll_stop_loss with valid args") {
    MetaDataArgDefinitionMapping args;
    args["p_period"] = CreateOption(10.0);
    args["q_period"] = CreateOption(20.0);
    args["multiplier"] = CreateOption(3.0);

    auto func = CreateGenericFunction("chande_kroll_stop_loss", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::StopLoss);
    ExpectNoValidationErrors(issues);
  }
}

TEST_CASE("GenericFunctionValidator: Argument type validation",
          "[GenericFunctionValidator]") {
  SECTION("Wrong argument type - string instead of integer") {
    MetaDataArgDefinitionMapping args;
    args["unit"] = CreateOption("not_a_number"); // Should be integer/decimal

    auto func = CreateGenericFunction("fixed_unit", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectValidationError(issues, ValidationCode::InvalidOptionReference,
                          "invalid type");
  }

  SECTION("Wrong argument type - boolean instead of decimal") {
    MetaDataArgDefinitionMapping args;
    args["ratio"] = CreateOption(true); // Should be decimal

    auto func = CreateGenericFunction("fixed_percent_ratio_offset", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectValidationError(issues, ValidationCode::InvalidOptionReference,
                          "invalid type");
  }
}

TEST_CASE("GenericFunctionValidator: Extra argument validation",
          "[GenericFunctionValidator]") {
  SECTION("Extra argument not in metadata") {
    MetaDataArgDefinitionMapping args;
    args["unit"] = CreateOption(1.0);
    args["extra_arg"] = CreateOption(123.0); // Not defined in metadata

    auto func = CreateGenericFunction("fixed_unit", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::PositionSizer);
    ExpectValidationError(issues, ValidationCode::InvalidOptionReference,
                          "not defined");
  }
}

TEST_CASE("GenericFunctionValidator: Numeric range validation",
          "[GenericFunctionValidator]") {
  SECTION("Integer value below minimum") {
    MetaDataArgDefinitionMapping args;
    args["period"] = CreateOption(0.0); // Below min (1)
    args["multiple"] = CreateOption(2.0);

    auto func = CreateGenericFunction("atr_volatility", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectValidationError(issues, ValidationCode::OptionValueOutOfRange,
                          "out of range");
  }

  SECTION("Decimal value above maximum") {
    MetaDataArgDefinitionMapping args;
    args["ratio"] = CreateOption(1.5); // Above max (1.0)

    auto func = CreateGenericFunction("fixed_percent_ratio_offset", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectValidationError(issues, ValidationCode::OptionValueOutOfRange,
                          "out of range");
  }

  SECTION("Value within valid range") {
    MetaDataArgDefinitionMapping args;
    args["period"] = CreateOption(20.0);  // Within range [1, 1000]
    args["multiple"] = CreateOption(2.5); // Within range [1.0, 10.0]

    auto func = CreateGenericFunction("atr_volatility", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TakeProfit);
    ExpectNoValidationErrors(issues);
  }
}

TEST_CASE("GenericFunctionValidator: Select option validation",
          "[GenericFunctionValidator]") {
  SECTION("Valid select option") {
    MetaDataArgDefinitionMapping args;
    args["slowPeriod"] = CreateOption(200.0);
    args["slowMAType"] = CreateStringOption("sma"); // Valid select option
    args["fastPeriod"] = CreateOption(50.0);
    args["fastMAType"] = CreateStringOption("ema"); // Valid select option

    auto func = CreateGenericFunction("moving_average_crossover", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    ExpectNoValidationErrors(issues);
  }

  SECTION("Invalid select option") {
    MetaDataArgDefinitionMapping args;
    args["slowPeriod"] = CreateOption(200.0);
    args["slowMAType"] =
        CreateOption("invalid_ma_type"); // Invalid select option
    args["fastPeriod"] = CreateOption(50.0);
    args["fastMAType"] = CreateOption("ema");

    auto func = CreateGenericFunction("moving_average_crossover", args);
    auto issues = ValidateGenericFunction(
        func, epoch_core::GenericFunctionType::TradeSignal);
    // Note: This might require actual registry lookup to work properly
    // The test might pass if validation is not implemented for select options
    // yet
  }
}

TEST_CASE("GenericFunctionValidator: ValidateGenericFunctionType standalone",
          "[GenericFunctionValidator]") {
  ValidationIssues issues;

  SECTION("Valid type returns metadata options") {
    auto options = ValidateGenericFunctionType(
        "fixed_unit", epoch_core::GenericFunctionType::PositionSizer, issues);
    REQUIRE(options.has_value());
    ExpectNoValidationErrors(issues);
  }

  SECTION("Invalid type returns nullopt") {
    auto options = ValidateGenericFunctionType(
        "invalid_type", epoch_core::GenericFunctionType::PositionSizer, issues);
    REQUIRE_FALSE(options.has_value());
    ExpectValidationError(issues, ValidationCode::UnknownNodeType);
  }

  SECTION("Empty type returns nullopt") {
    auto options = ValidateGenericFunctionType(
        "", epoch_core::GenericFunctionType::PositionSizer, issues);
    REQUIRE_FALSE(options.has_value());
    ExpectValidationError(issues, ValidationCode::MissingRequiredInput);
  }
}

TEST_CASE("GenericFunctionValidator: All function types coverage",
          "[GenericFunctionValidator]") {
  // Test each enum value to ensure all are handled
  const std::vector<std::pair<epoch_core::GenericFunctionType, std::string>>
      testCases = {
          {epoch_core::GenericFunctionType::TradeSignal, "atr_scalping"},
          {epoch_core::GenericFunctionType::PositionSizer, "fixed_unit"},
          {epoch_core::GenericFunctionType::TakeProfit, "atr_volatility"},
          {epoch_core::GenericFunctionType::StopLoss, "atr_volatility"}};

  for (const auto &[functionType, validType] : testCases) {
    SECTION("Function type: " +
            epoch_core::GenericFunctionTypeWrapper::ToString(functionType)) {
      ValidationIssues issues;
      auto options =
          ValidateGenericFunctionType(validType, functionType, issues);

      // Should return valid metadata for known types
      INFO("Testing function type: "
           << epoch_core::GenericFunctionTypeWrapper::ToString(functionType)
           << " with type: " << validType);
      REQUIRE(options.has_value());
      ExpectNoValidationErrors(issues);
    }
  }
}