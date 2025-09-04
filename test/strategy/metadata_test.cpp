#include "../common.h"
#include "epoch_metadata/strategy/metadata.h" // Your metadata declarations
#include <catch2/catch_all.hpp>
#include <epoch_metadata/transforms/registration.h>
#include <yaml-cpp/yaml.h>

// For convenience:
using namespace epoch_metadata;
using namespace epoch_metadata::strategy;

TEST_CASE("SessionVariant decode - success", "[SessionVariant]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  std::string yaml_str = R"(
  session: { start: "09:00", end: "16:00" }
  )";

  YAML::Node node = YAML::Load(yaml_str);
  auto session = node["session"].as<SessionVariant>();
  REQUIRE(std::holds_alternative<epoch_frame::SessionRange>(session));
  auto session_range = std::get<epoch_frame::SessionRange>(session);
  REQUIRE(session_range.start == TimeFromString("09:00"));
  REQUIRE(session_range.end == TimeFromString("16:00"));
}

TEST_CASE("AlgorithmNode decode - success", "[AlgorithmNode]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Example of minimal YAML that references the "atr" transform
  // (already registered above).
  std::string yaml_str = R"(
type: atr
options:
  period: 20
inputs:
  ARG: "c"
session: "NewYork"
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
  // ATR requires timeframe, so session should be set
  REQUIRE(algoNode.session.has_value());
  // Default is SessionType::NewYork
  REQUIRE(std::holds_alternative<epoch_core::SessionType>(*algoNode.session));
  REQUIRE(std::get<epoch_core::SessionType>(*algoNode.session) ==
          epoch_core::SessionType::NewYork);
}

TEST_CASE("AlgorithmNode decode ref - success", "[AlgorithmNode]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Example of minimal YAML that references the "atr" transform
  // (already registered above).
  std::string yaml_str = R"(
type: atr
options:
  period: .periodParam
inputs:
  ARG: "c"
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
  // ATR requires timeframe, so session should be set
  REQUIRE_FALSE(algoNode.session.has_value());
}

TEST_CASE("AlgorithmNode decode - missing required option throws",
          "[AlgorithmNode]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Attempt to load an 'atr' transform node but omit the required 'period'
  // option:
  std::string yaml_str = R"(
type: atr
options: {}   # 'period' is not provided
inputs:
  ARG: "c"
)";

  YAML::Node node = YAML::Load(yaml_str);

  // Expecting a std::runtime_error about a missing required field.
  REQUIRE_THROWS_AS(node.as<AlgorithmNode>(), std::runtime_error);
}

TEST_CASE("AlgorithmNode decode - unknown transform type throws",
          "[AlgorithmNode]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // 'nonexistent_transform' is not in the registry
  std::string yaml_str = R"(
type: nonexistent_transform
id: some_id
options:
  period: 10
)";

  YAML::Node node = YAML::Load(yaml_str);
  REQUIRE_THROWS_AS(node.as<AlgorithmNode>(), std::runtime_error);
}

TEST_CASE("AlgorithmNode decode - unknown extra option throws",
          "[AlgorithmNode]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

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
  REQUIRE_THROWS_WITH(node.as<AlgorithmNode>(),
                      Catch::Matchers::ContainsSubstring("Unknown options"));
}

// Now we test decoding some of the higher-level structs (AlgorithmMetaData,
// TradeSignalMetaData, etc.)

TEST_CASE("AlgorithmMetaData decode - success", "[AlgorithmMetaData]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

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

// You can add more negative/edge tests for TradeSignalMetaData, e.g. missing
// `type` field or an invalid enum, etc.
