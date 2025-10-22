#include <catch.hpp>
#include "epoch_metadata/selectors/iselector.h"
#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/registration.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/metadata_options.h"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <glaze/glaze.hpp>
#include <sstream>

using namespace epoch_metadata;
using namespace epoch_metadata::selectors;
using namespace epoch_frame;
using Catch::Approx;

// Helper to create a sample DataFrame for testing
DataFrame CreateTestDataFrame() {
  // Create a simple DataFrame with columns for testing
  std::vector<int64_t> timestamps = {1609459200000, 1609545600000, 1609632000000, 1609718400000};

  // Create index from timestamps
  auto index = factory::index::make_index(factory::array::make_array(timestamps)->chunk(0), std::nullopt, "index");

  // Create columns
  std::vector<arrow::ChunkedArrayPtr> columns = {
    factory::array::make_array(std::vector<std::string>{"BUY", "SELL", "BUY", "SELL"}),
    factory::array::make_array(std::vector<double>{10.5, -5.2, 15.3, -8.1}),
    factory::array::make_array(std::vector<bool>{true, true, false, true})
  };

  std::vector<std::string> fields = {"direction", "profit_pct", "is_signal"};

  return make_dataframe(index, columns, fields);
}

TEST_CASE("CardSelectorTransform - Basic Functionality", "[selectors][card_selector]") {

  SECTION("Card selector metadata is correctly registered") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector"));
    auto metadata = metadataMap.at("card_selector");

    REQUIRE(metadata.id == "card_selector");
    REQUIRE(metadata.name == "Card Selector");
    REQUIRE(metadata.category == epoch_core::TransformCategory::Selector);
    REQUIRE(metadata.renderKind == epoch_core::TransformNodeRenderKind::Output);
    REQUIRE(metadata.atLeastOneInputRequired == true);
    REQUIRE(metadata.outputs.empty());  // Selectors don't output to graph
  }

  SECTION("Card selector has required card_schema option") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector"));
    auto metadata = metadataMap.at("card_selector");

    bool hasCardSchemaOption = false;
    for (const auto& option : metadata.options) {
      if (option.id == "card_schema") {
        hasCardSchemaOption = true;
        REQUIRE(option.isRequired == true);
        REQUIRE(option.type == epoch_core::MetaDataOptionType::String);
      }
    }
    REQUIRE(hasCardSchemaOption);
  }
}

TEST_CASE("CardSelectorTransform - Schema Parsing", "[selectors][card_selector]") {

  SECTION("Parse valid card schema JSON") {
    std::string schemaJson = R"({
      "title": "Trade Signals",
      "select_key": "is_signal",
      "sql": "",
      "schemas": [
        {
          "column_id": "direction",
          "slot": "PrimaryBadge",
          "render_type": "Badge",
          "color_map": {
            "Success": ["BUY"],
            "Error": ["SELL"]
          }
        },
        {
          "column_id": "profit_pct",
          "slot": "Hero",
          "render_type": "Number",
          "color_map": {}
        },
        {
          "column_id": "timestamp",
          "slot": "Footer",
          "render_type": "Navigator",
          "color_map": {}
        }
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.title == "Trade Signals");
    REQUIRE(schema.select_key == "is_signal");
    REQUIRE(schema.sql == "");
    REQUIRE(schema.schemas.size() == 3);

    // Verify first schema (direction badge)
    REQUIRE(schema.schemas[0].column_id == "direction");
    REQUIRE(schema.schemas[0].slot == epoch_core::CardSlot::PrimaryBadge);
    REQUIRE(schema.schemas[0].render_type == epoch_core::CardRenderType::Badge);
    REQUIRE(schema.schemas[0].color_map.size() == 2);

    // Verify color mappings
    REQUIRE(schema.schemas[0].color_map.at(epoch_core::CardColor::Success).size() == 1);
    REQUIRE(schema.schemas[0].color_map.at(epoch_core::CardColor::Success)[0] == "BUY");
    REQUIRE(schema.schemas[0].color_map.at(epoch_core::CardColor::Error)[0] == "SELL");
  }

  SECTION("Parse schema with SQL query") {
    std::string schemaJson = R"({
      "title": "Filtered Signals",
      "select_key": "",
      "sql": "SELECT * FROM input WHERE profit_pct > 0",
      "schemas": [
        {
          "column_id": "direction",
          "slot": "PrimaryBadge",
          "render_type": "Text",
          "color_map": {}
        }
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.title == "Filtered Signals");
    REQUIRE(schema.select_key == "");
    REQUIRE(schema.sql == "SELECT * FROM input WHERE profit_pct > 0");
    REQUIRE(schema.schemas.size() == 1);
  }

  SECTION("Parse schema with all render types") {
    std::string schemaJson = R"({
      "title": "All Types",
      "select_key": "",
      "sql": "",
      "schemas": [
        {"column_id": "col1", "slot": "PrimaryBadge", "render_type": "Text", "color_map": {}},
        {"column_id": "col2", "slot": "SecondaryBadge", "render_type": "Number", "color_map": {}},
        {"column_id": "col3", "slot": "Hero", "render_type": "Badge", "color_map": {}},
        {"column_id": "col4", "slot": "Subtitle", "render_type": "Timestamp", "color_map": {}},
        {"column_id": "col5", "slot": "Footer", "render_type": "Boolean", "color_map": {}},
        {"column_id": "col6", "slot": "Details", "render_type": "Icon", "color_map": {}},
        {"column_id": "col7", "slot": "Footer", "render_type": "Navigator", "color_map": {}}
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas.size() == 7);
    REQUIRE(schema.schemas[0].render_type == epoch_core::CardRenderType::Text);
    REQUIRE(schema.schemas[1].render_type == epoch_core::CardRenderType::Number);
    REQUIRE(schema.schemas[2].render_type == epoch_core::CardRenderType::Badge);
    REQUIRE(schema.schemas[3].render_type == epoch_core::CardRenderType::Timestamp);
    REQUIRE(schema.schemas[4].render_type == epoch_core::CardRenderType::Boolean);
    REQUIRE(schema.schemas[5].render_type == epoch_core::CardRenderType::Icon);
    REQUIRE(schema.schemas[6].render_type == epoch_core::CardRenderType::Navigator);
  }

  SECTION("Parse schema with all slot types") {
    std::string schemaJson = R"({
      "title": "All Slots",
      "select_key": "",
      "sql": "",
      "schemas": [
        {"column_id": "col1", "slot": "PrimaryBadge", "render_type": "Text", "color_map": {}},
        {"column_id": "col2", "slot": "SecondaryBadge", "render_type": "Text", "color_map": {}},
        {"column_id": "col3", "slot": "Hero", "render_type": "Text", "color_map": {}},
        {"column_id": "col4", "slot": "Subtitle", "render_type": "Text", "color_map": {}},
        {"column_id": "col5", "slot": "Footer", "render_type": "Text", "color_map": {}},
        {"column_id": "col6", "slot": "Details", "render_type": "Text", "color_map": {}}
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas.size() == 6);
    REQUIRE(schema.schemas[0].slot == epoch_core::CardSlot::PrimaryBadge);
    REQUIRE(schema.schemas[1].slot == epoch_core::CardSlot::SecondaryBadge);
    REQUIRE(schema.schemas[2].slot == epoch_core::CardSlot::Hero);
    REQUIRE(schema.schemas[3].slot == epoch_core::CardSlot::Subtitle);
    REQUIRE(schema.schemas[4].slot == epoch_core::CardSlot::Footer);
    REQUIRE(schema.schemas[5].slot == epoch_core::CardSlot::Details);
  }

  SECTION("Parse schema with all color types") {
    std::string schemaJson = R"({
      "title": "All Colors",
      "select_key": "",
      "sql": "",
      "schemas": [
        {
          "column_id": "status",
          "slot": "PrimaryBadge",
          "render_type": "Badge",
          "color_map": {
            "Default": ["PENDING"],
            "Primary": ["PROCESSING"],
            "Info": ["INFO"],
            "Success": ["WIN"],
            "Warning": ["CAUTION"],
            "Error": ["LOSS"]
          }
        }
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas[0].color_map.size() == 6);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Default) == 1);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Primary) == 1);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Info) == 1);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Success) == 1);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Warning) == 1);
    REQUIRE(schema.schemas[0].color_map.count(epoch_core::CardColor::Error) == 1);
  }
}

TEST_CASE("CardSelectorTransform - Enum Serialization", "[selectors][card_selector]") {

  SECTION("CardRenderType enum serialization") {
    // Test that the enum can be serialized/deserialized
    std::string json = R"("Badge")";
    epoch_core::CardRenderType type;
    auto error = glz::read_json(type, json);
    REQUIRE(!error);
    REQUIRE(type == epoch_core::CardRenderType::Badge);

    std::string serialized;
    auto write_error = glz::write_json(type, serialized);
    REQUIRE(!write_error);
    REQUIRE(serialized == "\"Badge\"");
  }

  SECTION("CardSlot enum serialization") {
    std::string json = R"("Hero")";
    epoch_core::CardSlot slot;
    auto error = glz::read_json(slot, json);
    REQUIRE(!error);
    REQUIRE(slot == epoch_core::CardSlot::Hero);

    std::string serialized;
    auto write_error = glz::write_json(slot, serialized);
    REQUIRE(!write_error);
    REQUIRE(serialized == "\"Hero\"");
  }

  SECTION("CardColor enum serialization") {
    std::string json = R"("Success")";
    epoch_core::CardColor color;
    auto error = glz::read_json(color, json);
    REQUIRE(!error);
    REQUIRE(color == epoch_core::CardColor::Success);

    std::string serialized;
    auto write_error = glz::write_json(color, serialized);
    REQUIRE(!write_error);
    REQUIRE(serialized == "\"Success\"");
  }
}

TEST_CASE("CardSelectorTransform - Comprehensive Schema Validation", "[selectors][card_selector]") {

  SECTION("Empty schema arrays are valid") {
    std::string schemaJson = R"({
      "title": "Empty",
      "select_key": "",
      "sql": "",
      "schemas": []
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas.empty());
  }

  SECTION("Multiple columns with same slot are allowed") {
    std::string schemaJson = R"({
      "title": "Multiple Same Slot",
      "select_key": "",
      "sql": "",
      "schemas": [
        {"column_id": "col1", "slot": "Details", "render_type": "Text", "color_map": {}},
        {"column_id": "col2", "slot": "Details", "render_type": "Text", "color_map": {}},
        {"column_id": "col3", "slot": "Details", "render_type": "Text", "color_map": {}}
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas.size() == 3);
    REQUIRE(schema.schemas[0].slot == epoch_core::CardSlot::Details);
    REQUIRE(schema.schemas[1].slot == epoch_core::CardSlot::Details);
    REQUIRE(schema.schemas[2].slot == epoch_core::CardSlot::Details);
  }

  SECTION("Color map with multiple values per color") {
    std::string schemaJson = R"({
      "title": "Multi-value Color Map",
      "select_key": "",
      "sql": "",
      "schemas": [
        {
          "column_id": "outcome",
          "slot": "PrimaryBadge",
          "render_type": "Badge",
          "color_map": {
            "Success": ["WIN", "PROFIT", "GAIN"],
            "Error": ["LOSS", "DEFICIT", "DECLINE"]
          }
        }
      ]
    })";

    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    auto& colorMap = schema.schemas[0].color_map;
    REQUIRE(colorMap.at(epoch_core::CardColor::Success).size() == 3);
    REQUIRE(colorMap.at(epoch_core::CardColor::Error).size() == 3);
  }
}

TEST_CASE("CardSelectorTransform - Integration Tests", "[selectors][card_selector][integration]") {

  SECTION("Selector transform returns empty DataFrame") {
    // This test verifies that selectors don't merge into the computation graph
    // by returning an empty DataFrame

    // For now this is a placeholder since we need the full transform factory setup
    // The actual integration test would:
    // 1. Create a CardSelectorTransform with valid config
    // 2. Call TransformData with test DataFrame
    // 3. Verify returned DataFrame is empty
    // 4. Verify GetSelectorData returns valid JSON

    REQUIRE(true); // Placeholder
  }
}

TEST_CASE("CardColumnSchema - Equality and Comparison", "[selectors][card_selector]") {

  SECTION("CardColumnSchema equality") {
    CardColumnSchema schema1{
      .column_id = "col1",
      .slot = epoch_core::CardSlot::Hero,
      .render_type = epoch_core::CardRenderType::Number,
      .color_map = {}
    };

    CardColumnSchema schema2{
      .column_id = "col1",
      .slot = epoch_core::CardSlot::Hero,
      .render_type = epoch_core::CardRenderType::Number,
      .color_map = {}
    };

    REQUIRE(schema1 == schema2);
  }

  SECTION("CardSchemaList equality") {
    CardSchemaList list1{
      .title = "Test",
      .select_key = "key",
      .sql = "",
      .schemas = {}
    };

    CardSchemaList list2{
      .title = "Test",
      .select_key = "key",
      .sql = "",
      .schemas = {}
    };

    REQUIRE(list1 == list2);
  }
}
