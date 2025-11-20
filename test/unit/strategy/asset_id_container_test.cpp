//
// Unit tests for AssetIDContainer
//

#include <epoch_script/strategy/asset_id_container.h>
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/model/asset/asset_specification.hpp>
#include <epoch_data_sdk/model/asset/index_constituents.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glaze/glaze.hpp>

using namespace epoch_script::strategy;
using namespace data_sdk::asset;

TEST_CASE("AssetIDContainer basic construction", "[asset_id_container]") {
  SECTION("Default constructor") {
    AssetIDContainer container;
    CHECK(container.raw_asset_ids.empty());
  }

  SECTION("Vector constructor") {
    std::vector<std::string> ids = {"AAPL-Stocks", "MSFT-Stocks"};
    AssetIDContainer container(ids);
    CHECK(container.raw_asset_ids.size() == 2);
    CHECK(container.raw_asset_ids[0] == "AAPL-Stocks");
    CHECK(container.raw_asset_ids[1] == "MSFT-Stocks");
  }

  SECTION("Initializer list constructor") {
    AssetIDContainer container({"AAPL-Stocks", "MSFT-Stocks"});
    CHECK(container.raw_asset_ids.size() == 2);
    CHECK(container.raw_asset_ids[0] == "AAPL-Stocks");
    CHECK(container.raw_asset_ids[1] == "MSFT-Stocks");
  }
}

TEST_CASE("AssetIDContainer resolves regular assets", "[asset_id_container]") {
  SECTION("Single stock asset") {
    AssetIDContainer container({"AAPL-Stocks"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 1);
    CHECK(resolved[0] == "AAPL-Stocks");
  }

  SECTION("Multiple stock assets") {
    AssetIDContainer container({"AAPL-Stocks", "MSFT-Stocks", "GOOGL-Stocks"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 3);
    // Assets should be validated and returned
    CHECK(std::find(resolved.begin(), resolved.end(), "AAPL-Stocks") != resolved.end());
    CHECK(std::find(resolved.begin(), resolved.end(), "MSFT-Stocks") != resolved.end());
    CHECK(std::find(resolved.begin(), resolved.end(), "GOOGL-Stocks") != resolved.end());
  }

  SECTION("Duplicate assets are deduplicated") {
    AssetIDContainer container({"AAPL-Stocks", "AAPL-Stocks", "MSFT-Stocks"});
    auto resolved = container.Resolve();

    // Should only have 2 unique assets
    REQUIRE(resolved.size() == 2);
    CHECK(std::find(resolved.begin(), resolved.end(), "AAPL-Stocks") != resolved.end());
    CHECK(std::find(resolved.begin(), resolved.end(), "MSFT-Stocks") != resolved.end());
  }
}

TEST_CASE("AssetIDContainer handles FX/Crypto prefix", "[asset_id_container]") {
  SECTION("FX asset with dash - prepends ^") {
    AssetIDContainer container({"EURUSD-FX"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 1);
    // Should prepend ^ for FX assets
    CHECK(resolved[0].starts_with("^"));
  }

  SECTION("Crypto asset with dash - prepends ^") {
    AssetIDContainer container({"BTCUSD-Crypto"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 1);
    // Should prepend ^ for Crypto assets
    CHECK(resolved[0].starts_with("^"));
  }

  SECTION("FX asset already with ^ - no double prefix") {
    AssetIDContainer container({"^EURUSD-FX"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 1);
    // Should not double-prefix
    CHECK(resolved[0].starts_with("^"));
    CHECK(resolved[0].find("^^") == std::string::npos);
  }
}

TEST_CASE("AssetIDContainer expands index constituents", "[asset_id_container]") {
  SECTION("Index without dash expands to constituents") {
    AssetIDContainer container({"DJIA30"});
    auto resolved = container.Resolve();

    // Check that constituents were loaded
    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto constituents = indexDB.GetConstituents("DJIA30");

    if (constituents.has_value() && !constituents->empty()) {
      // Should expand to constituent count
      CHECK(resolved.size() == constituents->size());

      // The index itself should not be in the resolved list
      CHECK(std::find(resolved.begin(), resolved.end(), "DJIA30") == resolved.end());

      // All resolved IDs should be valid assets
      for (const auto& id : resolved) {
        CHECK_NOTHROW(MakeAsset(AssetSpecificationQuery{id}));
      }
    } else {
      WARN("DJIA30 has no constituents in database");
    }
  }

  SECTION("Multiple indices expand correctly") {
    AssetIDContainer container({"DJIA30", "AEX25"});
    auto resolved = container.Resolve();

    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto djia30Constituents = indexDB.GetConstituents("DJIA30");
    auto aex25Constituents = indexDB.GetConstituents("AEX25");

    if (djia30Constituents.has_value() && aex25Constituents.has_value()) {
      size_t expectedSize = djia30Constituents->size() + aex25Constituents->size();
      CHECK(resolved.size() == expectedSize);
    }
  }

  SECTION("Index ID with dash does NOT expand") {
    // IDs with dash are treated as regular assets, not indices
    AssetIDContainer container({"SOME-INDEX"});

    // This should fail because "SOME-INDEX" is not a valid asset
    CHECK_THROWS(container.Resolve());
  }
}

TEST_CASE("AssetIDContainer mixed assets", "[asset_id_container]") {
  SECTION("Mix of regular assets and indices") {
    AssetIDContainer container({"AAPL-Stocks", "DJIA30"});
    auto resolved = container.Resolve();

    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto constituents = indexDB.GetConstituents("DJIA30");

    if (constituents.has_value()) {
      // Check if AAPL is already in DJIA30
      bool aaplInDjia = std::find(constituents->begin(), constituents->end(), "AAPL-Stocks") != constituents->end();

      size_t expectedSize = aaplInDjia ? constituents->size() : (1 + constituents->size());
      CHECK(resolved.size() == expectedSize);

      // AAPL should be in the list
      CHECK(std::find(resolved.begin(), resolved.end(), "AAPL-Stocks") != resolved.end());
    }
  }

  SECTION("Mix of FX and regular assets") {
    AssetIDContainer container({"AAPL-Stocks", "EURUSD-FX"});
    auto resolved = container.Resolve();

    REQUIRE(resolved.size() == 2);

    // AAPL should be unchanged
    CHECK(std::find(resolved.begin(), resolved.end(), "AAPL-Stocks") != resolved.end());

    // EURUSD should have ^ prefix
    bool hasPrefixedFX = std::any_of(resolved.begin(), resolved.end(),
        [](const std::string& id) { return id.starts_with("^") && id.find("EURUSD") != std::string::npos; });
    CHECK(hasPrefixedFX);
  }
}

TEST_CASE("AssetIDContainer validates asset IDs", "[asset_id_container]") {
  SECTION("Invalid asset ID throws exception") {
    AssetIDContainer container({"INVALID_ASSET_ID"});

    // Should throw because this is not a valid asset and not an index
    CHECK_THROWS_AS(container.Resolve(), std::runtime_error);
  }

  SECTION("Valid assets do not throw") {
    AssetIDContainer container({"AAPL-Stocks", "MSFT-Stocks"});

    CHECK_NOTHROW(container.Resolve());
  }

  SECTION("Error message contains invalid asset ID") {
    AssetIDContainer container({"INVALID_ASSET_ID"});

    try {
      container.Resolve();
      FAIL("Should have thrown exception");
    } catch (const std::runtime_error& e) {
      std::string errorMsg = e.what();
      CHECK(errorMsg.find("INVALID_ASSET_ID") != std::string::npos);
    }
  }
}

TEST_CASE("AssetIDContainer serialization with glaze", "[asset_id_container]") {
  SECTION("Serialize to JSON") {
    AssetIDContainer container({"AAPL-Stocks", "MSFT-Stocks"});

    auto json_result = glz::write_json(container);
    REQUIRE(json_result.has_value());
    std::string json = json_result.value();
    CHECK_FALSE(json.empty());

    // Should serialize as a string array, not an object
    CHECK(json.starts_with("["));
    CHECK(json.ends_with("]"));
    CHECK(json.find("AAPL-Stocks") != std::string::npos);
    CHECK(json.find("MSFT-Stocks") != std::string::npos);
  }

  SECTION("Deserialize from JSON") {
    std::string json = R"(["AAPL-Stocks","MSFT-Stocks"])";

    AssetIDContainer container;
    auto result = glz::read_json(container, json);

    REQUIRE_FALSE(result);  // error_ctx is falsy when successful
    CHECK(container.raw_asset_ids.size() == 2);
    CHECK(container.raw_asset_ids[0] == "AAPL-Stocks");
    CHECK(container.raw_asset_ids[1] == "MSFT-Stocks");
  }

  SECTION("Round-trip serialization") {
    AssetIDContainer original({"AAPL-Stocks", "MSFT-Stocks", "GOOGL-Stocks"});

    // Serialize
    auto json_result = glz::write_json(original);
    REQUIRE(json_result.has_value());
    std::string json = json_result.value();

    // Deserialize
    AssetIDContainer deserialized;
    auto result = glz::read_json(deserialized, json);

    REQUIRE_FALSE(result);  // error_ctx is falsy when successful
    CHECK(deserialized.raw_asset_ids == original.raw_asset_ids);
  }
}

TEST_CASE("AssetIDContainer edge cases", "[asset_id_container]") {
  SECTION("Empty container") {
    AssetIDContainer container;
    auto resolved = container.Resolve();

    CHECK(resolved.empty());
  }

  SECTION("Resolving constituent that's also explicitly added") {
    // If an index contains an asset that's also explicitly listed,
    // it should only appear once in the resolved list
    const auto &indexDB = IndexConstituentsDatabase::GetInstance();
    auto constituents = indexDB.GetConstituents("DJIA30");

    if (constituents.has_value() && !constituents->empty()) {
      // Get first constituent
      std::string firstConstituent = *constituents->begin();

      AssetIDContainer container({firstConstituent, "DJIA30"});
      auto resolved = container.Resolve();

      // Count occurrences of first constituent
      int count = std::count(resolved.begin(), resolved.end(), firstConstituent);
      CHECK(count == 1);
    }
  }
}
