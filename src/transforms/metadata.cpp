//
// Created by dewe on 1/10/23.
//
#include "epoch_metadata/transforms/metadata.h"
#include "doc_deserialization_helper.h"
#include <array>
#include <cctype>
#include <epoch_core/ranges_to.h>
#include <initializer_list>
#include <ranges>
#include <sstream>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata::transforms {
void IOMetaData::decode(const YAML::Node &element) {
  if (element.IsScalar()) {
    *this =
        epoch_core::lookup(IOMetaDataConstants::MAP, element.as<std::string>());
  } else {
    id = element["id"].as<std::string>();
    name = element["name"].as<std::string>("");
    type = epoch_core::IODataTypeWrapper::FromString(
        element["type"].as<std::string>());
    allowMultipleConnections =
        element["allowMultipleConnections"].as<bool>(true);
  }
}

void TransformsMetaData::decode(const YAML::Node &element) {
  id = element["id"].as<std::string>();
  name = element["name"].as<std::string>();
  type = epoch_core::TransformTypeWrapper::FromString(
      element["type"].as<std::string>());
  inputs =
      element["inputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
  outputs =
      element["outputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
  options = element["options"].as<MetaDataOptionList>(MetaDataOptionList{});
  desc = MakeDescLink(element["desc"].as<std::string>(""));
  tags =
      element["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
  isCrossSectional = element["isCrossSectional"].as<bool>(false);
}

TransformsMetaData MakeZeroIndexSelectMetaData(std::string const &name);

TransformsMetaData MakeBooleanSelectMetaData(std::string const &id,
                                             std::string const &name) {
  return {
      .id = id,
      .name = name,
      .options = {},
      .type = epoch_core::TransformType::ControlFlow,
      .isCrossSectional = false,
      .desc = "Selects between two inputs based on a boolean condition. "
              "When condition is true, passes through the 'True Value' "
              "input, otherwise passes through the 'False Value' input.",
      .inputs = {{epoch_core::IODataType::Boolean, "condition", "Condition"},
                 {epoch_core::IODataType::Any, "true", "True Value"},
                 {epoch_core::IODataType::Any, "false", "False Value"}},
      .outputs = {IOMetaDataConstants::ANY_OUTPUT_METADATA}};
}

TransformsMetaData MakeEqualityTransformMetaData(std::string const &id,
                                                 std::string const &name) {
  TransformsMetaData metadata;

  metadata.id = id;
  metadata.name = name;

  metadata.type =
      epoch_core::TransformType::MathOperator; // Adjust type as necessary
  metadata.isCrossSectional = false;
  metadata.desc = name;
  metadata.tags = {"math", "comparison", "equal", "operator"};

  // Inputs
  if (id == "eq") {
    metadata.inputs = {IOMetaDataConstants::ANY_INPUT0_METADATA,
                       IOMetaDataConstants::ANY_INPUT1_METADATA};
  } else {
    metadata.inputs = {IOMetaDataConstants::NUMBER_INPUT0_METADATA,
                       IOMetaDataConstants::NUMBER_INPUT1_METADATA};
  }

  // Output
  metadata.outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA};

  return metadata;
}

TransformsMetaData MakeZeroIndexSelectMetaData(size_t N) {
  TransformsMetaData metadata;
  metadata.id = std::format("select_{}", N);
  metadata.name = std::format("Switch {} Inputs", N);
  metadata.options = {}; // Add any specific options if needed
  metadata.type = epoch_core::TransformType::ControlFlow;
  metadata.isCrossSectional = false;
  metadata.desc = "Selects one of " + std::to_string(N) +
                  " inputs based on a zero-indexed selector value";
  metadata.tags = {"flow-control", "selector", "switch", "conditional"};

  // Inputs: "index", "option_0", "option_1", ..., "option_{N-1}"
  std::vector<IOMetaData> inputs;
  inputs.emplace_back(epoch_core::IODataType::Integer, "index", "Index");
  for (size_t i = 0; i < N; ++i) {
    inputs.emplace_back(epoch_core::IODataType::Any, std::format("*{}", i),
                        std::to_string(i), false);
  }
  metadata.inputs = inputs;

  // Output: "selected"
  metadata.outputs = {IOMetaDataConstants::ANY_OUTPUT_METADATA};

  return metadata;
}

TransformsMetaData MakeLogicalTransformMetaData(std::string const &name) {
  TransformsMetaData metadata;

  std::stringstream ss;

  auto trimmedName =
      name | std::views::transform([](char x) {
        return static_cast<char>(x == ' ' ? '_' : std::tolower(x));
      }) |
      epoch_core::ranges::to_string_v;
  metadata.id = std::format("logical_{}", trimmedName);
  metadata.name = name;
  metadata.options = {}; // Add any specific options if needed
  metadata.type =
      epoch_core::TransformType::MathOperator; // Adjust type as necessary
  metadata.isCrossSectional = false;
  metadata.desc = name;
  metadata.tags = {"logic", "boolean", "operator", trimmedName};

  // Inputs
  if (trimmedName == "not") {
    metadata.inputs = {IOMetaDataConstants::BOOLEAN_INPUT_METADATA};
  } else {
    metadata.inputs = {IOMetaDataConstants::BOOLEAN_INPUT0_METADATA,
                       IOMetaDataConstants::BOOLEAN_INPUT1_METADATA};
  }

  // Output
  metadata.outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA};

  return metadata;
}

TransformsMetaData
MakePreviousValueCompareMetaData(std::string const &id, std::string const &name,
                                 std::string const &desc,
                                 std::string const &operator_name) {
  TransformsMetaData metadata;

  metadata.id = id;
  metadata.name = name;
  metadata.type = epoch_core::TransformType::MathOperator;
  metadata.isCrossSectional = false;
  metadata.desc = desc;
  metadata.tags = {"comparison", "temporal", "previous", operator_name,
                   "lookback"};

  // Options
  metadata.options = {
      MetaDataOption{.id = "periods",
                     .name = "Lookback Periods",
                     .type = epoch_core::MetaDataOptionType::Integer,
                     .defaultValue = MetaDataOptionDefinition(1.0),
                     .isRequired = true}};

  // Inputs
  metadata.inputs = {IOMetaDataConstants::DECIMAL_INPUT_METADATA};

  // Output
  metadata.outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA};

  return metadata;
}

TransformsMetaData MakeRangeValueCompareMetaData(
    std::string const &id, std::string const &name, std::string const &desc,
    std::string const &operator_name, bool is_highest) {
  TransformsMetaData metadata;

  metadata.id = id;
  metadata.name = name;
  metadata.type = epoch_core::TransformType::MathOperator;
  metadata.isCrossSectional = false;
  metadata.desc = desc;
  metadata.tags = {"comparison", "temporal", is_highest ? "highest" : "lowest",
                   operator_name, "lookback"};

  // Options
  metadata.options = {
      MetaDataOption{.id = "periods",
                     .name = "Lookback Periods",
                     .type = epoch_core::MetaDataOptionType::Integer,
                     .defaultValue = MetaDataOptionDefinition(14.0),
                     .isRequired = true}};

  // Inputs
  metadata.inputs = {IOMetaDataConstants::DECIMAL_INPUT_METADATA};

  // Output
  metadata.outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA};

  return metadata;
}

std::vector<TransformsMetaData> MakeComparativeMetaData() {
  std::vector<TransformsMetaData> metadataList;

  // Vector comparison operators (gt, lt, eq, etc.)
  for (auto const &[id, name] :
       std::initializer_list<std::array<std::string, 2>>{
           {"gt", "Greater Than"},
           {"gte", "Greater Than or Equal"},
           {"lt", "Less Than"},
           {"lte", "Less Than or Equal"},
           {"eq", "Equal"},
           {"neq", "Not Equal"}}) {
    metadataList.emplace_back(MakeEqualityTransformMetaData(id, name));
  }

  // Boolean select (if/else)
  metadataList.emplace_back(
      MakeBooleanSelectMetaData("boolean_select", "If Else"));

  // N-way selectors (select_2, select_3, etc.)
  for (size_t i = 2; i <= 5; ++i) {
    metadataList.emplace_back(MakeZeroIndexSelectMetaData(i));
  }

  // Logical operators
  for (auto const &name : {"OR", "AND", "NOT", "AND NOT", "XOR"}) {
    metadataList.emplace_back(MakeLogicalTransformMetaData(name));
  }

  // Previous value comparison operators
  metadataList.emplace_back(MakePreviousValueCompareMetaData(
      "crosses_above", "Crosses Above Previous",
      "Signals when the current value is greater than the value N periods ago.",
      "greater"));

  metadataList.emplace_back(MakePreviousValueCompareMetaData(
      "crosses_below", "Crosses Below Previous",
      "Signals when the current value is less than the value N periods ago.",
      "less"));

  metadataList.emplace_back(MakePreviousValueCompareMetaData(
      "crosses_equal", "Equals Previous",
      "Signals when the current value equals the value N periods ago.",
      "equal"));

  // Highest value comparison operators
  metadataList.emplace_back(MakeRangeValueCompareMetaData(
      "higher_than_highest", "Higher Than Highest",
      "Signals when the current value is greater than the highest value within "
      "the past N periods.",
      "greater", true));

  metadataList.emplace_back(
      MakeRangeValueCompareMetaData("at_highest", "At Highest",
                                    "Signals when the current value equals the "
                                    "highest value within the past N periods.",
                                    "equal", true));

  // Lowest value comparison operators
  metadataList.emplace_back(MakeRangeValueCompareMetaData(
      "lower_than_lowest", "Lower Than Lowest",
      "Signals when the current value is less than the lowest value within the "
      "past N periods.",
      "less", false));

  metadataList.emplace_back(
      MakeRangeValueCompareMetaData("at_lowest", "At Lowest",
                                    "Signals when the current value equals the "
                                    "lowest value within the past N periods.",
                                    "equal", false));

  return metadataList;
}

std::vector<TransformsMetaData> MakeScalarMetaData() {
  std::vector<TransformsMetaData> metadataList;

  metadataList.emplace_back(TransformsMetaData{
      .id = "number",
      .name = "Number",
      .options =
          {
              MetaDataOption{.id = "value",
                             .name = "",
                             .type = epoch_core::MetaDataOptionType::Decimal},
          },
      .type = epoch_core::TransformType::Scalar,
      .desc = "Outputs a constant numeric value. Useful for injecting fixed "
              "numbers into a pipeline.",
      .outputs = {IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
      .tags = {"scalar", "constant", "number"}});

  for (bool boolConstant : {true, false}) {
    metadataList.emplace_back(TransformsMetaData{
        .id = std::format("bool_{}", boolConstant),
        .name = std::format("Boolean {}", boolConstant),
        .options = {},
        .type = epoch_core::TransformType::Scalar,
        .desc =
            std::format("Outputs a constant boolean value of {}", boolConstant),
        .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
        .tags = {"scalar", "constant", "boolean"}});
  }

  for (auto const &[id, name] :
       std::initializer_list<std::array<std::string, 2>>{
           {"one", "One"},
           {"negative_one", "Negative One"},
           {"zero", "Zero"},
           {"pi", "Pi"},
           {"e", "Euler's Number"},
           {"phi", "Golden Ratio"},
           {"sqrt2", "Square Root of 2"},
           {"sqrt3", "Square Root of 3"},
           {"sqrt5", "Square Root of 5"},
           {"ln2", "Natural Log of 2"},
           {"ln10", "Natural Log of 10"},
           {"log2", "Log Base 2"},
           {"log10", "Log Base 10"},
           {"log2e", "Log Base 2 of Euler's Number"},
           {"log10e", "Log Base 10 of Euler's Number"},
           {"ln2e", "Natural Log of Euler's Number"},
           {"ln10e", "Natural Log of Euler's Number"}}) {
    metadataList.emplace_back(TransformsMetaData{
        .id = id,
        .name = name,
        .options = {},
        .type = epoch_core::TransformType::Scalar,
        .desc = name,
        .outputs = {IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
        .tags = {"scalar", "constant", "math", "number"}});
  }

  return metadataList;
}

std::vector<TransformsMetaData> MakeDataSource() {
  std::vector<TransformsMetaData> result;

  // Refactored Names Applied Below
  result.emplace_back(TransformsMetaData{
      .id = "market_data_source",
      .name = "Market Data Source",
      .options = {},
      .type = epoch_core::TransformType::DataSource,
      .desc = "Provides open, high, low, close, and volume data for a market "
              "instrument.",
      .outputs = {IOMetaDataConstants::OPEN_PRICE_METADATA,
                  IOMetaDataConstants::HIGH_PRICE_METADATA,
                  IOMetaDataConstants::LOW_PRICE_METADATA,
                  IOMetaDataConstants::CLOSE_PRICE_METADATA,
                  IOMetaDataConstants::VOLUME_METADATA},
      .tags = {"data", "source", "price", "ohlcv"},
  });

  return result;
}

std::vector<TransformsMetaData> MakeTradeSignalExecutor() {
  std::vector<TransformsMetaData> result;

  IOMetaData longMetaData{.type = epoch_core::IODataType::Boolean,
                          .id = "long",
                          .name = "Enter Long Trade"};

  IOMetaData shortMetaData{.type = epoch_core::IODataType::Boolean,
                           .id = "short",
                           .name = "Enter Short Trade"};

  IOMetaData closePositionMetaData{
      .type = epoch_core::IODataType::Boolean,
      .id = "close",
      .name = "Exit Trade",
  };

  IOMetaData allowSignalsMetaData{.type = epoch_core::IODataType::Boolean,
                                  .id = "allow",
                                  .name = "Allow Trading",
                                  .allowMultipleConnections = false};

  MetaDataOption closeIfIndecisive{
      .id = "closeIfIndecisive",
      .name = "Exit If Indecisive",
      .type = epoch_core::MetaDataOptionType::Boolean,
      .defaultValue = false,
  };

  return {
      TransformsMetaData{.id = "trade_signal_executor",
                         .name = "Trade Signal Executor",
                         .options = {closeIfIndecisive},
                         .type = epoch_core::TransformType::TradeSignalExecutor,
                         .desc = "Executes trade signals. If allow is true, "
                                 "all other signals are ignored.",
                         .inputs = {allowSignalsMetaData, longMetaData,
                                    shortMetaData, closePositionMetaData},
                         .atLeastOneInputRequired = true}};
}

} // namespace epoch_metadata::transforms