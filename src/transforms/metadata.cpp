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

} // namespace epoch_metadata::transforms