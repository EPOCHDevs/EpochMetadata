#pragma once
#include "../metadata_options.h"
#include "../../common/constants.h"
#include <string>
#include <vector>
#include <glaze/glaze.hpp>
#include "common/yaml_utils.h"


CREATE_ENUM(TransformType, Overlay, Indicator, Math, Input, TradeSignalExecutor, Comparative, CandleStickPattern);
CREATE_ENUM(IODataType, Decimal, Integer, Number, Boolean, String, Any);
CREATE_ENUM(TradeSignalExecutorType, LongOnly, LongWithExit, ShortOnly, ShortWithExit, LongShortOnly, LongShortWithExit);

namespace stratifyx::metadata::transforms {
    struct IOMetaData {
        IODataType type{IODataType::Decimal};
        std::string id{};
        std::string name{};

        void decode(YAML::Node const&);
        YAML::Node encode() const{
            return {};
        }
    };

    struct TransformsMetaData {
        std::string id;
        std::string name{};
        MetaDataOptionList options{};
        TransformType type;
        bool isCrossSectional{false};
        std::string desc{};
        std::vector<IOMetaData> inputs{};
        std::vector<IOMetaData> outputs{};

        void decode(YAML::Node const&);
        YAML::Node encode() const{
            return {};
        }
    };

    using TransformsMetaDataCreator = std::function<TransformsMetaData(std::string const &name)>;

    struct IOMetaDataConstants {
        inline static IOMetaData CLOSE_PRICE_METADATA{IODataType::Decimal, CLOSE(), "Close Price"};
        inline static IOMetaData OPEN_PRICE_METADATA{IODataType::Decimal, OPEN(), "Open Price"};
        inline static IOMetaData HIGH_PRICE_METADATA{IODataType::Decimal, HIGH(), "High Price"};
        inline static IOMetaData LOW_PRICE_METADATA{IODataType::Decimal, LOW(), "Low Price"};
        inline static IOMetaData VOLUME_METADATA{IODataType::Decimal, VOLUME(), "Volume"};
        inline static IOMetaData CONTRACT_METADATA{IODataType::String, CONTRACT(), "Contract"};

        inline static IOMetaData ANY_INPUT_METADATA{IODataType::Any, ARG, ""};
        inline static IOMetaData ANY_DECIMAL_INPUT_METADATA{IODataType::Decimal, ARG, ""};
        inline static IOMetaData ANY_NUMBER_INPUT_METADATA{IODataType::Number, ARG, ""};

        inline static std::unordered_map<std::string, IOMetaData> MAP{
                {"CLOSE",    CLOSE_PRICE_METADATA},
                {"OPEN",     OPEN_PRICE_METADATA},
                {"HIGH",     HIGH_PRICE_METADATA},
                {"LOW",      LOW_PRICE_METADATA},
                {"VOLUME",   VOLUME_METADATA},
                {"CONTRACT", CONTRACT_METADATA},
                {"DECIMAL",  ANY_DECIMAL_INPUT_METADATA},
                {"NUMBER",   ANY_NUMBER_INPUT_METADATA},
                {"ANY",      ANY_INPUT_METADATA}
        };
    };

    std::vector<TransformsMetaData> MakeComparativeMetaData();
    std::vector<TransformsMetaData> MakeMathMetaData();
    std::vector<TransformsMetaData> MakeTradeSignalExecutor();
    std::vector<TransformsMetaData> MakeTulipIndicators();
    std::vector<TransformsMetaData> MakeTulipCandles();
}  // namespace stratifyx::metadata::transforms

YAML_CONVERT(stratifyx::metadata::transforms::IOMetaData);
YAML_CONVERT(stratifyx::metadata::transforms::TransformsMetaData);
