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

// ============================================================================
// PythonSource Tests
// ============================================================================

TEST_CASE("PythonSource - empty source", "[PythonSource]") {
  PythonSource emptySource("");

  REQUIRE(emptySource.GetSource().empty());
  REQUIRE(emptySource.GetCompilationResult().empty());
  REQUIRE_FALSE(emptySource.GetBaseTimeframe().has_value());
  REQUIRE_FALSE(emptySource.IsIntraday());
}

TEST_CASE("PythonSource - EOD timeframe detection", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Simple algorithm using daily (EOD) data
  std::string source = R"(
sma_fast = SMA(close, period=10, timeframe='1D')
sma_slow = SMA(close, period=20, timeframe='1D')
signal = sma_fast > sma_slow
)";

  PythonSource pythonSource(source);

  REQUIRE_FALSE(pythonSource.GetCompilationResult().empty());
  REQUIRE(pythonSource.GetBaseTimeframe().has_value());
  REQUIRE(pythonSource.GetBaseTimeframe().value() == epoch_core::BaseDataTimeFrame::EOD);
  REQUIRE_FALSE(pythonSource.IsIntraday());
}

TEST_CASE("PythonSource - intraday timeframe detection", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Algorithm using minute (intraday) data
  std::string source = R"(
vwap_1min = VWAP(timeframe='1min')
signal = close > vwap_1min
)";

  PythonSource pythonSource(source);

  REQUIRE_FALSE(pythonSource.GetCompilationResult().empty());
  REQUIRE(pythonSource.GetBaseTimeframe().has_value());
  REQUIRE(pythonSource.GetBaseTimeframe().value() == epoch_core::BaseDataTimeFrame::Minute);
  REQUIRE(pythonSource.IsIntraday());
}

TEST_CASE("PythonSource - session implies intraday", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Algorithm with session (implies intraday)
  std::string source = R"(
atr_ny = ATR(period=14, session='NewYork')
signal = close > atr_ny
)";

  PythonSource pythonSource(source);

  REQUIRE_FALSE(pythonSource.GetCompilationResult().empty());
  REQUIRE(pythonSource.GetBaseTimeframe().has_value());
  REQUIRE(pythonSource.GetBaseTimeframe().value() == epoch_core::BaseDataTimeFrame::Minute);
  REQUIRE(pythonSource.IsIntraday());
}

TEST_CASE("PythonSource - no explicit timeframe", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Algorithm without explicit timeframe
  std::string source = R"(
sma = SMA(close, period=10)
signal = close > sma
)";

  PythonSource pythonSource(source);

  REQUIRE_FALSE(pythonSource.GetCompilationResult().empty());
  // No explicit timeframe should result in nullopt
  REQUIRE_FALSE(pythonSource.GetBaseTimeframe().has_value());
  REQUIRE_FALSE(pythonSource.IsIntraday());
}

TEST_CASE("PythonSource - equality operator", "[PythonSource]") {
  std::string source1 = "signal = close > SMA(close, 10)";
  std::string source2 = "signal = close > SMA(close, 10)";
  std::string source3 = "signal = close > SMA(close, 20)";

  PythonSource ps1(source1);
  PythonSource ps2(source2);
  PythonSource ps3(source3);

  REQUIRE(ps1 == ps2);  // Same source
  REQUIRE_FALSE(ps1 == ps3);  // Different source
}

TEST_CASE("PythonSource - glaze write_json serialization", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  std::string source = R"(signal = close > SMA(close, period=10, timeframe='1D'))";
  PythonSource original(source);

  // Serialize PythonSource to JSON
  auto json = glz::write_json(original);
  REQUIRE(json.has_value());

  // Serialize the raw string to JSON for comparison
  auto expectedJson = glz::write_json(source);
  REQUIRE(expectedJson.has_value());

  // PythonSource should serialize as just the source string
  REQUIRE(json.value() == expectedJson.value());
}

TEST_CASE("PythonSource - glaze read_json deserialization", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // JSON string containing EpochFlow source code
  std::string jsonInput = R"("signal = close > SMA(close, period=20, timeframe='1D')")";

  // Deserialize from JSON
  PythonSource deserialized;
  auto parseResult = glz::read_json(deserialized, jsonInput);

  REQUIRE_FALSE(parseResult);  // No error
  REQUIRE(deserialized.GetSource() == "signal = close > SMA(close, period=20, timeframe='1D')");
  REQUIRE_FALSE(deserialized.GetCompilationResult().empty());
  REQUIRE(deserialized.GetBaseTimeframe().has_value());
  REQUIRE(deserialized.GetBaseTimeframe().value() == epoch_core::BaseDataTimeFrame::EOD);
  REQUIRE_FALSE(deserialized.IsIntraday());
}

TEST_CASE("PythonSource - glaze round-trip serialization", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  std::string source = R"(
vwap = VWAP(timeframe='5min')
sma = SMA(close, period=10, timeframe='5min')
signal = vwap > sma
)";
  PythonSource original(source);

  // Write to JSON
  auto json = glz::write_json(original);
  REQUIRE(json.has_value());

  // Read from JSON
  PythonSource deserialized;
  auto parseResult = glz::read_json(deserialized, json.value());
  REQUIRE_FALSE(parseResult);  // No error

  // Verify round-trip preserves all data
  REQUIRE(deserialized.GetSource() == original.GetSource());
  REQUIRE(deserialized.GetCompilationResult().size() == original.GetCompilationResult().size());
  REQUIRE(deserialized.GetBaseTimeframe() == original.GetBaseTimeframe());
  REQUIRE(deserialized.IsIntraday() == original.IsIntraday());
  REQUIRE(deserialized == original);  // Test equality operator
}

TEST_CASE("PythonSource - glaze deserialization triggers compilation", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  // Create JSON with intraday source
  std::string jsonInput = R"("atr = ATR(period=14, session='NewYork')")";

  // Deserialize - should compile and detect intraday
  PythonSource pythonSource;
  auto parseResult = glz::read_json(pythonSource, jsonInput);

  REQUIRE_FALSE(parseResult);
  REQUIRE_FALSE(pythonSource.GetCompilationResult().empty());
  REQUIRE(pythonSource.GetBaseTimeframe().has_value());
  REQUIRE(pythonSource.GetBaseTimeframe().value() == epoch_core::BaseDataTimeFrame::Minute);
  REQUIRE(pythonSource.IsIntraday());
}

TEST_CASE("PythonSource - compilation result is cached", "[PythonSource]") {
  transforms::RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  std::string source = R"(
sma = SMA(close, period=10, timeframe='1D')
signal = close > sma
)";

  PythonSource pythonSource(source);

  // Verify compilation happened once and result is accessible
  const auto& result1 = pythonSource.GetCompilationResult();
  const auto& result2 = pythonSource.GetCompilationResult();

  REQUIRE_FALSE(result1.empty());
  REQUIRE(&result1 == &result2);  // Same reference (cached)
}
