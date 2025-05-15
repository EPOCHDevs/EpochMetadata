#pragma once
#include "epoch_metadata/constants.h"
#include "epoch_metadata/metadata_options.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

CREATE_ENUM(TransformType, Overlay, Indicator, Simple, MathFunction, DataSource,
            TradeSignalExecutor, MathOperator, ControlFlow, CandleStickPattern,
            CrossSectional, Scalar, Aggregate);
CREATE_ENUM(IODataType, Decimal, Integer, Number, Boolean, String, Any);

namespace epoch_metadata::transforms {
struct IOMetaData {
  epoch_core::IODataType type{epoch_core::IODataType::Decimal};
  std::string id{};
  std::string name{};
  bool allowMultipleConnections{false};

  void decode(YAML::Node const &);
  YAML::Node encode() const { return {}; }
};

struct TransformsMetaData {
  std::string id;
  std::string name{};
  MetaDataOptionList options{};
  epoch_core::TransformType type;
  bool isCrossSectional{false};
  std::string desc{};
  std::vector<IOMetaData> inputs{};
  std::vector<IOMetaData> outputs{};
  bool atLeastOneInputRequired{false};
  std::vector<std::string> tags{};
  bool requiresTimeFrame{false};

  void decode(YAML::Node const &);
  YAML::Node encode() const { return {}; }
};

using TransformsMetaDataCreator =
    std::function<TransformsMetaData(std::string const &name)>;

struct IOMetaDataConstants {
  // TODO: Move bar attributes to shared headers
  inline static IOMetaData CLOSE_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                "c", "Close Price", false};
  inline static IOMetaData OPEN_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                               "o", "Open Price", false};
  inline static IOMetaData HIGH_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                               "h", "High Price", false};
  inline static IOMetaData LOW_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                              "l", "Low Price", false};
  inline static IOMetaData VOLUME_METADATA{epoch_core::IODataType::Decimal, "v",
                                           "Volume", false};
  inline static IOMetaData CONTRACT_METADATA{epoch_core::IODataType::String,
                                             "s", "Contract", false};

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
      {"NUMBER_RESULT", NUMBER_OUTPUT_METADATA},
      {"ANY_RESULT", ANY_OUTPUT_METADATA},
      {"BOOLEAN", BOOLEAN_INPUT_METADATA},
      {"BOOLEAN_RESULT", BOOLEAN_OUTPUT_METADATA}};
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
