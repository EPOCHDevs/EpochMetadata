//
// Created by adesola on 1/30/25.
//
#include "data/factory.h"
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/model/asset/asset_specification.hpp>
#include <epoch_data_sdk/model/asset/index_constituents.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace epoch_script::data;
using namespace epoch_script::asset;
using namespace epoch_core;

TEST_CASE("MakeAssets expands index to constituents", "[factory_index]") {
  SECTION("Index ID without dash expands to constituents") {
    // Use AEX25 (without dash) which should trigger expansion
    std::vector<std::string> assetIds = {"AEX25"};

    auto [dataloaderAssets, strategyAssets, continuationAssets] =
        factory::MakeAssets(CountryCurrency::USD, assetIds, false);

    // Check that constituents were loaded
    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto constituents = indexDB.GetConstituents("AEX25");

    if (constituents.has_value() && !constituents->empty()) {
      // Strategy assets should contain constituents, not the index itself
      CHECK(strategyAssets.size() == constituents->size());

      // Verify constituents ARE in the sets
      CHECK(!dataloaderAssets.empty());
      CHECK(!strategyAssets.empty());
    } else {
      WARN("AEX25 has no constituents in database");
    }
  }

  SECTION("Non-index asset works as before") {
    std::vector<std::string> assetIds = {"AAPL-Stocks"};

    auto [dataloaderAssets, strategyAssets, continuationAssets] =
        factory::MakeAssets(CountryCurrency::USD, assetIds, false);

    // Should have exactly one asset
    CHECK(dataloaderAssets.size() == 1);
    CHECK(strategyAssets.size() == 1);

    // Verify it's the AAPL asset
    bool hasAAPL = std::any_of(
        dataloaderAssets.begin(), dataloaderAssets.end(),
        [](const Asset &a) { return a.GetID() == "AAPL-Stocks"; });
    CHECK(hasAAPL);
  }

  SECTION("Mixed assets (index and regular) work correctly") {
    std::vector<std::string> assetIds = {"AAPL-Stocks", "AEX25"};

    auto [dataloaderAssets, strategyAssets, continuationAssets] =
        factory::MakeAssets(CountryCurrency::USD, assetIds, false);

    // Should have AAPL plus AEX25 constituents
    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto constituents = indexDB.GetConstituents("AEX25");

    if (constituents.has_value()) {
      size_t expectedSize = 1 + constituents->size();  // AAPL + constituents
      CHECK(strategyAssets.size() == expectedSize);

      // Verify AAPL is present
      bool hasAAPL = std::any_of(
          strategyAssets.begin(), strategyAssets.end(),
          [](const Asset &a) { return a.GetID() == "AAPL-Stocks"; });
      CHECK(hasAAPL);
    }
  }

  SECTION("Asset ID with dash does NOT trigger index expansion") {
    // Any ID with dash should be treated as normal asset (not checked for index expansion)
    // This verifies the dash check works correctly
    std::vector<std::string> assetIds = {"AAPL-Stocks"};

    auto [dataloaderAssets, strategyAssets, continuationAssets] =
        factory::MakeAssets(CountryCurrency::USD, assetIds, false);

    // Should create the asset directly without any index expansion attempt
    CHECK(dataloaderAssets.size() == 1);
    CHECK(strategyAssets.size() == 1);

    bool hasAAPL = std::any_of(
        strategyAssets.begin(), strategyAssets.end(),
        [](const Asset &a) { return a.GetID() == "AAPL-Stocks"; });
    CHECK(hasAAPL);
  }

  SECTION("Multiple indices expand correctly") {
    // Use two indices without dashes
    std::vector<std::string> assetIds = {"AEX25", "DJIA30"};

    auto [dataloaderAssets, strategyAssets, continuationAssets] =
        factory::MakeAssets(CountryCurrency::USD, assetIds, false);

    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto aex25Constituents = indexDB.GetConstituents("AEX25");
    auto djiaConstituents = indexDB.GetConstituents("DJIA30");

    if (aex25Constituents.has_value() && djiaConstituents.has_value()) {
      size_t expectedSize = aex25Constituents->size() + djiaConstituents->size();
      CHECK(strategyAssets.size() == expectedSize);
    }
  }
}

TEST_CASE("MakeAssets constituent assets added to correct sets", "[factory_index]") {
  std::vector<std::string> assetIds = {"ASX200"};

  auto [dataloaderAssets, strategyAssets, continuationAssets] =
      factory::MakeAssets(CountryCurrency::USD, assetIds, false);

  const auto &indexDB = IndexConstituentsDatabase::GetInstance();
  auto constituents = indexDB.GetConstituents("ASX200");

  if (constituents.has_value() && !constituents->empty()) {
    SECTION("Constituents added to dataloaderAssets") {
      CHECK(dataloaderAssets.size() == constituents->size());

      // Pick a known constituent and verify it's there
      bool hasBHP = std::any_of(
          dataloaderAssets.begin(), dataloaderAssets.end(),
          [](const Asset &a) { return a.GetID() == "BHP-Stocks"; });
      if (constituents->size() > 50) {
        CHECK(hasBHP);
      }
    }

    SECTION("Constituents added to strategyAssets") {
      CHECK(strategyAssets.size() == constituents->size());

      // Verify same constituents in strategy assets
      bool hasBHP = std::any_of(
          strategyAssets.begin(), strategyAssets.end(),
          [](const Asset &a) { return a.GetID() == "BHP-Stocks"; });
      if (constituents->size() > 50) {
        CHECK(hasBHP);
      }
    }

    SECTION("No continuation assets for indices") {
      // Indices themselves don't create continuation assets
      CHECK(continuationAssets.empty());
    }
  }
}
