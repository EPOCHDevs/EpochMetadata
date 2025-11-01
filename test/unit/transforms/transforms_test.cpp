
#include "epochflow/strategy/registration.h"
#include <catch.hpp>
#include <epochflow/core/constants.h>
#include <epochflow/core/bar_attribute.h>
#include <epochflow/transforms/core/registration.h>
#include <epochflow/transforms/core/transform_definition.h>
#include <unordered_map>

using namespace epochflow;
using Catch::Approx;

TEST_CASE("Transform Definition") {

  SECTION("TransformDefinition Constructor and Basic Methods") {
    // Create metadata to avoid registry lookup
    epochflow::transforms::TransformsMetaData metadata;
    metadata.id = "example_type";

    TransformDefinitionData data{
        .type = "example_type",
        .id = "1234",
        .options = {},
        .timeframe = epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY,
        .inputs = {{"input1", {"value1"}}},
        .metaData = metadata};

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
                          epochflow::MetaDataOptionDefinition{3.14});
      REQUIRE(epochflow::MetaDataOptionDefinition{
                  transform.GetOptions().at("key1")}
                  .GetDecimal() == Approx(3.14));

      transform.SetOption("key2",
                          epochflow::MetaDataOptionDefinition{42.0});
      REQUIRE(epochflow::MetaDataOptionDefinition{
                  transform.GetOptions().at("key2")}
                  .GetInteger() == Approx(42));
    }

    SECTION("SetPeriod and SetPeriods") {
      transform.SetPeriod(10);
      REQUIRE(epochflow::MetaDataOptionDefinition{
                  transform.GetOptions().at("period")}
                  .GetInteger() == Approx(10));

      transform.SetPeriods(20);
      REQUIRE(epochflow::MetaDataOptionDefinition{
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
                          epochflow::MetaDataOptionDefinition{7.5});
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
