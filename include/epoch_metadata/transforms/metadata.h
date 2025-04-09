#pragma once
#include "epoch_metadata/metadata_options.h"
#include <glaze/glaze.hpp>
#include <string>
#include <vector>
#include "epoch_metadata/constants.h"

CREATE_ENUM(TransformType, Overlay, Indicator, Math, DataSource,
            TradeSignalExecutor, Comparative, CandleStickPattern);
CREATE_ENUM(IODataType, Decimal, Integer, Number, Boolean, String, Any);

namespace metadata::transforms {
  struct IOMetaData {
    epoch_core::IODataType type{epoch_core::IODataType::Decimal};
    std::string id{};
    std::string name{};

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

    void decode(YAML::Node const &);
    YAML::Node encode() const { return {}; }
  };

  using TransformsMetaDataCreator =
      std::function<TransformsMetaData(std::string const &name)>;

  struct IOMetaDataConstants {
    // TODO: Move bar attributes to shared headers
    inline static IOMetaData CLOSE_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                  "c", "Close Price"};
    inline static IOMetaData OPEN_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                 "o", "Open Price"};
    inline static IOMetaData HIGH_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                 "h", "High Price"};
    inline static IOMetaData LOW_PRICE_METADATA{epoch_core::IODataType::Decimal,
                                                "l", "Low Price"};
    inline static IOMetaData VOLUME_METADATA{epoch_core::IODataType::Decimal, "v",
                                             "Volume"};
    inline static IOMetaData CONTRACT_METADATA{epoch_core::IODataType::String,
                                               "s", "Contract"};

    inline static IOMetaData ANY_INPUT_METADATA{epoch_core::IODataType::Any, ARG,
                                                ""};
    inline static IOMetaData ANY_DECIMAL_INPUT_METADATA{
      epoch_core::IODataType::Decimal, ARG, ""};
    inline static IOMetaData ANY_NUMBER_INPUT_METADATA{
      epoch_core::IODataType::Number, ARG, ""};

    inline static std::unordered_map<std::string, IOMetaData> MAP{
        {"CLOSE", CLOSE_PRICE_METADATA},
        {"OPEN", OPEN_PRICE_METADATA},
        {"HIGH", HIGH_PRICE_METADATA},
        {"LOW", LOW_PRICE_METADATA},
        {"VOLUME", VOLUME_METADATA},
        {"CONTRACT", CONTRACT_METADATA},
        {"DECIMAL", ANY_DECIMAL_INPUT_METADATA},
        {"NUMBER", ANY_NUMBER_INPUT_METADATA},
        {"ANY", ANY_INPUT_METADATA}};
  };

  std::vector<TransformsMetaData> MakeComparativeMetaData();
  std::vector<TransformsMetaData> MakeMathMetaData();
  std::vector<TransformsMetaData> MakeDataSource();
  std::vector<TransformsMetaData> MakeTradeSignalExecutor();
  std::vector<TransformsMetaData> MakeTulipIndicators();
  std::vector<TransformsMetaData> MakeTulipCandles();
} // namespace metadata::transforms

namespace YAML {
template <> struct convert<metadata::transforms::IOMetaData> {
  static bool decode(const Node &node, metadata::transforms::IOMetaData &t) {
    t.decode(node);
    return true;
  }
};

template <> struct convert<metadata::transforms::TransformsMetaData> {
  static bool decode(const Node &node,
                     metadata::transforms::TransformsMetaData &t) {
    t.decode(node);
    return true;
  }
};
} // namespace YAML
