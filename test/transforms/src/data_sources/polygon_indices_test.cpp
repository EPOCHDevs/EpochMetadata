#include <catch2/catch_test_macros.hpp>
#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/registry.h"
#include "transforms/src/data_sources/polygon_indices_metadata.h"

using namespace epoch_metadata::transforms;
using namespace epoch_metadata::transform;

TEST_CASE("Polygon Indices Metadata Registration", "[polygon_indices]") {
  SECTION("MakePolygonIndicesDataSources returns two nodes") {
    auto metadataList = MakePolygonIndicesDataSources();
    REQUIRE(metadataList.size() == 2);
  }

  SECTION("Common Indices node has correct basic properties") {
    auto metadataList = MakePolygonIndicesDataSources();
    auto& commonIndices = metadataList[0];

    REQUIRE(commonIndices.id == "common_indices");
    REQUIRE(commonIndices.name == "Common Indices");
    REQUIRE(commonIndices.category == epoch_core::TransformCategory::DataSource);
    REQUIRE(commonIndices.plotKind == epoch_core::TransformPlotKind::Null);
    REQUIRE(commonIndices.requiresTimeFrame == false);
  }

  SECTION("Dynamic Indices node has correct basic properties") {
    auto metadataList = MakePolygonIndicesDataSources();
    auto& indices = metadataList[1];

    REQUIRE(indices.id == "indices");
    REQUIRE(indices.name == "Indices");
    REQUIRE(indices.category == epoch_core::TransformCategory::DataSource);
    REQUIRE(indices.plotKind == epoch_core::TransformPlotKind::Null);
    REQUIRE(indices.requiresTimeFrame == false);
  }
}

TEST_CASE("Common Indices Configuration", "[polygon_indices][common_indices]") {
  auto metadataList = MakePolygonIndicesDataSources();
  auto& commonIndices = metadataList[0];

  SECTION("Has index SelectOption parameter") {
    REQUIRE(commonIndices.options.size() == 1);
    auto& indexOption = commonIndices.options[0];

    REQUIRE(indexOption.id == "index");
    REQUIRE(indexOption.name == "Index");
    REQUIRE(indexOption.type == epoch_core::MetaDataOptionType::Select);
    REQUIRE(indexOption.desc == "Select the market index");
  }

  SECTION("SelectOption contains common indices") {
    auto& indexOption = commonIndices.options[0];
    REQUIRE(indexOption.selectOption.size() == 10);

    // Verify a few key indices are present
    bool hasSPX = false;
    bool hasDJI = false;
    bool hasVIX = false;

    for (const auto& opt : indexOption.selectOption) {
      if (opt.name == "SPX" && opt.value == "S&P 500") hasSPX = true;
      if (opt.name == "DJI" && opt.value == "Dow Jones Industrial Average") hasDJI = true;
      if (opt.name == "VIX" && opt.value == "CBOE Volatility Index") hasVIX = true;
    }

    REQUIRE(hasSPX);
    REQUIRE(hasDJI);
    REQUIRE(hasVIX);
  }

  SECTION("Has correct output fields") {
    REQUIRE(commonIndices.outputs.size() == 8);

    // Verify required outputs
    REQUIRE(commonIndices.outputs[0].id == "open");
    REQUIRE(commonIndices.outputs[0].type == epoch_core::IODataType::Decimal);
    REQUIRE(commonIndices.outputs[0].allowMultipleConnections == true);

    REQUIRE(commonIndices.outputs[1].id == "high");
    REQUIRE(commonIndices.outputs[2].id == "low");
    REQUIRE(commonIndices.outputs[3].id == "close");

    REQUIRE(commonIndices.outputs[4].id == "volume");
    REQUIRE(commonIndices.outputs[4].allowMultipleConnections == false);

    REQUIRE(commonIndices.outputs[5].id == "vw");
    REQUIRE(commonIndices.outputs[6].id == "n");
    REQUIRE(commonIndices.outputs[6].type == epoch_core::IODataType::Integer);

    REQUIRE(commonIndices.outputs[7].id == "timestamp");
    REQUIRE(commonIndices.outputs[7].type == epoch_core::IODataType::Integer);
  }

  SECTION("Has no input fields") {
    REQUIRE(commonIndices.inputs.empty());
  }

  SECTION("Has requiredDataSources set to 'c'") {
    REQUIRE(commonIndices.requiredDataSources.size() == 1);
    REQUIRE(commonIndices.requiredDataSources[0] == "c");
  }

  SECTION("Has strategy metadata") {
    REQUIRE(!commonIndices.strategyTypes.empty());
    REQUIRE(!commonIndices.assetRequirements.empty());
    REQUIRE(!commonIndices.usageContext.empty());
    REQUIRE(!commonIndices.limitations.empty());

    // Verify API endpoint mentioned in description
    REQUIRE(commonIndices.desc.find("Polygon.io") != std::string::npos);
    REQUIRE(commonIndices.desc.find("/v2/aggs/ticker/{ticker}/range") != std::string::npos);
  }
}

TEST_CASE("Dynamic Indices Configuration", "[polygon_indices][indices]") {
  auto metadataList = MakePolygonIndicesDataSources();
  auto& indices = metadataList[1];

  SECTION("Has ticker String parameter") {
    REQUIRE(indices.options.size() == 1);
    auto& tickerOption = indices.options[0];

    REQUIRE(tickerOption.id == "ticker");
    REQUIRE(tickerOption.name == "Index Ticker");
    REQUIRE(tickerOption.type == epoch_core::MetaDataOptionType::String);
    REQUIRE(tickerOption.desc == "Index ticker symbol (e.g., SPX, DJI, NDX)");
  }

  SECTION("Has same output fields as common_indices") {
    REQUIRE(indices.outputs.size() == 8);
    REQUIRE(indices.outputs[0].id == "open");
    REQUIRE(indices.outputs[1].id == "high");
    REQUIRE(indices.outputs[2].id == "low");
    REQUIRE(indices.outputs[3].id == "close");
    REQUIRE(indices.outputs[4].id == "volume");
    REQUIRE(indices.outputs[5].id == "vw");
    REQUIRE(indices.outputs[6].id == "n");
    REQUIRE(indices.outputs[7].id == "timestamp");
  }

  SECTION("Has no input fields") {
    REQUIRE(indices.inputs.empty());
  }

  SECTION("Has requiredDataSources set to 'c'") {
    REQUIRE(indices.requiredDataSources.size() == 1);
    REQUIRE(indices.requiredDataSources[0] == "c");
  }

  SECTION("Has comprehensive descriptions") {
    REQUIRE(!indices.desc.empty());
    REQUIRE(!indices.usageContext.empty());
    REQUIRE(!indices.limitations.empty());

    REQUIRE(indices.desc.find("dynamic ticker symbol") != std::string::npos);
    REQUIRE(indices.limitations.find("Polygon.io subscription") != std::string::npos);
  }
}
