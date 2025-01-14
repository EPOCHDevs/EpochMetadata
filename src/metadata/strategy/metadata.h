//
// Created by dewe on 9/10/24.
//

#pragma once
#include "metadata_options.h"
#include "enum_wrapper.h"

CREATE_ENUM(AlgorithmType, TakeProfit, StopLoss, Sizer, Commission, Slippage, FuturesContinuation);
CREATE_ENUM(TradeSignalType, TrendFollowing, MeanReverting, CandleStickPattern, Momentum, EventDriven);

namespace metadata::strategy {
    struct AlgorithmBaseMetaData {
        std::string id;
        std::string name;
        MetaDataOptionList options{};
        std::string desc{};
    };

    struct AlgorithmMetaData {
        std::string id;
        std::string name;
        MetaDataOptionList options{};
        std::string desc{};
        bool isGroup{false};
        bool requiresTimeframe{true};
    };

    struct TradeSignalMetaData {
        std::string id;
        std::string name;
        MetaDataOptionList options{};
        std::string desc{};
        bool isGroup{false};
        bool requiresTimeframe{true};
        TradeSignalType type{TradeSignalType::Null};
        std::string algorithm;
        std::string executor;
    };

    // Copy member variables to support glaze serialization form decomposition
}  // namespace metadata::strategy

namespace YAML {
    template<>
    struct convert<metadata::strategy::AlgorithmBaseMetaData> {
        static bool decode(YAML::Node const &, metadata::strategy::AlgorithmBaseMetaData &);
    };

    template<>
    struct convert<metadata::strategy::AlgorithmMetaData> {
        static bool decode(YAML::Node const &, metadata::strategy::AlgorithmMetaData &);
    };

    template<>
    struct convert<metadata::strategy::TradeSignalMetaData> {
        static bool decode(YAML::Node const &, metadata::strategy::TradeSignalMetaData &);
    };
}