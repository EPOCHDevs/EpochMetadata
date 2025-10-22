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
  requiredDataSources =
      element["requiredDataSources"].as<std::vector<std::string>>(
          std::vector<std::string>{});
  intradayOnly = element["intradayOnly"].as<bool>(false);
  allowNullInputs = element["allowNullInputs"].as<bool>(false);

  // Enhanced metadata for RAG/LLM
  strategyTypes = element["strategyTypes"].as<std::vector<std::string>>(std::vector<std::string>{});
  relatedTransforms = element["relatedTransforms"].as<std::vector<std::string>>(std::vector<std::string>{});
  assetRequirements = element["assetRequirements"].as<std::vector<std::string>>(std::vector<std::string>{});
  usageContext = element["usageContext"].as<std::string>("");
  limitations = element["limitations"].as<std::string>("");
}

TransformsMetaData MakeZeroIndexSelectMetaData(std::string const &name);

TransformsMetaData MakeBooleanSelectMetaData(std::string const &id,
                                             std::string const &name) {
  return {
      .id = id,
      .category = epoch_core::TransformCategory::ControlFlow,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
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
      .outputs = {IOMetaDataConstants::ANY_OUTPUT_METADATA},
      .strategyTypes = {"conditional-logic"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Conditional routing for strategy logic. Route different values based on conditions like time-of-day filters, regime detection, or risk states. Common use: switch between aggressive/conservative position sizing based on volatility regime.",
      .limitations = "Can only choose between two values. For more options, use select_N transforms (select_2, select_3, etc.)."};
}

TransformsMetaData MakeEqualityTransformMetaData(std::string const &id,
                                                 std::string const &name) {
  TransformsMetaData metadata;

  metadata.id = id;
  metadata.name = name;

  metadata.renderKind = epoch_core::TransformNodeRenderKind::Operator;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;

  metadata.isCrossSectional = false;
  metadata.desc = name + " comparison. Returns true when first input " +
                  (id == "gt" ? "is greater than" :
                   id == "gte" ? "is greater than or equal to" :
                   id == "lt" ? "is less than" :
                   id == "lte" ? "is less than or equal to" :
                   id == "eq" ? "equals" : "does not equal") + " second input.";
  metadata.usageContext = "Basic comparison for signal generation. Common uses: price vs MA crossovers, indicator threshold levels, multi-timeframe confirmations.";
  metadata.strategyTypes = {"signal-generation", "threshold-detection"};
  metadata.assetRequirements = {"single-asset"};
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

  // TODO:
  // https://linear.app/epoch-inc/issue/STR-160/update-switch-to-dynamicselect
  metadata.renderKind = epoch_core::TransformNodeRenderKind::Standard;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
  metadata.isCrossSectional = false;
  metadata.desc = "Selects one of " + std::to_string(N) +
                  " inputs based on a zero-indexed selector value";
  metadata.usageContext = "Multi-way routing for strategy logic. Use integer index to select between " + std::to_string(N) + " different values/signals. Common use: regime-based strategy selection where index comes from market state detection (e.g., 0=trend strategy, 1=mean-reversion, 2=defensive).";
  metadata.strategyTypes = {"multi-strategy-selection", "regime-switching", "conditional-routing"};
  metadata.assetRequirements = {"single-asset"};
  metadata.limitations = "Index must be integer 0 to " + std::to_string(N-1) + ". Out-of-range indices may cause errors. For binary choice, use boolean_branch instead.";
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
  metadata.desc = name + " boolean operator for combining conditions.";
  metadata.usageContext = "Combine multiple signals/conditions into complex trading logic. AND for requiring all conditions, OR for any condition, NOT for inverting signals. Common pattern: (price > MA) AND (volume > threshold) for confirmed breakouts.";
  metadata.strategyTypes = {"signal-combination", "conditional-logic", "multi-condition-filtering"};
  metadata.assetRequirements = {"single-asset"};
  metadata.limitations = "Simple boolean logic only - no fuzzy logic or weighted combinations. Chain multiple operators for complex conditions (can become visually cluttered).";
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
  std::string usageContext;
  if (value_type == "previous") {
    desc = "Signals when the current value is " + op_name + " the value " +
           std::to_string(default_periods) + " period(s) ago.";
    usageContext = "Detects momentum and trend changes by comparing current value to historical values. Use for rate-of-change signals, momentum confirmation, or lag-based entry timing. Higher periods = longer-term momentum detection.";
  } else if (value_type == "highest") {
    desc = "Signals when the current value is " + op_name +
           " the highest value within " + "the past " +
           std::to_string(default_periods) + " periods.";
    usageContext = "Identifies breakouts to new highs or pullbacks from highs. 'Greater Than Highest' signals new high breakouts. 'Less Than Highest' indicates pullback depth. Useful for breakout strategies and identifying strength/weakness.";
  } else { // lowest
    desc = "Signals when the current value is " + op_name +
           " the lowest value within " + "the past " +
           std::to_string(default_periods) + " periods.";
    usageContext = "Identifies breakouts to new lows or bounces from lows. 'Less Than Lowest' signals new low breakdowns. 'Greater Than Lowest' indicates bounce strength. Useful for breakdown detection and oversold bounce strategies.";
  }

  TransformsMetaData metadata;
  metadata.id = id;
  metadata.name = name;
  metadata.category = epoch_core::TransformCategory::Math;
  metadata.renderKind = epoch_core::TransformNodeRenderKind::Standard;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
  metadata.isCrossSectional = false;
  metadata.desc = desc;
  metadata.usageContext = usageContext;
  metadata.strategyTypes = {value_type == "previous" ? "momentum" : "breakout", "signal-generation", "threshold-detection"};
  metadata.assetRequirements = {"single-asset"};
  metadata.limitations = "Lagging indicator - signals occur after moves start. Sensitive to lookback period choice. No volatility adjustment.";
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

std::vector<TransformsMetaData> MakeLagMetaData() {
  std::vector<TransformsMetaData> metadataList;

  metadataList.emplace_back(TransformsMetaData{
      .id = "lag",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line,
      .name = "Lag",
      .options = {
          MetaDataOption{.id = "period",
                         .name = "Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(1)),
                         .min = 1,
                         .desc = "Number of periods to shift the data backward",
                         .tuningGuidance = "Lag 1 for previous bar comparison. Larger lags for detecting longer-term patterns or creating features for machine learning models. Common: 1 (prev bar), 5 (prev week on daily), 20 (prev month)."}
      },
      .desc = "Shifts each element in the input by the specified period, "
              "creating a lagged series. Works with any data type.",
      .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
      .outputs = {IOMetaDataConstants::ANY_OUTPUT_METADATA},
      .tags = {"math", "lag", "delay", "shift", "temporal"},
      .requiresTimeFrame = false,
      .strategyTypes = {"feature-engineering", "temporal-comparison"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Access historical values for comparison or feature creation. Use lag(1) to compare current vs previous bar. Combine multiple lags for pattern detection or ML features.",
      .limitations = "Shifts data backward, so first N bars will be null/undefined. Not a predictive transform - only accesses past data."});

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
      .tags = {"scalar", "constant", "number"},
      .strategyTypes = {"parameter-injection", "threshold-setting"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Inject constant values for thresholds, parameters, or fixed position sizes. Common uses: threshold levels for signals (e.g., RSI > 70), fixed position sizing, mathematical constants in calculations.",
      .limitations = "Static value only - cannot adapt to market conditions. For dynamic values, use indicators or calculations."});

    metadataList.emplace_back(TransformsMetaData{
    .id = "text",
    .category = epoch_core::TransformCategory::Scalar,
    .renderKind = epoch_core::TransformNodeRenderKind::Input,
    .plotKind = epoch_core::TransformPlotKind::Null,
    .name = "Text",
    .options =
        {
            MetaDataOption{.id = "value",
                           .name = "",
                           .type = epoch_core::MetaDataOptionType::String},
        },
    .desc = "Outputs a constant text/string value. Useful for injecting fixed "
            "text into a pipeline.",
    .outputs = {IOMetaDataConstants::STRING_OUTPUT_METADATA},
    .tags = {"scalar", "constant", "text", "string"},
    .strategyTypes = {"parameter-injection", "labeling"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Inject constant text values for labels, identifiers, or text-based parameters. Common uses: asset identifiers, category labels, text annotations.",
    .limitations = "Static value only - cannot adapt to market conditions. For dynamic text, use string operations or text indicators."});

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
        .tags = {"scalar", "constant", "boolean"},
        .strategyTypes = {"testing", "placeholder-logic"},
        .assetRequirements = {"single-asset"},
        .usageContext = boolConstant ? "Always-true condition for testing, enabling branches, or placeholder logic." : "Always-false condition for disabling branches, testing, or placeholder logic.",
        .limitations = "Constant value - no dynamic behavior. Mainly for development/testing."});
  }

  for (auto const &[id, name] :
       std::initializer_list<std::array<std::string, 2>>{
           {"null", "Null"},
           {"one", "1"},
           {"negative_one", "-1"},
           {"zero", "0"},
           {"pi", "π"},
           {"e", "e"},
           {"phi", "φ"},
           {"sqrt2", "√2"},
           {"sqrt3", "√3"},
           {"sqrt5", "√5"},
           {"ln2", "ln(2)"},
           {"ln10", "ln(10)"},
           {"log2e", "log2(e)"},
           {"log10e", "log10(e)"}}) {
    metadataList.emplace_back(TransformsMetaData{
        .id = id,
        .category = epoch_core::TransformCategory::Scalar,
        .renderKind = epoch_core::TransformNodeRenderKind::Label,
        .plotKind = epoch_core::TransformPlotKind::Null,
        .name = name,
        .options = {},
        .desc = name,
        .outputs = {id == "null"
                        ? IOMetaDataConstants::ANY_OUTPUT_METADATA
                        : IOMetaDataConstants::DECIMAL_OUTPUT_METADATA},
        .tags = {"scalar", "constant", "math", "number"}});
  }

  return metadataList;
}

std::vector<TransformsMetaData> MakeDataSource() {
  std::vector<TransformsMetaData> result;

  // Refactored Names Applied Below
  result.emplace_back(TransformsMetaData{
      .id = MARKET_DATA_SOURCE_ID,
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
                  IOMetaDataConstants::VOLUME_METADATA,
                  {epoch_core::IODataType::Number, "vw", "Volume Weighted Average Price", true},
                  {epoch_core::IODataType::Number, "n", "Trade Count", true}},
      .tags = {"data", "source", "price", "ohlcv"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"o", "h", "l", "c", "v", "vw", "n"},
      .strategyTypes = {"data-input"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Foundation node providing raw OHLCV market data to all strategies. Every strategy pipeline starts here. Outputs connect to indicators, comparisons, and calculations. VWAP and trade count available for advanced volume analysis.",
      .limitations = "Data quality depends on feed provider. Historical data may have gaps or errors. Intraday data limited by subscription/exchange access."});

  return result;
}

std::vector<TransformsMetaData> MakeTradeSignalExecutor() {
  std::vector<TransformsMetaData> result;

  IOMetaData longMetaData{.type = epoch_core::IODataType::Boolean,
                          .id = "enter_long",
                          .name = "Enter Long"};

  IOMetaData shortMetaData{.type = epoch_core::IODataType::Boolean,
                           .id = "enter_short",
                           .name = "Enter Short"};

  IOMetaData closeLongPositionMetaData{
      .type = epoch_core::IODataType::Boolean,
      .id = "exit_long",
      .name = "Exit Long",
  };

  IOMetaData closeShortPositionMetaData{
      .type = epoch_core::IODataType::Boolean,
      .id = "exit_short",
      .name = "Exit Short",
  };

  // No indecision option; we use a fixed policy documented in the description.

  return {TransformsMetaData{
      .id = TRADE_SIGNAL_EXECUTOR_ID,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .plotKind = epoch_core::TransformPlotKind::trade_signal,
      .name = "Trade Signal Executor",
      .options = {},
      .desc = "Executes trade signals. Precedence: handle exits first ("
              "'Exit Long'/'Exit Short'). For entries, if both 'Enter Long' "
              "and 'Enter Short' are true on the same step, skip opening any "
              "new position. Otherwise, open the requested side.",
      .inputs = {longMetaData, shortMetaData, closeLongPositionMetaData,
                 closeShortPositionMetaData},
      .atLeastOneInputRequired = true,
      .requiresTimeFrame = false,
      .strategyTypes = {"execution", "position-management"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Terminal node that converts boolean signals into trade execution. Connect entry/exit conditions from your strategy logic. Handles position state management - exits before entries, no simultaneous long+short entries. Every backtestable strategy must end with this node.",
      .limitations = "Simple execution only - no position sizing, no risk management, no order types. Assumes immediate fills at close price. Simultaneous long+short entry signals conflict and result in no action (prevents ambiguity)."}};
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

TransformsMetaData MakeCalendarEffectMetaData(
    const std::string &effect_type,
    const std::string &custom_id = "",
    const std::string &custom_name = "") {

  TransformsMetaData metadata;

  if (effect_type == "turn_of_month") {
    metadata.id = custom_id.empty() ? "turn_of_month" : custom_id;
    metadata.name = custom_name.empty() ? "Turn of Month" : custom_name;
    metadata.desc = "Detects the turn-of-month calendar anomaly: marks the last N trading days of the month and the first M trading days of the next month. Research shows statistically significant positive returns during this window.";
    metadata.usageContext = "Implement turn-of-month effect strategies. Research shows SPY returns highest during days -1 to +3 of each month. Use as entry timing filter or position sizing multiplier. Combine with other signals for confirmation.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "timing"};
    metadata.tags = {"calendar", "seasonal", "month", "turn-of-month"};
    metadata.options = {
        MetaDataOption{.id = "days_before",
                       .name = "Days Before Month End",
                       .type = epoch_core::MetaDataOptionType::Integer,
                       .defaultValue = MetaDataOptionDefinition(static_cast<double>(2)),
                       .min = 0,
                       .max = 15,
                       .desc = "Number of trading days before month end to include",
                       .tuningGuidance = "Research suggests 1-2 days before month end. More days may dilute effect."},
        MetaDataOption{.id = "days_after",
                       .name = "Days After Month Start",
                       .type = epoch_core::MetaDataOptionType::Integer,
                       .defaultValue = MetaDataOptionDefinition(static_cast<double>(3)),
                       .min = 0,
                       .max = 15,
                       .desc = "Number of trading days after month start to include",
                       .tuningGuidance = "Research suggests 3-4 days after month start. Test on your specific market."}
    };
  } else if (effect_type == "day_of_week") {
    metadata.id = custom_id.empty() ? "day_of_week" : custom_id;
    metadata.name = custom_name.empty() ? "Day of Week" : custom_name;
    metadata.desc = "Detects specific weekdays for day-of-week calendar effects (Monday effect, Friday effect, etc.). Returns true on the specified weekday.";
    metadata.usageContext = "Implement weekday-based strategies. Monday effect (historically negative), Friday effect (tendency for rallies), etc. Use as entry/exit timing or position sizing filter. Note: many classic effects have weakened over time.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "timing"};
    metadata.tags = {"calendar", "day-of-week", "weekday", "seasonal"};
    metadata.options = {
        MetaDataOption{.id = "weekday",
                       .name = "Weekday",
                       .type = epoch_core::MetaDataOptionType::Select,
                       .defaultValue = MetaDataOptionDefinition(std::string("Monday")),
                       .selectOption = {{"Monday", "Monday"}, {"Tuesday", "Tuesday"}, {"Wednesday", "Wednesday"},
                                       {"Thursday", "Thursday"}, {"Friday", "Friday"}},
                       .desc = "The specific weekday to detect"}
    };
  } else if (effect_type == "month_of_year") {
    metadata.id = custom_id.empty() ? "month_of_year" : custom_id;
    metadata.name = custom_name.empty() ? "Month of Year" : custom_name;
    metadata.desc = "Detects specific months for seasonal patterns (January effect, sell in May, etc.). Returns true during the specified month.";
    metadata.usageContext = "Implement seasonal month effects. January effect (small caps), 'Sell in May and go away' (summer underperformance), Santa Claus rally (December). Use as regime filter or position sizing. Test on your specific market - many effects are weaker than historical data suggests.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "monthly-pattern"};
    metadata.tags = {"calendar", "month", "seasonal", "january-effect"};
    metadata.options = {
        MetaDataOption{.id = "month",
                       .name = "Month",
                       .type = epoch_core::MetaDataOptionType::Select,
                       .defaultValue = MetaDataOptionDefinition(std::string("January")),
                       .selectOption = {{"January", "January"}, {"February", "February"}, {"March", "March"},
                                       {"April", "April"}, {"May", "May"}, {"June", "June"},
                                       {"July", "July"}, {"August", "August"}, {"September", "September"},
                                       {"October", "October"}, {"November", "November"}, {"December", "December"}},
                       .desc = "The specific month to detect"}
    };
  } else if (effect_type == "quarter") {
    metadata.id = custom_id.empty() ? "quarter" : custom_id;
    metadata.name = custom_name.empty() ? "Quarter" : custom_name;
    metadata.desc = "Detects specific quarters for quarterly patterns (Q4 rally, Q1 effect, etc.). Returns true during the specified quarter.";
    metadata.usageContext = "Implement quarterly seasonal patterns. Q4 historically strong (year-end rally), Q1 continuation. Useful for pension fund rebalancing effects, earnings seasonality. Combine with other factors for robustness.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "quarterly-pattern"};
    metadata.tags = {"calendar", "quarter", "seasonal"};
    metadata.options = {
        MetaDataOption{.id = "quarter",
                       .name = "Quarter",
                       .type = epoch_core::MetaDataOptionType::Select,
                       .defaultValue = MetaDataOptionDefinition(std::string("Q1")),
                       .selectOption = {{"Q1", "Q1 (Jan-Mar)"}, {"Q2", "Q2 (Apr-Jun)"},
                                       {"Q3", "Q3 (Jul-Sep)"}, {"Q4", "Q4 (Oct-Dec)"}},
                       .desc = "The specific quarter to detect"}
    };
  } else if (effect_type == "holiday") {
    metadata.id = custom_id.empty() ? "holiday" : custom_id;
    metadata.name = custom_name.empty() ? "Holiday Effect" : custom_name;
    metadata.desc = "Detects days before/after holidays. Pre-holiday and post-holiday effects show tendency for positive returns. Requires country-specific holiday calendar.";
    metadata.usageContext = "Implement holiday effect strategies. Markets tend to rally before holidays (reduced volume, positive sentiment). Use for timing entries/exits around holidays. Effectiveness varies by market and holiday.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "holiday-effect"};
    metadata.tags = {"calendar", "holiday", "seasonal"};
    metadata.options = {
        MetaDataOption{.id = "days_before",
                       .name = "Days Before Holiday",
                       .type = epoch_core::MetaDataOptionType::Integer,
                       .defaultValue = MetaDataOptionDefinition(static_cast<double>(1)),
                       .min = 0,
                       .max = 5,
                       .desc = "Number of trading days before holiday"},
        MetaDataOption{.id = "days_after",
                       .name = "Days After Holiday",
                       .type = epoch_core::MetaDataOptionType::Integer,
                       .defaultValue = MetaDataOptionDefinition(static_cast<double>(0)),
                       .min = 0,
                       .max = 5,
                       .desc = "Number of trading days after holiday"},
        MetaDataOption{.id = "country",
                       .name = "Holiday Calendar",
                       .type = epoch_core::MetaDataOptionType::Select,
                       .defaultValue = MetaDataOptionDefinition(std::string("USFederalHolidayCalendar")),
                       .selectOption = {{"USFederalHolidayCalendar", "US Federal Holidays"}},
                       .desc = "Holiday calendar to use for detecting holidays"}
    };
  } else if (effect_type == "week_of_month") {
    metadata.id = custom_id.empty() ? "week_of_month" : custom_id;
    metadata.name = custom_name.empty() ? "Week of Month" : custom_name;
    metadata.desc = "Detects specific weeks within a month (first week, last week, etc.). Returns true during the specified week of the month.";
    metadata.usageContext = "Implement week-of-month patterns. First week can show momentum continuation from prior month. Last week may show turn-of-month effect buildup. Useful for intramonth timing strategies.";
    metadata.strategyTypes = {"calendar-anomaly", "seasonal", "timing"};
    metadata.tags = {"calendar", "week", "seasonal", "intramonth"};
    metadata.options = {
        MetaDataOption{.id = "week",
                       .name = "Week of Month",
                       .type = epoch_core::MetaDataOptionType::Select,
                       .defaultValue = MetaDataOptionDefinition(std::string("First")),
                       .selectOption = {{"First", "First Week"}, {"Second", "Second Week"},
                                       {"Third", "Third Week"}, {"Fourth", "Fourth Week"},
                                       {"Last", "Last Week"}},
                       .desc = "Which week of the month to detect"}
    };
  }

  // Common metadata for all calendar effects
  metadata.category = epoch_core::TransformCategory::Statistical;
  metadata.renderKind = epoch_core::TransformNodeRenderKind::Standard;
  metadata.plotKind = epoch_core::TransformPlotKind::Null;
  metadata.isCrossSectional = false;
  metadata.requiresTimeFrame = true;
  metadata.assetRequirements = {"single-asset"};
  metadata.limitations = "Calendar effects have weakened over time as they became widely known. Backtest thoroughly and use recent data. Transaction costs may eliminate edge. Combine with other signals for robustness.";

  // All calendar effects output boolean
  metadata.inputs = {};  // No inputs - operates on index timestamps
  metadata.outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA};

  return metadata;
}

std::vector<TransformsMetaData> MakeCalendarEffectMetaData() {
  std::vector<TransformsMetaData> metadataList;

  // Generate metadata for each calendar effect type
  metadataList.emplace_back(MakeCalendarEffectMetaData("turn_of_month"));
  metadataList.emplace_back(MakeCalendarEffectMetaData("day_of_week"));
  metadataList.emplace_back(MakeCalendarEffectMetaData("month_of_year"));
  metadataList.emplace_back(MakeCalendarEffectMetaData("quarter"));
  metadataList.emplace_back(MakeCalendarEffectMetaData("holiday"));
  metadataList.emplace_back(MakeCalendarEffectMetaData("week_of_month"));

  return metadataList;
}

std::vector<TransformsMetaData> MakeChartFormationMetaData() {
  std::vector<TransformsMetaData> metadataList;

  // FlexiblePivotDetector - Infrastructure for pivot detection
  metadataList.emplace_back(TransformsMetaData{
      .id = "flexible_pivot_detector",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Pivot Point Detector",
      .options = {
          MetaDataOption{.id = "left_count",
                         .name = "Left Lookback Bars",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(5)),
                         .min = 1,
                         .max = 50,
                         .desc = "Number of bars to check before the pivot",
                         .tuningGuidance = "Lower values (2-5) detect more pivots with more noise. Higher values (10-20) detect only significant pivots but may lag."},
          MetaDataOption{.id = "right_count",
                         .name = "Right Lookback Bars",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(5)),
                         .min = 1,
                         .max = 50,
                         .desc = "Number of bars to check after the pivot",
                         .tuningGuidance = "Symmetric with left_count detects centered pivots. Asymmetric allows early detection (smaller right_count) or confirmation (larger right_count)."}
      },
      .desc = "Detects pivot points (local highs and lows) in price data with configurable asymmetric lookback. Foundation for chart pattern detection.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Integer, "pivot_type", "Pivot Type (0=none, 1=low, 2=high, 3=both)"},
          {epoch_core::IODataType::Number, "pivot_level", "Pivot Price Level"},
          {epoch_core::IODataType::Integer, "pivot_index", "Pivot Bar Index"}
      },
      .tags = {"pivot", "swing-points", "pattern-detection", "price-action"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"pattern-detection", "support-resistance"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Foundation transform for detecting swing highs/lows. Use pivots to identify support/resistance or feed into pattern detectors (head-shoulders, triangles, etc.). Higher lookback = fewer, more significant pivots.",
      .limitations = "Requires right_count bars to confirm pivot, causing detection lag. Choppy markets produce many false pivots. No volume or volatility weighting."});

  // HeadAndShoulders - Bearish reversal pattern
  metadataList.emplace_back(TransformsMetaData{
      .id = "head_and_shoulders",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Head and Shoulders",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(50)),
                         .min = 20,
                         .max = 200,
                         .desc = "Number of bars to search for pattern formation",
                         .tuningGuidance = "30-50 for intraday, 50-100 for daily charts. Longer lookback detects larger patterns but increases lag."},
          MetaDataOption{.id = "head_ratio_before",
                         .name = "Head Height Ratio (Before)",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1.0002),
                         .min = 1.0001,
                         .max = 1.1,
                         .desc = "Minimum ratio of head to left shoulder height",
                         .tuningGuidance = "1.0002 means head must be 0.02% higher than left shoulder. Higher values require more pronounced head."},
          MetaDataOption{.id = "head_ratio_after",
                         .name = "Head Height Ratio (After)",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1.0002),
                         .min = 1.0001,
                         .max = 1.1,
                         .desc = "Minimum ratio of head to right shoulder height",
                         .tuningGuidance = "1.0002 means head must be 0.02% higher than right shoulder. Higher values require more pronounced head."},
          MetaDataOption{.id = "neckline_slope_max",
                         .name = "Maximum Neckline Slope",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1e-4),
                         .min = 1e-5,
                         .max = 0.01,
                         .desc = "Maximum allowed slope for neckline (nearly horizontal)",
                         .tuningGuidance = "1e-4 requires nearly flat neckline. Increase for sloped necklines, decrease for strictly horizontal."}
      },
      .desc = "Detects bearish head-and-shoulders reversal pattern: left shoulder, higher head, right shoulder at similar level to left, with neckline support.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "pattern_detected", "Pattern Detected"},
          {epoch_core::IODataType::Number, "neckline_level", "Neckline Support Level"},
          {epoch_core::IODataType::Number, "target", "Breakout Target Price"}
      },
      .tags = {"reversal", "bearish", "head-and-shoulders", "chart-pattern", "topping-pattern"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"reversal-trading", "pattern-recognition", "top-detection"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Classic topping pattern signaling trend reversal. Wait for neckline break confirmation before entering short. Target = neckline - (head - neckline). Combine with volume analysis - volume should decrease at right shoulder.",
      .limitations = "Subjective pattern - detection may differ from manual charting. Many false signals in choppy markets. Neckline break required for confirmation. Time to complete pattern can be long."});

  // InverseHeadAndShoulders - Bullish reversal pattern
  metadataList.emplace_back(TransformsMetaData{
      .id = "inverse_head_and_shoulders",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Inverse Head and Shoulders",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(50)),
                         .min = 20,
                         .max = 200,
                         .desc = "Number of bars to search for pattern formation",
                         .tuningGuidance = "30-50 for intraday, 50-100 for daily charts. Longer lookback detects larger patterns but increases lag."},
          MetaDataOption{.id = "head_ratio_before",
                         .name = "Head Depth Ratio (Before)",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1.0002),
                         .min = 1.0001,
                         .max = 1.1,
                         .desc = "Minimum ratio of head to left shoulder depth (inverted pattern)",
                         .tuningGuidance = "1.0002 means head must be 0.02% lower than left shoulder. Higher values require more pronounced head."},
          MetaDataOption{.id = "head_ratio_after",
                         .name = "Head Depth Ratio (After)",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1.0002),
                         .min = 1.0001,
                         .max = 1.1,
                         .desc = "Minimum ratio of head to right shoulder depth (inverted pattern)",
                         .tuningGuidance = "1.0002 means head must be 0.02% lower than right shoulder. Higher values require more pronounced head."},
          MetaDataOption{.id = "neckline_slope_max",
                         .name = "Maximum Neckline Slope",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(1e-4),
                         .min = 1e-5,
                         .max = 0.01,
                         .desc = "Maximum allowed slope for neckline (nearly horizontal)",
                         .tuningGuidance = "1e-4 requires nearly flat neckline. Increase for sloped necklines, decrease for strictly horizontal."}
      },
      .desc = "Detects bullish inverse head-and-shoulders reversal pattern: left shoulder low, lower head, right shoulder at similar level to left, with neckline resistance.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "pattern_detected", "Pattern Detected"},
          {epoch_core::IODataType::Number, "neckline_level", "Neckline Resistance Level"},
          {epoch_core::IODataType::Number, "target", "Breakout Target Price"}
      },
      .tags = {"reversal", "bullish", "inverse-head-and-shoulders", "chart-pattern", "bottoming-pattern"},
      .requiresTimeFrame = true,
      .strategyTypes = {"reversal-trading", "pattern-recognition", "bottom-detection"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Classic bottoming pattern signaling uptrend reversal. Wait for neckline breakout confirmation before entering long. Target = neckline + (neckline - head). Volume should increase on neckline breakout.",
      .limitations = "Subjective pattern - detection may differ from manual charting. Many false signals in choppy markets. Neckline break required for confirmation. Pattern completion can take significant time."});

  // DoubleTopBottom - Double top/bottom reversal patterns
  metadataList.emplace_back(TransformsMetaData{
      .id = "double_top_bottom",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Double Top/Bottom",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(20)),
                         .min = 10,
                         .max = 100,
                         .desc = "Number of bars to search for pattern",
                         .tuningGuidance = "20-30 for shorter-term patterns, 50-100 for major reversal patterns."},
          MetaDataOption{.id = "pattern_type",
                         .name = "Pattern Type",
                         .type = epoch_core::MetaDataOptionType::Select,
                         .defaultValue = MetaDataOptionDefinition(std::string("both")),
                         .selectOption = {{"tops", "Double Top Only"}, {"bottoms", "Double Bottom Only"}, {"both", "Both Patterns"}},
                         .desc = "Which pattern type to detect"},
          MetaDataOption{.id = "similarity_tolerance",
                         .name = "Peak/Trough Similarity Tolerance",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.015),
                         .min = 0.005,
                         .max = 0.05,
                         .desc = "Maximum price difference between peaks/troughs as ratio",
                         .tuningGuidance = "0.01-0.015 for strict patterns. Higher values (0.02-0.03) allow more variation but increase false positives."}
      },
      .desc = "Detects double top (bearish) and double bottom (bullish) reversal patterns. Two peaks/troughs at similar levels with intervening trough/peak.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "pattern_detected", "Pattern Detected"},
          {epoch_core::IODataType::Number, "breakout_level", "Breakout/Breakdown Level"},
          {epoch_core::IODataType::Number, "target", "Price Target"}
      },
      .tags = {"reversal", "double-top", "double-bottom", "chart-pattern", "M-pattern", "W-pattern"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"reversal-trading", "pattern-recognition", "top-bottom-detection"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Double top = bearish M pattern at resistance. Double bottom = bullish W pattern at support. Target = breakout level +/- (peak - trough). Wait for breakout confirmation. Volume typically lighter on 2nd peak/trough.",
      .limitations = "Requires similar peak/trough heights - tolerance parameter critical. False signals common without confirmation. Time between peaks/troughs varies widely. Pattern incomplete until breakout."});

  // Flag - Bull/bear flag continuation patterns
  metadataList.emplace_back(TransformsMetaData{
      .id = "flag",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Flag Pattern",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(30)),
                         .min = 10,
                         .max = 100,
                         .desc = "Number of bars to search for consolidation",
                         .tuningGuidance = "20-30 for typical flags. Longer periods may detect larger patterns but flag should be relatively brief."},
          MetaDataOption{.id = "min_pivot_points",
                         .name = "Minimum Pivot Points",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(4)),
                         .min = 3,
                         .max = 10,
                         .desc = "Minimum pivots for each trendline",
                         .tuningGuidance = "3-4 for early detection. 5-6 for higher confidence. More pivots = stricter pattern but slower detection."},
          MetaDataOption{.id = "r_squared_min",
                         .name = "Minimum R-Squared",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.7),
                         .min = 0.5,
                         .max = 0.99,
                         .desc = "Minimum R-squared for trendline fit quality",
                         .tuningGuidance = "0.7-0.8 balanced. Higher (0.85-0.9) for cleaner patterns but fewer detections. Lower (0.6-0.7) more detections but noisier."},
          MetaDataOption{.id = "slope_parallel_tolerance",
                         .name = "Parallel Tolerance",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.2),
                         .min = 0.05,
                         .max = 0.5,
                         .desc = "Tolerance for parallel trendlines (0.2 = 20% difference)",
                         .tuningGuidance = "0.15-0.25 typical. Stricter (0.1) requires very parallel lines. Looser (0.3-0.4) allows more channel variation."}
      },
      .desc = "Detects bull and bear flag continuation patterns. Bull flag: uptrend + downward-sloping consolidation. Bear flag: downtrend + upward-sloping consolidation.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "bull_flag", "Bull Flag Detected"},
          {epoch_core::IODataType::Boolean, "bear_flag", "Bear Flag Detected"},
          {epoch_core::IODataType::Number, "slmax", "Upper Trendline Slope"},
          {epoch_core::IODataType::Number, "slmin", "Lower Trendline Slope"}
      },
      .tags = {"continuation", "flag", "bull-flag", "bear-flag", "chart-pattern", "consolidation"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"trend-continuation", "breakout-trading", "pattern-recognition"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Flags are brief consolidations within strong trends. Bull flag counter-trend consolidation in uptrend. Bear flag counter-trend bounce in downtrend. Target = flagpole height projected from breakout. Volume should contract during flag, expand on breakout.",
      .limitations = "Requires preceding strong move (flagpole) which is not explicitly validated. Flag duration should be brief - long consolidations may be different pattern. Parallel trendlines requirement may miss valid but imperfect flags."});

  // Triangles - Ascending/descending/symmetrical triangles
  metadataList.emplace_back(TransformsMetaData{
      .id = "triangles",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Triangle Patterns",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(50)),
                         .min = 20,
                         .max = 200,
                         .desc = "Number of bars to search for triangle formation",
                         .tuningGuidance = "40-60 for typical triangles. Larger patterns need longer lookback (100+). Shorter lookback (20-30) for intraday."},
          MetaDataOption{.id = "triangle_type",
                         .name = "Triangle Type",
                         .type = epoch_core::MetaDataOptionType::Select,
                         .defaultValue = MetaDataOptionDefinition(std::string("all")),
                         .selectOption = {{"ascending", "Ascending (Bullish)"}, {"descending", "Descending (Bearish)"}, {"symmetrical", "Symmetrical (Neutral)"}, {"all", "All Types"}},
                         .desc = "Which triangle pattern type to detect"},
          MetaDataOption{.id = "r_squared_min",
                         .name = "Minimum R-Squared",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.8),
                         .min = 0.5,
                         .max = 0.99,
                         .desc = "Minimum R-squared for trendline quality",
                         .tuningGuidance = "0.75-0.85 typical for triangles (higher than flags due to longer formation). Lower values increase detections but reduce quality."}
      },
      .desc = "Detects triangle consolidation patterns. Ascending: flat resistance + rising support. Descending: falling resistance + flat support. Symmetrical: converging trendlines.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "pattern_detected", "Pattern Detected"},
          {epoch_core::IODataType::Number, "upper_slope", "Upper Trendline Slope"},
          {epoch_core::IODataType::Number, "lower_slope", "Lower Trendline Slope"},
          {epoch_core::IODataType::String, "triangle_type", "Detected Triangle Type"}
      },
      .tags = {"consolidation", "triangle", "ascending-triangle", "descending-triangle", "symmetrical-triangle", "chart-pattern"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"breakout-trading", "consolidation-patterns", "pattern-recognition"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Triangles are consolidation patterns preceding breakouts. Ascending (bullish bias): flat top, rising lows. Descending (bearish bias): falling highs, flat bottom. Symmetrical (neutral): converging highs/lows. Trade breakout direction. Volume contracts during formation, expands on breakout.",
      .limitations = "Direction uncertain until breakout (especially symmetrical). False breakouts common - wait for confirmation. Pattern can fail if price doesn't breakout before apex. Slope thresholds (0.0001) may need adjustment for different price scales."});

  // Pennant - Short-term continuation pattern
  metadataList.emplace_back(TransformsMetaData{
      .id = "pennant",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Pennant Pattern",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(20)),
                         .min = 10,
                         .max = 50,
                         .desc = "Number of bars to search for pennant",
                         .tuningGuidance = "15-25 typical. Pennants are brief consolidations. Longer lookback may confuse with triangles."},
          MetaDataOption{.id = "min_pivot_points",
                         .name = "Minimum Pivot Points",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(3)),
                         .min = 2,
                         .max = 6,
                         .desc = "Minimum pivots for each trendline",
                         .tuningGuidance = "3 minimum for pennant. 4 for higher confidence. Pennants form quickly so fewer pivots than triangles."},
          MetaDataOption{.id = "r_squared_min",
                         .name = "Minimum R-Squared",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.7),
                         .min = 0.5,
                         .max = 0.99,
                         .desc = "Minimum R-squared for trendline quality",
                         .tuningGuidance = "0.65-0.75 typical for pennants (slightly lower than triangles due to brief formation)."},
          MetaDataOption{.id = "max_duration",
                         .name = "Maximum Duration",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(15)),
                         .min = 5,
                         .max = 30,
                         .desc = "Maximum bars for pennant formation",
                         .tuningGuidance = "10-20 bars typical. Pennants are brief. Longer consolidations are likely triangles or flags."}
      },
      .desc = "Detects pennant continuation patterns - brief consolidations with converging trendlines following strong moves. Similar to symmetrical triangles but shorter duration.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "bull_pennant", "Bull Pennant Detected"},
          {epoch_core::IODataType::Boolean, "bear_pennant", "Bear Pennant Detected"},
          {epoch_core::IODataType::Number, "slmax", "Upper Trendline Slope"},
          {epoch_core::IODataType::Number, "slmin", "Lower Trendline Slope"}
      },
      .tags = {"continuation", "pennant", "consolidation", "chart-pattern", "brief-consolidation"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"h", "l"},
      .strategyTypes = {"trend-continuation", "breakout-trading", "pattern-recognition"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Pennants are very brief consolidations in strong trends. Converging trendlines form symmetrical triangle shape. Breakout typically in direction of preceding trend (flagpole). Best traded near apex. Volume contracts during formation, expands on breakout.",
      .limitations = "Current implementation assumes bullish for simplicity - proper version needs preceding trend analysis. Very brief formation makes detection challenging. Requires converging lines which may miss valid pennants. Max_duration parameter critical to distinguish from triangles."});

  // SessionTimeWindow - Detect proximity to session boundaries
  metadataList.emplace_back(TransformsMetaData{
      .id = "session_time_window",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Session Time Window",
      .options = {
          MetaDataOption{.id = "session_type",
                         .name = "Session Type",
                         .type = epoch_core::MetaDataOptionType::Select,
                         .defaultValue = MetaDataOptionDefinition(std::string("London")),
                         .selectOption = {{"Sydney", "Sydney Session"}, {"Tokyo", "Tokyo Session"}, {"London", "London Session"}, {"NewYork", "New York Session"}, {"AsianKillZone", "Asian Kill Zone"}, {"LondonOpenKillZone", "London Open Kill Zone"}, {"NewYorkKillZone", "New York Kill Zone"}, {"LondonCloseKillZone", "London Close Kill Zone"}},
                         .desc = "Trading session or kill zone to track"},
          MetaDataOption{.id = "minute_offset",
                         .name = "Minute Offset",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(15)),
                         .min = 0,
                         .max = 360,
                         .desc = "Minutes from session boundary"},
          MetaDataOption{.id = "boundary_type",
                         .name = "Boundary Type",
                         .type = epoch_core::MetaDataOptionType::String,
                         .defaultValue = MetaDataOptionDefinition(std::string("start")),
                         .desc = "Session boundary: 'start' or 'end'"}
      },
      .desc = "Detects when bars occur exactly X minutes from session start or end. Useful for timing entries/exits around session boundaries.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "in_window", "In Time Window"}
      },
      .tags = {"session", "time", "timing", "smc", "session-boundary"},
      .requiresTimeFrame = true,
      .intradayOnly = true,
      .strategyTypes = {"session-timing", "intraday-timing", "time-based-entry"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Filter trades to specific times relative to session boundaries. Use for opening range breakouts (e.g., 15 minutes from session start) or pre-close strategies (e.g., 30 minutes before session end). Combine with other signals for time-based entry/exit.",
      .limitations = "Only detects exact timestamp matches - requires bars at precise offset. Session times may vary by market and daylight saving time. Intraday data required."});

  // ConsolidationBox - Horizontal rectangle pattern (Bulkowski)
  metadataList.emplace_back(TransformsMetaData{
      .id = "consolidation_box",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Consolidation Box",
      .options = {
          MetaDataOption{.id = "lookback",
                         .name = "Lookback Period",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(40)),
                         .min = 20,
                         .max = 150,
                         .desc = "Number of bars to search for consolidation box",
                         .tuningGuidance = "30-50 for typical boxes on intraday. 60-100 for daily/longer timeframes. Consolidation should span multiple swings."},
          MetaDataOption{.id = "min_pivot_points",
                         .name = "Minimum Pivot Points",
                         .type = epoch_core::MetaDataOptionType::Integer,
                         .defaultValue = MetaDataOptionDefinition(static_cast<double>(5)),
                         .min = 4,
                         .max = 12,
                         .desc = "Minimum total touches across both boundaries (Bulkowski: 5 minimum)",
                         .tuningGuidance = "5 per Bulkowski (3 on one line, 2 on other). Higher values (6-8) require more confirmation but reduce false positives."},
          MetaDataOption{.id = "r_squared_min",
                         .name = "Minimum R-Squared",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.75),
                         .min = 0.6,
                         .max = 0.95,
                         .desc = "Minimum R-squared for horizontal line fit quality",
                         .tuningGuidance = "0.75-0.85 typical. Lower values allow rougher boxes. Higher values require cleaner consolidation but may miss valid patterns."},
          MetaDataOption{.id = "max_slope",
                         .name = "Maximum Slope (Horizontal Threshold)",
                         .type = epoch_core::MetaDataOptionType::Decimal,
                         .defaultValue = MetaDataOptionDefinition(0.0001),
                         .min = 0.00001,
                         .max = 0.001,
                         .desc = "Maximum allowed slope for boundaries (nearly horizontal)",
                         .tuningGuidance = "0.0001 requires very flat boundaries. Increase for slightly sloped rectangles. Price scale dependent - adjust for Bitcoin vs stocks."}
      },
      .desc = "Detects horizontal consolidation boxes (rectangles) based on Bulkowski's criteria: parallel horizontal support/resistance with minimum 5 touches. Classic range-bound pattern preceding breakouts.",
      .inputs = {},
      .outputs = {
          {epoch_core::IODataType::Boolean, "box_detected", "Box Pattern Detected"},
          {epoch_core::IODataType::Number, "box_top", "Upper Boundary (Resistance)"},
          {epoch_core::IODataType::Number, "box_bottom", "Lower Boundary (Support)"},
          {epoch_core::IODataType::Number, "box_height", "Box Height"},
          {epoch_core::IODataType::Integer, "touch_count", "Total Touches"},
          {epoch_core::IODataType::Number, "upper_slope", "Upper Boundary Slope (should be ~0)"},
          {epoch_core::IODataType::Number, "lower_slope", "Lower Boundary Slope (should be ~0)"},
          {epoch_core::IODataType::Number, "target_up", "Upside Breakout Target"},
          {epoch_core::IODataType::Number, "target_down", "Downside Breakdown Target"}
      },
      .tags = {"consolidation", "range", "rectangle", "horizontal", "chart-pattern", "bulkowski", "support-resistance"},
      .requiresTimeFrame = true,
      .strategyTypes = {"range-trading", "breakout-trading", "mean-reversion", "fade-strategy", "pattern-recognition"},
      .assetRequirements = {"single-asset"},
      .usageContext = "Consolidation boxes are horizontal ranges with clear support/resistance. Trade strategies: (1) Fade edges - sell resistance, buy support with tight stops. (2) Breakout - enter on confirmed break above/below box with target = box_height. Volume typically declines during consolidation, spikes on breakout. Bulkowski stats: Rectangle Top breaks up 63%, Rectangle Bottom breaks down 63%.",
      .limitations = "Requires clear horizontal boundaries - slope threshold critical. Box detection lags until pattern complete. Direction uncertainty until breakout. False breakouts common - use confirmation (volume, follow-through). Max_slope may need adjustment for different price scales/assets. Does not validate preceding trend like Bulkowski's manual analysis."});

  return metadataList;
}

} // namespace epoch_metadata::transforms