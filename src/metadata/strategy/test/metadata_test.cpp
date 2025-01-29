#include <catch2/catch_all.hpp>
#include <yaml-cpp/yaml.h>

#include "../metadata.h"                 // Your metadata declarations


// For convenience:
using namespace metadata;
using namespace metadata::strategy;

/**
 * Helper: Create a mock transform MetaData entry (or real ones if you have them).
 * This is needed so that when AlgorithmNode decoding looks up a transform type,
 * the registry returns the correct TransformsMetaData.
 */
static transforms::TransformsMetaData MakeMockTransformMetaData(
        const std::string &id,
        const std::vector<MetaDataOption> &options
) {
    transforms::TransformsMetaData meta;
    meta.id = id;
    meta.name = "Mock Transform " + id;
    meta.options = options;
    return meta;
}


TEST_CASE("AlgorithmNode decode - success", "[AlgorithmNode]") {
    // Example of minimal YAML that references the "atr" transform
    // (already registered above).
    std::string yaml_str = R"(
type: atr
options:
  period: 20
inputs:
  "*": "c"
)";

    YAML::Node node = YAML::Load(yaml_str);

    auto algoNode = node.as<AlgorithmNode>();
    REQUIRE(algoNode.type == "atr");
    REQUIRE(algoNode.id == "atr");

    // Ensure "period" option was parsed
    REQUIRE(algoNode.options.size() == 1);
    REQUIRE(algoNode.options.count("period") == 1);
    REQUIRE(algoNode.options.at("period").IsType<double>());
    REQUIRE(algoNode.options.at("period").GetInteger() == 20);
}

TEST_CASE("AlgorithmNode decode ref - success", "[AlgorithmNode]") {
    // Example of minimal YAML that references the "atr" transform
    // (already registered above).
    std::string yaml_str = R"(
type: atr
options:
  period: .periodParam
inputs:
  "*": "c"
)";

    YAML::Node node = YAML::Load(yaml_str);

    auto algoNode = node.as<AlgorithmNode>();
    REQUIRE(algoNode.type == "atr");
    REQUIRE(algoNode.id == "atr");

    // Ensure "period" option was parsed
    REQUIRE(algoNode.options.size() == 1);
    REQUIRE(algoNode.options.count("period") == 1);
    REQUIRE(algoNode.options.at("period").IsType<MetaDataArgRef>());
    REQUIRE(algoNode.options.at("period").GetRef() == "periodParam");
}

TEST_CASE("AlgorithmNode decode - missing required option throws", "[AlgorithmNode]") {
    // Attempt to load an 'atr' transform node but omit the required 'period' option:
    std::string yaml_str = R"(
type: atr
options: {}   # 'period' is not provided
inputs:
  "*": "c"
)";

    YAML::Node node = YAML::Load(yaml_str);

    // Expecting a std::runtime_error about a missing required field.
    REQUIRE_THROWS_AS(node.as<AlgorithmNode>(), std::runtime_error);
}

TEST_CASE("AlgorithmNode decode - unknown transform type throws", "[AlgorithmNode]") {
    // 'nonexistent_transform' is not in the registry
    std::string yaml_str = R"(
type: nonexistent_transform
id: some_id
options:
  period: 10
)";

    YAML::Node node = YAML::Load(yaml_str);
    REQUIRE_THROWS_AS(
            node.as<AlgorithmNode>(),
            std::runtime_error
    );
}

TEST_CASE("AlgorithmNode decode - unknown extra option throws", "[AlgorithmNode]") {
    // 'atr' is recognized, but let's pass an unknown field "foo"
    std::string yaml_str = R"(
type: atr
id: test_atr
options:
  period: 10
  foo: "extra"
)";

    YAML::Node node = YAML::Load(yaml_str);

    // The decoding code explicitly checks for leftover fields in 'options'
    // and should throw an error about "Unknown options"
    REQUIRE_THROWS_WITH(
            node.as<AlgorithmNode>(),
            Catch::Matchers::ContainsSubstring("Unknown options")
    );
}

// Now we test decoding some of the higher-level structs (AlgorithmMetaData, TradeSignalMetaData, etc.)

TEST_CASE("AlgorithmMetaData decode - success", "[AlgorithmMetaData]") {
    std::string yaml_str = R"(
id: cppi
name: "Constant Proportion Portfolio Insurance"
options:
  - { id: multiplier, name: "Multiplier", type: Decimal, default: 1 }
  - { id: floorPct,   name: "Floor",      type: Decimal, default: 0.9 }
desc: "$QUANTPEDIA/introduction-to-cppi-constant-proportion-portfolio-insurance"
isGroup: false
requiresTimeframe: false
)";

    YAML::Node node = YAML::Load(yaml_str);
    REQUIRE_NOTHROW(node.as<AlgorithmMetaData>());
    AlgorithmMetaData amd = node.as<AlgorithmMetaData>();

    CHECK(amd.id == "cppi");
    CHECK(amd.name == "Constant Proportion Portfolio Insurance");
    CHECK_FALSE(amd.isGroup);
    CHECK_FALSE(amd.requiresTimeframe);

    REQUIRE(amd.options.size() == 2);
    CHECK(amd.options[0].id == "multiplier");
    CHECK(amd.options[1].id == "floorPct");
    // ... check other fields as needed ...
}

TEST_CASE("TradeSignalMetaData decode - success", "[TradeSignalMetaData]") {
    // Example from your 'atr_scalping' in tradesignals.yaml:
    std::string yaml_str = R"(
id: atr_scalping
name: ATR Scalping
options:
  - { id: atr_period, name: ATR Period, default: 30, type: Integer }
  - { id: atr_rolling_mean_period, name: ATR Rolling Mean Period, default: 5, type: Integer }
  - { id: breakout_period, name: Breakout Period, default: 5, type: Integer }
desc: "https://quantra.quantinsti.com/startCourseDetails?cid=134&section_no=11&unit_no=10&course_type=paid&unit_type=Notebook"
type: Momentum
algorithm:
  - { type: atr, id: atr, options: { period: .atr_period }, inputs: { "*" : "c" } }
  - { type: sma, id: sma, options: { period: .atr_rolling_mean_period }, inputs: { "*" : "atr#atr" } }
executor: { id: LongShortWithExit, type: LongShortOnly, options: { closeIfIndecisive: true }, inputs: { long: "long#vector_gt" , short: "short#vector_lt" } }
)";

    YAML::Node node = YAML::Load(yaml_str);
    const TradeSignalMetaData tsmd = node.as<TradeSignalMetaData>();

    CHECK(tsmd.id == "atr_scalping");
    CHECK(tsmd.name == "ATR Scalping");
    CHECK(tsmd.type == TradeSignalType::Momentum); // from the enum you created
    CHECK(tsmd.isGroup == false);
    CHECK(tsmd.requiresTimeframe == true);

    REQUIRE(tsmd.options.size() == 3);
    CHECK(tsmd.options[0].id == "atr_period");
    CHECK(tsmd.options[1].id == "atr_rolling_mean_period");
    CHECK(tsmd.options[2].id == "breakout_period");

    // Now test the first algorithm node
    REQUIRE(tsmd.algorithm.size() == 2);
    CHECK(tsmd.algorithm[0].type == "atr");
    CHECK(tsmd.algorithm[0].id == "atr");
    // ...
    // The second node, etc.

    // And the executor
    CHECK(tsmd.executor.type == "LongShortOnly");
    CHECK(tsmd.executor.id == "LongShortWithExit");
    // ...
}

// You can add more negative/edge tests for TradeSignalMetaData, e.g. missing `type` field or an invalid enum, etc.
