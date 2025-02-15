//
// Created by dewe on 1/10/23.
//
#include "metadata.h"
#include "indicators.h"
#include "candles.h"
#include <yaml-cpp/yaml.h>
#include "../doc_deserialization_helper.h"


namespace metadata::transforms {

    void IOMetaData::decode(const YAML::Node & element) {
        if (element.IsScalar()) {
            *this = epoch::lookup(IOMetaDataConstants::MAP, element.as<std::string>());
        } else {
            id = element["id"].as<std::string>();
            name = element["name"].as<std::string>("");
            type = IODataTypeWrapper::FromString(element["type"].as<std::string>());
        }
    }

    void TransformsMetaData::decode(const YAML::Node & element) {
        id = element["id"].as<std::string>();
        name = element["name"].as<std::string>();
        type = TransformTypeWrapper::FromString(element["type"].as<std::string>());
        inputs = element["inputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
        outputs = element["outputs"].as<std::vector<IOMetaData>>(std::vector<IOMetaData>{});
        options = element["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        desc = MakeDescLink(element["desc"].as<std::string>(""));
        isCrossSectional = element["isCrossSectional"].as<bool>(false);
    }

    TransformsMetaData MakeZeroIndexSelectMetaData(std::string const &name);

    TransformsMetaData MakeBooleanSelectMetaData(std::string const &id, std::string const &name) {
        return {
                .id = id,
                .name = name,
                .options = {},
                .type = TransformType::Comparative,
                .isCrossSectional = false,
                .desc = "Select based on boolean input.",
                .inputs = {
                        {IODataType::Boolean, "index", "Index"},
                        {IODataType::Any,     "true",  "True"},
                        {IODataType::Any,     "false", "False"}
                },
                .outputs = {
                        {IODataType::Any, "selected", ""}
                }
        };
    }

    TransformsMetaData MakeEqualityTransformMetaData(std::string const &id, bool isVector, std::string const &name) {
        TransformsMetaData metadata;

        metadata.id = id;
        metadata.name = name;

        if (!isVector) {
            metadata.options = {
                    MetaDataOption{
                            .id="value",
                            .name="Value",
                            .type=MetaDataOptionType::Decimal
                    }
            };
        }

        metadata.type = TransformType::Comparative; // Adjust type as necessary
        metadata.isCrossSectional = false;
        metadata.desc = "";

        // Inputs
        if (isVector) {
            metadata.inputs.emplace_back(IODataType::Number, ARG0, "");
            metadata.inputs.emplace_back(IODataType::Number, ARG1, "");
        } else {
            metadata.inputs.emplace_back(IODataType::Number, ARG, "");
        }

        // Output
        metadata.outputs = {
                {IODataType::Boolean, "result", ""}
        };

        return metadata;
    }

    TransformsMetaData MakeMathTransformMetaData(std::string const &id, std::string const &name) {
        TransformsMetaData metadata;

        metadata.id = id;
        metadata.name = name;
        metadata.type = TransformType::Math; // Adjust type as necessary
        metadata.isCrossSectional = false;
        metadata.desc = "";

        metadata.options.emplace_back(MetaDataOption{
            .id="value",
            .name="Value",
            .type=MetaDataOptionType::Integer
        });

        // Inputs
        metadata.inputs.emplace_back(IODataType::Number, ARG, "");

        // Output
        metadata.outputs = {
                {IODataType::Number, "result", ""}
        };

        return metadata;
    }

    TransformsMetaData MakeZeroIndexSelectMetaData(std::string const &id, std::string const &name) {
        // Ensure the name ends with an underscore followed by a number, e.g., "zero_index_select_3"
        size_t underscore_pos = id.find_last_of('_');
        if (underscore_pos == std::string::npos || underscore_pos == id.size() - 1) {
            throw std::invalid_argument("Invalid Name, must end with an underscore followed by a digit.");
        }

        std::string number_part = id.substr(underscore_pos + 1);
        for (char c: number_part) {
            if (!std::isdigit(c)) {
                throw std::invalid_argument("Invalid Name, last part after underscore must be digits.");
            }
        }
        const size_t N = std::stoul(number_part);

        TransformsMetaData metadata;
        metadata.id = id;
        metadata.name = name;
        metadata.options = {}; // Add any specific options if needed
        metadata.type = TransformType::Comparative,
                metadata.isCrossSectional = false;
        metadata.desc = "Select based on zero based index";

        // Inputs: "index", "option_0", "option_1", ..., "option_{N-1}"
        std::vector<IOMetaData> inputs;
        inputs.emplace_back(IODataType::Integer, "index", "Index");
        for (size_t i = 0; i < N; ++i) {
            inputs.emplace_back(IODataType::Any, fmt::format("option_{}", i), fmt::format("Option {}", i));
        }
        metadata.inputs = inputs;

        // Output: "selected"
        metadata.outputs = {
                {IODataType::Any, "selected", ""}
        };

        return metadata;
    }

    TransformsMetaData MakeLogicalTransformMetaData(std::string const &id, std::string const &name) {
        TransformsMetaData metadata;

        metadata.id = id;
        metadata.name = name;
        metadata.options = {}; // Add any specific options if needed
        metadata.type = TransformType::Comparative; // Adjust type as necessary
        metadata.isCrossSectional = false;
        metadata.desc = "";

        // Inputs
        if (id == "logical_not") {
            metadata.inputs.emplace_back(IODataType::Boolean, ARG, "");
        } else {
            metadata.inputs.emplace_back(IODataType::Boolean, ARG0, "");
            metadata.inputs.emplace_back(IODataType::Boolean, ARG1, "");
        }

        // Output
        metadata.outputs = {
                {IODataType::Boolean, "result", ""}
        };

        return metadata;
    }

    std::vector<TransformsMetaData> MakeComparativeMetaData() {
        std::vector<TransformsMetaData> metadataList;

        // Define constant equality transforms
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_gt", false, "Greater Than"));   // Greater Than
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_gte", false, "Greater Than or Equal"));  // Greater Than Or Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_lt", false, "Less Than"));   // Less Than
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_lte", false, "Less Than or Equal"));  // Less Than Or Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_eq", false, "Equal"));   // Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("constant_neq", false, "Not Equal"));  // Not Equals

        // Define vector equality transforms
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_gt", true, "Vector Greater Than"));      // Greater Than
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_gte", true, "Vector Greater Than or Equal"));     // Greater Than Or Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_lt", true, "Vector Less Than"));      // Less Than
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_lte", true, "Vector Less Than or Equal"));     // Less Than Or Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_eq", true, "Vector Equal"));      // Equals
        metadataList.emplace_back(MakeEqualityTransformMetaData("vector_neq", true, "Vector Not Equal"));      // Not Equals

        metadataList.emplace_back(MakeBooleanSelectMetaData("boolean_select", "If Else"));
        metadataList.emplace_back(MakeZeroIndexSelectMetaData("select_2", "Select 2"));
        metadataList.emplace_back(MakeZeroIndexSelectMetaData("select_3", "Select 3"));
        metadataList.emplace_back(MakeZeroIndexSelectMetaData("select_4", "Select 4"));
        metadataList.emplace_back(MakeZeroIndexSelectMetaData("select_5", "Select 5"));

        metadataList.emplace_back(MakeLogicalTransformMetaData("logical_or", "OR"));
        metadataList.emplace_back(MakeLogicalTransformMetaData("logical_and", "AND"));
        metadataList.emplace_back(MakeLogicalTransformMetaData("logical_not", "NOT"));
        metadataList.emplace_back(MakeLogicalTransformMetaData("logical_and_not", "AND NOT"));
        metadataList.emplace_back(MakeLogicalTransformMetaData("logical_xor", "XOR"));

        return metadataList;
    }

    std::vector<TransformsMetaData> MakeMathMetaData() {
        std::vector<TransformsMetaData> metadataList;

        metadataList.emplace_back(MakeMathTransformMetaData("constant_add", "Add"));
        metadataList.emplace_back(MakeMathTransformMetaData("constant_sub", "Subtract"));
        metadataList.emplace_back(MakeMathTransformMetaData("constant_div", "Divide"));
        metadataList.emplace_back(MakeMathTransformMetaData("constant_mul",  "Multiply"));
        metadataList.emplace_back(MakeMathTransformMetaData("constant_exp", "Exponent"));

        return metadataList;
    }

    auto beautify = [](std::string const &id) {
        std::vector<std::string> parts;
        boost::split(parts, id, boost::is_any_of("_"));

        for (auto &part: parts) {
            if (!part.empty()) {
                // Convert entire word to lowercase first
                boost::algorithm::to_lower(part);

                // Capitalize first character
                part[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(part[0])));
            }
        }

        return boost::algorithm::join(parts, " ");
    };

    std::vector<TransformsMetaData> MakeDataSource() {
        std::vector<TransformsMetaData> result;

        // Refactored Names Applied Below
        result.emplace_back(TransformsMetaData{
                .id = "PriceBar",
                .name = "Market Price",
                .options= {},
                .type = TransformType::DataSource,
                .desc = "",
                .outputs = {IOMetaDataConstants::CLOSE_PRICE_METADATA,
                            IOMetaDataConstants::OPEN_PRICE_METADATA,
                            IOMetaDataConstants::HIGH_PRICE_METADATA,
                            IOMetaDataConstants::LOW_PRICE_METADATA},
        });

        return result;
    }

    std::vector<TransformsMetaData> MakeTradeSignalExecutor() {
        std::vector<TransformsMetaData> result;

        IOMetaData longMetaData{
                .type = IODataType::Boolean,
                .id = "long",
                .name = "Enter Long Trade"
        };

        IOMetaData shortMetaData{
                .type = IODataType::Boolean,
                .id = "short",
                .name = "Enter Short Trade"
        };

        IOMetaData closePositionMetaData{
                .type = IODataType::Boolean,
                .id = "close",
                .name = "Exit Trade",
        };

        MetaDataOption closeIfIndecisive{
                .id="closeIfIndecisive",
                .name="Exit If Indecisive",
                .type=MetaDataOptionType::Boolean,
                .defaultValue=false,
        };

        return {
                TransformsMetaData{
                        .id = "TradeSignalExecutor",
                        .name = "Trade Signal Executor",
                        .options= {closeIfIndecisive},
                        .type = TransformType::TradeSignalExecutor,
                        .desc = "",
                        .inputs = {longMetaData, shortMetaData, closePositionMetaData},
                        .atLeastOneInputRequired = true
                }
        };
    }

    std::vector<TransformsMetaData> MakeTulipIndicators() {

        auto makeTulipOptions = [&](std::string const &option) {
            static std::unordered_set<std::string> skip{
                    "open", "high", "low", "close", "volume"
            };
            MetaDataOption optionMetaData{
                    .id =option,
                    .name = beautify(option),
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue=std::nullopt,
                    .isRequired=true,
                    .selectOption={}
            };

            if (option.starts_with("period") || option.ends_with("period")) {
                optionMetaData.type = MetaDataOptionType::Integer;
                optionMetaData.min = 0;
                optionMetaData.max = 10000;
            } else if (option == "stddev") {
                optionMetaData.type = MetaDataOptionType::Integer;
                optionMetaData.min = 0;
                optionMetaData.max = 10;
            }
            return optionMetaData;
        };

        auto makeTulipInputs = [](auto const& inputs) {

            static std::unordered_set<std::string> skip{
                    "open", "high", "low", "close", "volume"
            };
            std::vector<IOMetaData> ioMetaDataList;
            bool useSingleWildCard = inputs.size() == 1;
            for (auto const& [i, input]: ranges::view::enumerate(inputs))
            {
                std::string_view inputStr{input};

                IOMetaData ioMetaData;
                if (inputStr == "real") {
                    ioMetaData.id = useSingleWildCard ? ARG : fmt::format("{}{}", ARG, i);
                } else {
                    AssertWithTraceFromStream(skip.contains(input), "Invalid tulip input id: " << input);
                    ioMetaData.id = inputStr.at(0);
                    //  name won't be displayed anyway
                }
                ioMetaDataList.emplace_back(ioMetaData);
            }

            return ioMetaDataList;
        };

        auto makeTulipOutputs = [&](std::string const &output) -> IOMetaData {
            return {
                    .type=(output == "crossany" || output == "crossover") ? IODataType::Boolean : IODataType::Decimal,
                    .id=output,
                    .name = beautify(output)
            };
        };

        auto getType = [](int type) -> TransformType {
            switch (type) {
                case TI_TYPE_OVERLAY:
                    return TransformType::Overlay;
                case TI_TYPE_INDICATOR:
                    return TransformType::Indicator;
                case TI_TYPE_MATH:
                    return TransformType::Math;
                case TI_TYPE_SIMPLE:
                    return TransformType::Math;
                case TI_TYPE_COMPARATIVE:
                    return TransformType::Comparative;
                default:
                    break;
            }
            ThrowWithTraceFromStream("Invalid Tulip Type: " << type);
        };

        std::vector<TransformsMetaData> allIndicators(TI_INDICATOR_COUNT);
        std::ranges::transform(std::span{ti_indicators, ti_indicators + TI_INDICATOR_COUNT}, allIndicators.begin(),
                               [&](const ti_indicator_info &tiIndicatorInfo) {
                                   const std::span optionSpan{tiIndicatorInfo.option_names,
                                                              tiIndicatorInfo.option_names + tiIndicatorInfo.options};
                                   const std::span inputSpan{tiIndicatorInfo.input_names,
                                                             tiIndicatorInfo.input_names + tiIndicatorInfo.inputs};
                                   const std::span outputSpan{tiIndicatorInfo.output_names,
                                                              tiIndicatorInfo.output_names + tiIndicatorInfo.outputs};
                                   
                                   return TransformsMetaData{
                                           .id = tiIndicatorInfo.name,
                                           .name = tiIndicatorInfo.full_name,
                                           .options = ranges::to<std::vector>(
                                                   optionSpan | std::views::transform(makeTulipOptions)),
                                           .type = getType(tiIndicatorInfo.type),
                                           .isCrossSectional = false,
                                           .desc=fmt::format("https://tulipindicators.org/{}", tiIndicatorInfo.name),
                                           .inputs = makeTulipInputs(inputSpan),
                                           .outputs = ranges::to<std::vector>(
                                                   outputSpan | std::views::transform(makeTulipOutputs))};
                               });
        return allIndicators;
    }

    std::vector<MetaDataOption> MakeCandleOptions() {
        // Derive defaults from tc_config_default()
        auto defaults = tc_config_default();

        std::vector<MetaDataOption> options;

        // period is integer
        {
            MetaDataOption o{
                    .id = "period",
                    .name = "Period",
                    .type = MetaDataOptionType::Integer,
                    .defaultValue = static_cast<double>(defaults->period),
                    .isRequired = true,
                    .min = 0,
                    .max = 1000
            };
            options.push_back(o);
        }

        // body_none
        {
            MetaDataOption o{
                    .id = "body_none",
                    .name = "Body None Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->body_none),
                    .isRequired = true
            };
            options.push_back(o);
        }

        // body_short
        {
            MetaDataOption o{
                    .id = "body_short",
                    .name = "Body Short Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->body_short),
                    .isRequired = true
            };
            options.push_back(o);
        }

        // body_long
        {
            MetaDataOption o{
                    .id = "body_long",
                    .name = "Body Long Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->body_long),
                    .isRequired = true
            };
            options.push_back(o);
        }

        // wick_none
        {
            MetaDataOption o{
                    .id = "wick_none",
                    .name = "Wick None Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->wick_none),
                    .isRequired = true
            };
            options.push_back(o);
        }

        // wick_long
        {
            MetaDataOption o{
                    .id = "wick_long",
                    .name = "Wick Long Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->wick_long),
                    .isRequired = true
            };
            options.push_back(o);
        }

        // near
        {
            MetaDataOption o{
                    .id = "near",
                    .name = "Near Threshold",
                    .type = MetaDataOptionType::Decimal,
                    .defaultValue = static_cast<double>(defaults->near),
                    .isRequired = true
            };
            options.push_back(o);
        }

        return options;
    }

    std::vector<TransformsMetaData> MakeTulipCandles() {
        // Iterate over tc_candles until we hit a null element.
        std::vector<TransformsMetaData> allCandles(TC_CANDLE_COUNT);

        auto makeCandleOutputs = [](tc_candle_info const &info) {
            // Output: A single output indicating the presence of the pattern.
            // Named after the candle pattern.
            return std::vector<IOMetaData>{
                    {
                            .type = IODataType::Decimal,
                            .id = info.name,
                            .name = beautify(info.name)
                    }
            };
        };

        std::ranges::transform(std::span{tc_candles, tc_candles + TC_CANDLE_COUNT}, allCandles.begin(),
                               [makeCandleOutputs](const auto &c) {
                                   return TransformsMetaData{
                                           .id = c.name,
                                           .name = c.full_name,
                                           .options = MakeCandleOptions(),
                                           .type = TransformType::CandleStickPattern,
                                           .isCrossSectional = false,
                                           .desc = "https://www.strike.money/technical-analysis/types-of-candlesticks-patterns",
                                           .inputs = {
                                                   IOMetaDataConstants::CLOSE_PRICE_METADATA,
                                                   IOMetaDataConstants::OPEN_PRICE_METADATA,
                                                   IOMetaDataConstants::HIGH_PRICE_METADATA,
                                                   IOMetaDataConstants::LOW_PRICE_METADATA
                                           },
                                           .outputs = makeCandleOutputs(c)
                                   };
                               });

        return allCandles;
    }
}