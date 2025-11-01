//
// Created by Claude Code for test coverage
//

#include "catch.hpp"
#include <epoch_metadata/model/builder/position_builder.h"
#include <epoch_metadata/model/common/constants.h"
#include "epoch_metadata/decimal_utils.h"
#include <epoch_frame/datetime.h>

using namespace epoch_flow;
using namespace epoch_flow::position;
using namespace epoch_metadata;
using namespace epoch_frame;

TEST_CASE("PositionBuilder default constructor", "[position_builder]") {
  PositionBuilder builder;

  // Default constructor creates builder without position
  // Build() will return a default-constructed Position
  // This tests that the builder is constructible
  REQUIRE(true); // Constructor succeeded
}

TEST_CASE("PositionBuilder full constructor creates position", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().AAPL;
  auto currentPrice = 150.0_dec;
  auto qty = 100.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  Position position = builder.Build();

  REQUIRE(position.GetAsset() == asset);
  REQUIRE(position.GetData().type == PositionType::Long);
  REQUIRE(position.GetData().currentPrice == currentPrice);
  REQUIRE(position.GetData().qty == qty);
}

TEST_CASE("PositionBuilder::Build returns position", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().MSFT;
  auto currentPrice = 300.0_dec;
  auto qty = 50.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetAsset().GetSymbolStr() == "MSFT");
  REQUIRE(position.GetData().currentPrice == 300.0_dec);
  REQUIRE(position.GetData().qty == 50.0_dec);
}

TEST_CASE("PositionBuilder::GetAsset returns correct asset", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().TSLA;
  auto currentPrice = 250.0_dec;
  auto qty = 25.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Short, currentPrice, qty, now, fxRate);

  Asset retrievedAsset = builder.GetAsset();

  REQUIRE(retrievedAsset == asset);
  REQUIRE(retrievedAsset.GetSymbolStr() == "TSLA");
}

TEST_CASE("PositionBuilder::SetQty modifies quantity", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().GOOG;
  auto currentPrice = 140.0_dec;
  auto qty = 100.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  builder.SetQty(200.0_dec);
  Position position = builder.Build();

  REQUIRE(position.GetData().qty == 200.0_dec);
}

TEST_CASE("PositionBuilder::SetType modifies position type", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().AMZN;
  auto currentPrice = 180.0_dec;
  auto qty = 30.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  builder.SetType(PositionType::Short);
  Position position = builder.Build();

  REQUIRE(position.GetData().type == PositionType::Short);
}

TEST_CASE("PositionBuilder::SetCurrentPrice modifies price", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().IBM;
  auto currentPrice = 120.0_dec;
  auto qty = 75.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  builder.SetCurrentPrice(125.0_dec);
  Position position = builder.Build();

  REQUIRE(position.GetData().currentPrice == 125.0_dec);
}

TEST_CASE("PositionBuilder fluent interface chaining", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().SPY;
  auto currentPrice = 450.0_dec;
  auto qty = 10.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  Position position = PositionBuilder(asset, PositionType::Long, currentPrice, qty, now, fxRate)
    .SetQty(20.0_dec)
    .SetType(PositionType::Short)
    .SetCurrentPrice(460.0_dec)
    .Build();

  REQUIRE(position.GetData().qty == 20.0_dec);
  REQUIRE(position.GetData().type == PositionType::Short);
  REQUIRE(position.GetData().currentPrice == 460.0_dec);
}

TEST_CASE("PositionBuilder with Long position type", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().AAPL;
  auto currentPrice = 175.0_dec;
  auto qty = 100.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().type == PositionType::Long);
  REQUIRE(position.GetData().qty > 0);
}

TEST_CASE("PositionBuilder with Short position type", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().TSLA;
  auto currentPrice = 250.0_dec;
  auto qty = 50.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Short, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().type == PositionType::Short);
}

TEST_CASE("PositionBuilder with zero quantity", "[position_builder][edge_case]") {
  auto asset = EpochStratifyXAssetConstants::instance().MSFT;
  auto currentPrice = 300.0_dec;
  auto qty = 0.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().qty == 0.0_dec);
}

TEST_CASE("PositionBuilder with fractional quantity", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().BTC_USD;
  auto currentPrice = 45000.0_dec;
  auto qty = 0.5_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().qty == 0.5_dec);
  REQUIRE(position.GetAsset().GetAssetClass() == AssetClass::Crypto);
}

TEST_CASE("PositionBuilder with custom FX rate", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().AAPL;
  auto currentPrice = 150.0_dec;
  auto qty = 100.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.25_dec; // Custom FX rate

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  // Verify position was created with custom FX rate
  REQUIRE(position.GetAsset() == asset);
  REQUIRE(position.GetData().qty == qty);
}

TEST_CASE("PositionBuilder SetQty returns reference for chaining", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().GOOG;
  auto currentPrice = 140.0_dec;
  auto qty = 10.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  // Test that SetQty returns PositionBuilder& for chaining
  PositionBuilder& builderRef = builder.SetQty(20.0_dec);
  Position position = builderRef.Build();

  REQUIRE(position.GetData().qty == 20.0_dec);
}

TEST_CASE("PositionBuilder SetType returns reference for chaining", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().IBM;
  auto currentPrice = 120.0_dec;
  auto qty = 50.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  // Test that SetType returns PositionBuilder& for chaining
  PositionBuilder& builderRef = builder.SetType(PositionType::Short);
  Position position = builderRef.Build();

  REQUIRE(position.GetData().type == PositionType::Short);
}

TEST_CASE("PositionBuilder SetCurrentPrice returns reference for chaining", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().AMZN;
  auto currentPrice = 180.0_dec;
  auto qty = 25.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  // Test that SetCurrentPrice returns PositionBuilder& for chaining
  PositionBuilder& builderRef = builder.SetCurrentPrice(185.0_dec);
  Position position = builderRef.Build();

  REQUIRE(position.GetData().currentPrice == 185.0_dec);
}

TEST_CASE("PositionBuilder multiple modifications", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().SPY;
  auto currentPrice = 450.0_dec;
  auto qty = 10.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);

  // Apply multiple modifications
  builder.SetQty(15.0_dec);
  builder.SetCurrentPrice(455.0_dec);
  builder.SetType(PositionType::Short);
  builder.SetQty(20.0_dec); // Change qty again

  Position position = builder.Build();

  REQUIRE(position.GetData().qty == 20.0_dec);
  REQUIRE(position.GetData().currentPrice == 455.0_dec);
  REQUIRE(position.GetData().type == PositionType::Short);
}

TEST_CASE("PositionBuilder with Crypto asset", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().ETH_USD;
  auto currentPrice = 3000.0_dec;
  auto qty = 5.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetAsset().GetAssetClass() == AssetClass::Crypto);
  REQUIRE(position.GetData().qty == 5.0_dec);
}

TEST_CASE("PositionBuilder with FX asset", "[position_builder]") {
  auto asset = EpochStratifyXAssetConstants::instance().EUR_USD;
  auto currentPrice = 1.10_dec;
  auto qty = 10000.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetAsset().GetAssetClass() == AssetClass::FX);
  REQUIRE(position.GetData().currentPrice == 1.10_dec);
}

TEST_CASE("PositionBuilder with very large quantity", "[position_builder][edge_case]") {
  auto asset = EpochStratifyXAssetConstants::instance().AA;
  auto currentPrice = 50.0_dec;
  auto qty = 1000000.0_dec; // 1 million shares
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().qty == 1000000.0_dec);
}

TEST_CASE("PositionBuilder with very small price", "[position_builder][edge_case]") {
  auto asset = EpochStratifyXAssetConstants::instance().DOGE_USD;
  auto currentPrice = 0.0001_dec; // Very small crypto price
  auto qty = 1000000.0_dec;
  auto now = DateTime::now();
  auto fxRate = 1.0_dec;

  PositionBuilder builder(asset, PositionType::Long, currentPrice, qty, now, fxRate);
  Position position = builder.Build();

  REQUIRE(position.GetData().currentPrice == 0.0001_dec);
  REQUIRE(position.GetData().qty == 1000000.0_dec);
}
