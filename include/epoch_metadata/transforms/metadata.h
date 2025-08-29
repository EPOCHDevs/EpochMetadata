#pragma once
#include "epoch_metadata/constants.h"
#include "epoch_metadata/metadata_options.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

// Semantic search / palette bucket
CREATE_ENUM(TransformCategory,
            Aggregate,   // aggregate nodes
            ControlFlow, // control flow nodes
            Scalar,      // constants, booleans, editable numbers
            DataSource,  // OHLCV & fundamental feeds
            Math,        // element-wise math & stat functions
            Trend,       // moving-average style trend tools
            Momentum,    // RSI, MACD, Stoch, etc.
            Volatility,  // ATR, Parkinson, Yang-Zhang …
            Volume,      // OBV, VWAP, volume indexes
            PriceAction, // candlestick & chart patterns
            Statistical, // z-score, regression, percentiles
            Factor,      // cross-sectional ranks & spreads
            Utility,     // switches, selectors, helpers
            Executor);   // trade / order sink nodes

// How the block looks in the blueprint
CREATE_ENUM(TransformNodeRenderKind,
            Input,       // data feeds (outputs only)
            Output,      // trade / log sinks
            Label,       // read-only scalar / text
            NumberInput, // editable scalar value
            Operator,    // +  −  ×  ÷ glyph node
            Simple,      // just name
            Standard);   // header, options, side handles

// Chart helper (omit / null ⇒ not plotted)
CREATE_ENUM(
    TransformPlotKind,
    ao,                 // Awesome Oscillator
    aroon,              // Aroon indicator
    bbands,             // Bollinger Bands helper
    bb_percent_b,       // Bollinger Bands %B
    column,             // column plot
    cci,                // Commodity Channel Index
    chande_kroll_stop,  // Chande Kroll Stop
    elders,             // Elder Ray Index
    fisher,             // Fisher Transform,
    fosc,               // Forcast Oscillator
    h_line,             // horizontal line
    ichimoku,           // Ichimoku Cloud
    line,               // generic overlay
    gap,                // Gap indicator
    panel_line,         // generic overlay, but not on top of the main plot,
    panel_line_percent, // generic overlay, but not on top of the main plot,
    qstick,             // Qstick indicator
    qqe,                // QQE indicator
    order_blocks,       // Order Blocks
    flag,               // flag helper
    macd,               // MACD (histogram + signal)
    retracements,       // Retracement lines
    sessions,           // Sessions
    rsi,                // RSI panel
    psar,               // Parabolic-SAR dots
    atr,                // Average True Range
    shl,                // Swing Highs and Lows
    bos_choch,          // Break of Structure and Change of Character
    fvg,                // Fair Value Gap
    liquidity,          // Liquidity
    stoch,              // Stochastic oscillator
    previous_high_low,  // Previous High and Low
    pivot_point_sr,     // Pivot Point Support/Resistance
    vwap,               // VWAP overlay
    vortex,             // Vortex Indicator
    trade_signal);      // Trade Signal Executor

CREATE_ENUM(IODataType, Decimal, Integer, Number, Boolean, String, Any, List,
            Struct);

namespace epoch_metadata::transforms {
constexpr auto MARKET_DATA_SOURCE_ID = "market_data_source";
constexpr auto TRADE_SIGNAL_EXECUTOR_ID = "trade_signal_executor";
struct IOMetaData {
  epoch_core::IODataType type{epoch_core::IODataType::Decimal};
  std::string id{};
  std::string name{};
  bool allowMultipleConnections{false};
  bool isFilter{false};

  void decode(YAML::Node const &);
  YAML::Node encode() const { return {}; }
};

struct TransformCategoryMetaData {
  epoch_core::TransformCategory category;
  std::string name;
  std::string desc;
};
std::vector<TransformCategoryMetaData> MakeTransformCategoryMetaData();

struct TransformsMetaData {
  std::string id;
  epoch_core::TransformCategory category;
  epoch_core::TransformNodeRenderKind renderKind;
  epoch_core::TransformPlotKind plotKind{epoch_core::TransformPlotKind::Null};
  std::string name{};
  MetaDataOptionList options{};
  bool isCrossSectional{false};
  std::string desc{};
  std::vector<IOMetaData> inputs{};
  std::vector<IOMetaData> outputs{};
  bool atLeastOneInputRequired{false};
  std::vector<std::string> tags{};
  bool requiresTimeFrame{false};
  std::vector<std::string> requiredDataSources{};

  void decode(YAML::Node const &);
  YAML::Node encode() const { return {}; }
};

using TransformsMetaDataCreator =
    std::function<TransformsMetaData(std::string const &name)>;

struct IOMetaDataConstants {
  // TODO: Move bar attributes to shared headers
  inline static IOMetaData CLOSE_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                "c", "Close Price", true};
  inline static IOMetaData OPEN_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                               "o", "Open Price", true};
  inline static IOMetaData HIGH_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                               "h", "High Price", true};
  inline static IOMetaData LOW_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                              "l", "Low Price", true};
  inline static IOMetaData VOLUME_METADATA{epoch_core::IODataType::Decimal, "v",
                                           "Volume", true};
  inline static IOMetaData CONTRACT_METADATA{epoch_core::IODataType::String,
                                             "s", "Contract", true};

  inline static IOMetaData ANY_INPUT_METADATA{epoch_core::IODataType::Any, ARG,
                                              "", false};

  inline static IOMetaData ANY_INPUT0_METADATA{epoch_core::IODataType::Any,
                                               ARG0, "", false};

  inline static IOMetaData ANY_INPUT1_METADATA{epoch_core::IODataType::Any,
                                               ARG1, "", false};

  inline static IOMetaData ANY_INPUT2_METADATA{epoch_core::IODataType::Any,
                                               ARG2, "", false};

  inline static IOMetaData DECIMAL_INPUT_METADATA{
      epoch_core::IODataType::Decimal, ARG, "", false};

  inline static IOMetaData DECIMAL_INPUT0_METADATA{
      epoch_core::IODataType::Decimal, ARG0, "", false};

  inline static IOMetaData DECIMAL_INPUT1_METADATA{
      epoch_core::IODataType::Decimal, ARG1, "", false};

  inline static IOMetaData DECIMAL_INPUT2_METADATA{
      epoch_core::IODataType::Decimal, ARG2, "", false};

  inline static IOMetaData NUMBER_INPUT_METADATA{epoch_core::IODataType::Number,
                                                 ARG, "", false};

  inline static IOMetaData NUMBER_INPUT0_METADATA{
      epoch_core::IODataType::Number, ARG0, "", false};

  inline static IOMetaData NUMBER_INPUT1_METADATA{
      epoch_core::IODataType::Number, ARG1, "", false};

  inline static IOMetaData NUMBER_INPUT2_METADATA{
      epoch_core::IODataType::Number, ARG2, "", false};

  inline static IOMetaData ANY_OUTPUT_METADATA{epoch_core::IODataType::Any,
                                               "result", "", true};

  inline static IOMetaData BOOLEAN_INPUT_METADATA{
      epoch_core::IODataType::Boolean, ARG, "", false};

  inline static IOMetaData BOOLEAN_INPUT0_METADATA{
      epoch_core::IODataType::Boolean, ARG0, "", false};

  inline static IOMetaData BOOLEAN_INPUT1_METADATA{
      epoch_core::IODataType::Boolean, ARG1, "", false};

  inline static IOMetaData BOOLEAN_INPUT2_METADATA{
      epoch_core::IODataType::Boolean, ARG2, "", false};

  inline static IOMetaData DECIMAL_OUTPUT_METADATA{
      epoch_core::IODataType::Decimal, "result", "", true};

  inline static IOMetaData BOOLEAN_OUTPUT_METADATA{
      epoch_core::IODataType::Boolean, "result", "", true};

  inline static IOMetaData NUMBER_OUTPUT_METADATA{
      epoch_core::IODataType::Number, "result", "", true};

  inline static IOMetaData INTEGER_OUTPUT_METADATA{
      epoch_core::IODataType::Integer, "result", "", true};

  inline static IOMetaData LIST_INPUT_METADATA{epoch_core::IODataType::List,
                                               ARG, "", false};

  inline static IOMetaData LIST_OUTPUT_METADATA{epoch_core::IODataType::List,
                                                "result", "", true};

  inline static IOMetaData STRUCT_INPUT_METADATA{epoch_core::IODataType::Struct,
                                                 ARG, "", false};

  inline static IOMetaData STRUCT_OUTPUT_METADATA{
      epoch_core::IODataType::Struct, "result", "", true};

  inline static std::unordered_map<std::string, IOMetaData> MAP{
      {"CLOSE", CLOSE_PRICE_METADATA},
      {"OPEN", OPEN_PRICE_METADATA},
      {"HIGH", HIGH_PRICE_METADATA},
      {"LOW", LOW_PRICE_METADATA},
      {"VOLUME", VOLUME_METADATA},
      {"CONTRACT", CONTRACT_METADATA},
      {"DECIMAL", DECIMAL_INPUT_METADATA},
      {"NUMBER", NUMBER_INPUT_METADATA},
      {"ANY", ANY_INPUT_METADATA},
      {"DECIMAL_RESULT", DECIMAL_OUTPUT_METADATA},
      {"INTEGER_RESULT", INTEGER_OUTPUT_METADATA},
      {"NUMBER_RESULT", NUMBER_OUTPUT_METADATA},
      {"ANY_RESULT", ANY_OUTPUT_METADATA},
      {"BOOLEAN", BOOLEAN_INPUT_METADATA},
      {"BOOLEAN_RESULT", BOOLEAN_OUTPUT_METADATA},
      {"LIST", LIST_INPUT_METADATA},
      {"LIST_RESULT", LIST_OUTPUT_METADATA},
      {"STRUCT", STRUCT_INPUT_METADATA},
      {"STRUCT_RESULT", STRUCT_OUTPUT_METADATA}};
};

std::vector<TransformsMetaData> MakeComparativeMetaData();
std::vector<TransformsMetaData> MakeScalarMetaData();
std::vector<TransformsMetaData> MakeDataSource();
std::vector<TransformsMetaData> MakeTradeSignalExecutor();
std::vector<TransformsMetaData> MakeTulipIndicators();
std::vector<TransformsMetaData> MakeTulipCandles();
} // namespace epoch_metadata::transforms

namespace YAML {
template <> struct convert<epoch_metadata::transforms::IOMetaData> {
  static bool decode(const Node &node,
                     epoch_metadata::transforms::IOMetaData &t) {
    t.decode(node);
    return true;
  }
};

template <> struct convert<epoch_metadata::transforms::TransformsMetaData> {
  static bool decode(const Node &node,
                     epoch_metadata::transforms::TransformsMetaData &t) {
    t.decode(node);
    return true;
  }
};
} // namespace YAML
