
#include "epoch_metadata/strategy/registration.h"
#include <catch.hpp>
#include "epoch_metadata/constants.h"
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/transforms/registration.h"
#include "epoch_metadata/transforms/transform_definition.h"
#include <unordered_map>

using namespace epoch_metadata;
using Catch::Approx;

TEST_CASE("Transform Definition") {

  SECTION("TransformDefinition Constructor and Basic Methods") {
    TransformDefinitionData data{
        .type = "example_type",
        .id = "1234",
        .options = {},
        .timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY,
        .inputs = {{"input1", {"value1"}}}};

    TransformDefinition transform(data);

    SECTION("Constructor initializes correctly") {
      REQUIRE(transform.GetType() == "example_type");
      REQUIRE(transform.GetId() == "1234");
      REQUIRE(transform.GetTimeframe().ToString() == "1D");
      REQUIRE(transform.GetInputs().at("input1") ==
              std::vector<std::string>{"value1"});
    }

    SECTION("SetOption updates options correctly") {
      transform.SetOption("key1",
                          epoch_metadata::MetaDataOptionDefinition{3.14});
      REQUIRE(epoch_metadata::MetaDataOptionDefinition{
                  transform.GetOptions().at("key1")}
                  .GetDecimal() == Approx(3.14));

      transform.SetOption("key2",
                          epoch_metadata::MetaDataOptionDefinition{42.0});
      REQUIRE(epoch_metadata::MetaDataOptionDefinition{
                  transform.GetOptions().at("key2")}
                  .GetInteger() == Approx(42));
    }

    SECTION("SetPeriod and SetPeriods") {
      transform.SetPeriod(10);
      REQUIRE(epoch_metadata::MetaDataOptionDefinition{
                  transform.GetOptions().at("period")}
                  .GetInteger() == Approx(10));

      transform.SetPeriods(20);
      REQUIRE(epoch_metadata::MetaDataOptionDefinition{
                  transform.GetOptions().at("periods")}
                  .GetInteger() == Approx(20));
    }

    SECTION("SetType methods") {
      transform.SetType("new_type");
      REQUIRE(transform.GetType() == "new_type");

      TransformDefinition copy = transform.SetTypeCopy("copied_type");
      REQUIRE(copy.GetType() == "copied_type");
      REQUIRE(transform.GetType() == "new_type");

      transform.SetTypeIfEmpty("should_not_change");
      REQUIRE(transform.GetType() == "new_type");
    }

    SECTION("SetInput creates a copy with new inputs") {
      InputMapping newInputs = {{"new_input", {"new_value"}}};
      TransformDefinition copy = transform.SetInput(newInputs);
      REQUIRE(copy.GetInputs().at("new_input") ==
              std::vector<std::string>{"new_value"});
      REQUIRE(transform.GetInputs().at("input1") ==
              std::vector<std::string>{"value1"});
    }

    SECTION("GetOptionAsDouble with and without fallback") {
      transform.SetOption("double_key",
                          epoch_metadata::MetaDataOptionDefinition{7.5});
      REQUIRE(transform.GetOptionAsDouble("double_key") == Approx(7.5));
      REQUIRE(transform.GetOptionAsDouble("missing_key", 1.5) == Approx(1.5));
    }
  }

  SECTION("TransformDefinition Constructor with Descriptor") {
    YAML::Node node;
    node["id"] = "1234";
    node["tag"] = "example_tag";
    node["type"] = "sma";
    node["timeframe"]["interval"] = 1;
    node["timeframe"]["type"] = "day";
    node["options"]["period"] = 5;
    node["inputs"]["SLOT"] = "value1";

    TransformDefinition transform(node);

    SECTION("Constructor initializes correctly from descriptor") {
      REQUIRE(transform.GetType() == "sma");
      REQUIRE(transform.GetId() == "1234");
      REQUIRE(transform.GetTimeframe().ToString() == "1D");
      REQUIRE(transform.GetInputs().at("SLOT") ==
              std::vector<std::string>{"value1"});
    }
  }
}
