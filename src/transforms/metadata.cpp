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
#include <unordered_map>
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
    isFilter = element["isFilter"].as<bool>(false);
  }
}

void TransformsMetaData::decode(const YAML::Node &element) {
  id = element["id"].as<std::string>();
  name = element["name"].as<std::string>();
  category = epoch_core::TransformCategoryWrapper::FromString(
      element["category"].as<std::string>());
  renderKind = epoch_core::TransformNodeRenderKindWrapper::FromString(
      element["renderKind"].as<std::string>());
  plotKind = epoch_core::TransformPlotKindWrapper::FromString(
      element["plotKind"].as<std::string>("Null"));
  inputs =
      element["inputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
  outputs =
      element["outputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
  options = element["options"].as<MetaDataOptionList>(MetaDataOptionList{});
  desc = MakeDescLink(element["desc"].as<std::string>(""));
  tags =
      element["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
  isCrossSectional = element["isCrossSectional"].as<bool>(false);
  requiresTimeFrame = element["requiresTimeFrame"].as<bool>(false);
}

TransformsMetaData MakeZeroIndexSelectMetaData(std::string const &name);

TransformsMetaData MakeBooleanSelectMetaData(std::string const &id,
                                             std::string const &name) {
  return {
      .id = id,
      .category = epoch_core::TransformCategory::ControlFlow,
      .renderKind = epoch_core::TransformNodeRenderKind::Gate,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = name,
      .options = {},
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

  metadata.renderKind = epoch_core::TransformNodeRenderKind::Operator;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;

  metadata.isCrossSectional = false;
  metadata.desc = name;
  metadata.tags = {"math", "comparison", name, "operator"};

  // Inputs
  if (id.ends_with("eq")) {
    metadata.inputs = {IOMetaDataConstants::ANY_INPUT0_METADATA,
                       IOMetaDataConstants::ANY_INPUT1_METADATA};
    metadata.category = epoch_core::TransformCategory::Utility;
  } else {
    metadata.category = epoch_core::TransformCategory::Math;
    metadata.inputs = {IOMetaDataConstants::NUMBER_INPUT0_METADATA,
                       IOMetaDataConstants::NUMBER_INPUT1_METADATA};
    metadata.category = epoch_core::TransformCategory::Math;
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
  metadata.category = epoch_core::TransformCategory::ControlFlow;
  metadata.renderKind = epoch_core::TransformNodeRenderKind::DynamicSelect;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
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
  metadata.category = epoch_core::TransformCategory::Math;
  metadata.renderKind = epoch_core::TransformNodeRenderKind::Operator;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
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

TransformsMetaData MakeValueCompareMetaData(
    const std::string &value_type,    // "previous", "highest", or "lowest"
    const std::string &operator_type, // "gt", "gte", "lt", "lte", "eq", "neq"
    int default_periods = 14, const std::string &custom_id = "",
    const std::string &custom_name = "") {

  // Map operator types to friendly names and enum values
  static const std::unordered_map<std::string,
                                  std::pair<std::string, std::string>>
      operator_map = {{"gt", {"Greater Than", "GreaterThan"}},
                      {"gte", {"Greater Than or Equal", "GreaterThanOrEquals"}},
                      {"lt", {"Less Than", "LessThan"}},
                      {"lte", {"Less Than or Equal", "LessThanOrEquals"}},
                      {"eq", {"Equal", "Equals"}},
                      {"neq", {"Not Equal", "NotEquals"}}};

  // Map value types to friendly names and tags
  static const std::unordered_map<
      std::string, std::pair<std::string, std::vector<std::string>>>
      value_type_map = {
          {"previous",
           {"Previous Value",
            {"comparison", "temporal", "previous", "lookback"}}},
          {"highest",
           {"Highest Value",
            {"comparison", "temporal", "highest", "lookback", "max"}}},
          {"lowest",
           {"Lowest Value",
            {"comparison", "temporal", "lowest", "lookback", "min"}}}};

  // Get the operator info
  auto op_it = operator_map.find(operator_type);
  if (op_it == operator_map.end()) {
    throw std::runtime_error("Invalid operator type: " + operator_type);
  }
  const auto &[op_name, op_enum] = op_it->second;

  // Get the value type info
  auto val_it = value_type_map.find(value_type);
  if (val_it == value_type_map.end()) {
    throw std::runtime_error("Invalid value type: " + value_type);
  }
  const auto &[val_name, tags] = val_it->second;

  // Create unique ID and name
  std::string id =
      custom_id.empty() ? (value_type + "_" + operator_type) : custom_id;
  std::string name =
      custom_name.empty() ? (op_name + " " + val_name) : custom_name;

  // Create description based on type and operator
  std::string desc;
  if (value_type == "previous") {
    desc = "Signals when the current value is " + op_name + " the value " +
           std::to_string(default_periods) + " period(s) ago.";
  } else if (value_type == "highest") {
    desc = "Signals when the current value is " + op_name +
           " the highest value within " + "the past " +
           std::to_string(default_periods) + " periods.";
  } else { // lowest
    desc = "Signals when the current value is " + op_name +
           " the lowest value within " + "the past " +
           std::to_string(default_periods) + " periods.";
  }

  TransformsMetaData metadata;
  metadata.id = id;
  metadata.name = name;
  metadata.category = epoch_core::TransformCategory::Math;
  metadata.renderKind = epoch_core::TransformNodeRenderKind::Standard;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
  metadata.isCrossSectional = false;
  metadata.desc = desc;
  metadata.tags = tags;

  // Period option
  metadata.options = {MetaDataOption{
      .id = "periods",
      .name = "Lookback Periods",
      .type = epoch_core::MetaDataOptionType::Integer,
      .defaultValue =
          MetaDataOptionDefinition(static_cast<double>(default_periods)),
      .isRequired = true}};

  // Input/Output
  metadata.inputs = {IOMetaDataConstants::DECIMAL_INPUT_METADATA};
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

  // All temporal comparison operators (18 combinations)
  // Previous value comparisons (with default period = 1)
  for (const auto &op : {"gt", "gte", "lt", "lte", "eq", "neq"}) {
    metadataList.emplace_back(MakeValueCompareMetaData("previous", op, 1));
  }

  // Highest value comparisons (with default period = 14)
  for (const auto &op : {"gt", "gte", "lt", "lte", "eq", "neq"}) {
    metadataList.emplace_back(MakeValueCompareMetaData("highest", op, 14));
  }

  // Lowest value comparisons (with default period = 14)
  for (const auto &op : {"gt", "gte", "lt", "lte", "eq", "neq"}) {
    metadataList.emplace_back(MakeValueCompareMetaData("lowest", op, 14));
  }

  return metadataList;
}

std::vector<TransformsMetaData> MakeScalarMetaData() {
  std::vector<TransformsMetaData> metadataList;

  metadataList.emplace_back(TransformsMetaData{
      .id = "number",
      .category = epoch_core::TransformCategory::Scalar,
      .renderKind = epoch_core::TransformNodeRenderKind::NumberInput,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Number",
      .options =
          {
              MetaDataOption{.id = "value",
                             .name = "",
                             .type = epoch_core::MetaDataOptionType::Decimal},
          },
      .desc = "Outputs a constant numeric value. Useful for injecting fixed "
              "numbers into a pipeline.",
      .outputs = {IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
      .tags = {"scalar", "constant", "number"}});

  for (bool boolConstant : {true, false}) {
    metadataList.emplace_back(TransformsMetaData{
        .id = std::format("bool_{}", boolConstant),
        .category = epoch_core::TransformCategory::Scalar,
        .renderKind = epoch_core::TransformNodeRenderKind::Label,
        .plotKind = epoch_core::TransformPlotKind::Null,
        .name = std::format("Boolean {}", boolConstant),
        .options = {},
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
           {"log2e", "Log Base 2 of Euler's Number"},
           {"log10e", "Log Base 10 of Euler's Number"}}) {
    metadataList.emplace_back(TransformsMetaData{
        .id = id,
        .category = epoch_core::TransformCategory::Scalar,
        .renderKind = epoch_core::TransformNodeRenderKind::Label,
        .plotKind = epoch_core::TransformPlotKind::Null,
        .name = name,
        .options = {},
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
      .category = epoch_core::TransformCategory::DataSource,
      .renderKind = epoch_core::TransformNodeRenderKind::Input,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Market Data Source",
      .options = {},
      .desc = "Provides open, high, low, close, and volume data for a market "
              "instrument.",
      .outputs = {IOMetaDataConstants::OPEN_PRICE_METADATA,
                  IOMetaDataConstants::HIGH_PRICE_METADATA,
                  IOMetaDataConstants::LOW_PRICE_METADATA,
                  IOMetaDataConstants::CLOSE_PRICE_METADATA,
                  IOMetaDataConstants::VOLUME_METADATA},
      .tags = {"data", "source", "price", "ohlcv"},
      .requiresTimeFrame = true});

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
                                  .allowMultipleConnections = false,
                                  .isFilter = true};

  MetaDataOption closeIfIndecisive{
      .id = "closeIfIndecisive",
      .name = "Exit If Indecisive",
      .type = epoch_core::MetaDataOptionType::Boolean,
      .defaultValue = false,
  };

  return {TransformsMetaData{
      .id = "trade_signal_executor",
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Trade Signal Executor",
      .options = {closeIfIndecisive},
      .desc = "Executes trade signals. If allow is true, "
              "all other signals are ignored.",
      .inputs = {allowSignalsMetaData, longMetaData, shortMetaData,
                 closePositionMetaData},
      .atLeastOneInputRequired = true,
      .requiresTimeFrame = false}};
}

std::vector<TransformCategoryMetaData> MakeTransformCategoryMetaData() {
  return {{epoch_core::TransformCategory::Aggregate, "Aggregate",
           "Nodes for combining multiple data inputs"},
          {epoch_core::TransformCategory::ControlFlow, "Control Flow",
           "Nodes for conditional logic and flow control"},
          {epoch_core::TransformCategory::Scalar, "Scalar",
           "Nodes for constants, booleans, and editable numbers"},
          {epoch_core::TransformCategory::DataSource, "Data Source",
           "Nodes for market data and fundamental feeds"},
          {epoch_core::TransformCategory::Math, "Math",
           "Nodes for mathematical and statistical operations"},
          {epoch_core::TransformCategory::Trend, "Trend",
           "Nodes for trend identification and analysis"},
          {epoch_core::TransformCategory::Momentum, "Momentum",
           "Nodes for momentum-based market analysis"},
          {epoch_core::TransformCategory::Volatility, "Volatility",
           "Nodes for measuring market volatility"},
          {epoch_core::TransformCategory::Volume, "Volume",
           "Nodes for volume-based market analysis"},
          {epoch_core::TransformCategory::PriceAction, "Price Action",
           "Nodes for price pattern recognition"},
          {epoch_core::TransformCategory::Statistical, "Statistical",
           "Nodes for advanced statistical analysis"},
          {epoch_core::TransformCategory::Factor, "Factor",
           "Nodes for cross-sectional analysis"},
          {epoch_core::TransformCategory::Utility, "Utility",
           "Helper nodes for various operations"},
          {epoch_core::TransformCategory::Executor, "Executor",
           "Nodes for trade execution and order management"}};
}

} // namespace epoch_metadata::transforms