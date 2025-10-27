#include <catch.hpp>
#include "epoch_metadata/selectors/card_selector.h"
#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/registration.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/config_helper.h"
#include "epoch_metadata/metadata_options.h"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <glaze/glaze.hpp>
#include <sstream>

#include "epoch_metadata/transforms/registry.h"

using namespace epoch_metadata;
using namespace epoch_metadata::transform;
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

  SECTION("Card selector filter metadata is correctly registered") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector_filter"));
    auto metadata = metadataMap.at("card_selector_filter");

    REQUIRE(metadata.id == "card_selector_filter");
    REQUIRE(metadata.name == "Card Selector (Filter)");
    REQUIRE(metadata.category == epoch_core::TransformCategory::Selector);
    REQUIRE(metadata.atLeastOneInputRequired == true);
    REQUIRE(metadata.outputs.empty());  // Selectors don't output to graph
  }

  SECTION("Card selector SQL metadata is correctly registered") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector_sql"));
    auto metadata = metadataMap.at("card_selector_sql");

    REQUIRE(metadata.id == "card_selector_sql");
    REQUIRE(metadata.name == "Card Selector (SQL)");
    REQUIRE(metadata.category == epoch_core::TransformCategory::Selector);
    REQUIRE(metadata.atLeastOneInputRequired == true);
    REQUIRE(metadata.outputs.empty());  // Selectors don't output to graph
  }

  SECTION("Card selector filter has required card_schema option") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector_filter"));
    auto metadata = metadataMap.at("card_selector_filter");

    bool hasCardSchemaOption = false;
    for (const auto& option : metadata.options) {
      if (option.id == "card_schema") {
        hasCardSchemaOption = true;
        REQUIRE(option.isRequired == true);
        REQUIRE(option.type == epoch_core::MetaDataOptionType::CardSchema);
      }
    }
    REQUIRE(hasCardSchemaOption);
  }

  SECTION("Card selector SQL has required card_schema option") {
    auto& registry = transforms::ITransformRegistry::GetInstance();
    auto metadataMap = registry.GetMetaData();

    REQUIRE(metadataMap.contains("card_selector_sql"));
    auto metadata = metadataMap.at("card_selector_sql");

    bool hasCardSchemaOption = false;
    for (const auto& option : metadata.options) {
      if (option.id == "card_schema") {
        hasCardSchemaOption = true;
        REQUIRE(option.isRequired == true);
        REQUIRE(option.type == epoch_core::MetaDataOptionType::CardSchema);
      }
    }
    REQUIRE(hasCardSchemaOption);
  }
}

TEST_CASE("CardSchemaFilter - JSON Parsing", "[selectors][card_selector]") {

  SECTION("Parse schema with select_key") {
    std::string schemaJson = R"({
      "title": "Trade Signals",
      "select_key": "is_signal",
      "icon": "Info",
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
          "render_type": "Decimal",
          "color_map": {}
        },
        {
          "column_id": "timestamp",
          "slot": "Footer",
          "render_type": "Timestamp",
          "color_map": {}
        }
      ]
    })";

    CardSchemaFilter schema;
    auto error = glz::read_json(schema, schemaJson);

    if (error) {
      FAIL(glz::format_error(error, schemaJson));
    }
    REQUIRE(schema.title == "Trade Signals");
    REQUIRE(schema.select_key == "is_signal");
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

  SECTION("Parse schema with all render types") {
    std::string schemaJson = R"({
      "title": "All Types",
      "select_key": "filter_col",
      "schemas": [
        {"column_id": "col1", "slot": "PrimaryBadge", "render_type": "Text", "color_map": {}},
        {"column_id": "col2", "slot": "SecondaryBadge", "render_type": "Decimal", "color_map": {}},
        {"column_id": "col3", "slot": "Hero", "render_type": "Badge", "color_map": {}},
        {"column_id": "col4", "slot": "Subtitle", "render_type": "Timestamp", "color_map": {}},
        {"column_id": "col5", "slot": "Footer", "render_type": "Boolean", "color_map": {}}
      ]
    })";

    CardSchemaFilter schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.schemas.size() == 5);
    REQUIRE(schema.schemas[0].render_type == epoch_core::CardRenderType::Text);
    REQUIRE(schema.schemas[1].render_type == epoch_core::CardRenderType::Decimal);
    REQUIRE(schema.schemas[2].render_type == epoch_core::CardRenderType::Badge);
    REQUIRE(schema.schemas[3].render_type == epoch_core::CardRenderType::Timestamp);
    REQUIRE(schema.schemas[4].render_type == epoch_core::CardRenderType::Boolean);
  }
}

TEST_CASE("CardSchemaSQL - JSON Parsing", "[selectors][card_selector]") {

  SECTION("Parse schema with SQL query") {
    std::string schemaJson = R"({
      "title": "Filtered Signals",
      "sql": "SELECT * FROM self WHERE SLOT1 > 0",
      "schemas": [
        {
          "column_id": "direction",
          "slot": "PrimaryBadge",
          "render_type": "Text",
          "color_map": {}
        }
      ]
    })";

    CardSchemaSQL schema;
    auto error = glz::read_json(schema, schemaJson);

    REQUIRE(!error);
    REQUIRE(schema.title == "Filtered Signals");
    REQUIRE(schema.sql.GetSql() == "SELECT * FROM self WHERE SLOT1 > 0");
    REQUIRE(schema.schemas.size() == 1);
  }

  SECTION("Parse SQL schema with complex query") {
    std::string schemaJson = R"({
      "title": "Complex Filter",
      "sql": "SELECT * FROM self WHERE SLOT0 = 'BUY' AND SLOT1 > 10 ORDER BY SLOT1 DESC",
      "schemas": [
        {"column_id": "direction", "slot": "PrimaryBadge", "render_type": "Badge", "color_map": {}},
        {"column_id": "profit_pct", "slot": "Hero", "render_type": "Decimal", "color_map": {}}
      ]
    })";

    CardSchemaSQL schema;
    auto error = glz::read_json(schema, schemaJson);

    if(error) {
      FAIL(glz::format_error(error, schemaJson));
    }
    REQUIRE(schema.sql.GetSql().find("FROM self") != std::string::npos);
    REQUIRE(schema.sql.GetSql().find("WHERE") != std::string::npos);
    REQUIRE(schema.sql.GetSql().find("ORDER BY") != std::string::npos);
  }
}

TEST_CASE("CardColumnSchema - Equality and Comparison", "[selectors][card_selector]") {

  SECTION("CardColumnSchema equality") {
    CardColumnSchema schema1{
      .column_id = "col1",
      .slot = epoch_core::CardSlot::Hero,
      .render_type = epoch_core::CardRenderType::Decimal,
      .color_map = {}
    };

    CardColumnSchema schema2{
      .column_id = "col1",
      .slot = epoch_core::CardSlot::Hero,
      .render_type = epoch_core::CardRenderType::Decimal,
      .color_map = {}
    };

    REQUIRE(schema1 == schema2);
  }

  SECTION("CardSchemaFilter equality") {
    CardSchemaFilter list1{
      .title = "Test",
      .select_key = "key",
      .schemas = {}
    };

    CardSchemaFilter list2{
      .title = "Test",
      .select_key = "key",
      .schemas = {}
    };

    REQUIRE(list1 == list2);
  }

  SECTION("CardSchemaSQL equality") {
    CardSchemaSQL schema1{
      .title = "Test SQL",
      .sql = SqlStatement("SELECT * FROM self WHERE SLOT0 > 0"),
      .schemas = {}
    };

    CardSchemaSQL schema2{
      .title = "Test SQL",
      .sql = SqlStatement("SELECT * FROM self WHERE SLOT0 > 0"),
      .schemas = {}
    };

    REQUIRE(schema1 == schema2);
  }
}

TEST_CASE("CardSelectorFromFilter - Transform Functionality", "[selectors][card_selector]") {

  SECTION("Filter selector returns filtered DataFrame") {
    // Create test DataFrame
    auto df = CreateTestDataFrame();

    // Create CardSchemaFilter configuration
    CardSchemaFilter schema{
      .title = "Trade Signals",
      .select_key = "is_signal",
      .schemas = {
        CardColumnSchema{
          .column_id = "direction",
          .slot = epoch_core::CardSlot::PrimaryBadge,
          .render_type = epoch_core::CardRenderType::Badge,
          .color_map = {}
        }
      }
    };

    // Serialize schema to JSON
    std::string schemaJson = glz::write_json(schema).value_or("{}");

    // Create transform config using helper function
    auto transformConfig = epoch_metadata::transform::card_selector_filter_cfg(
      "test_selector",
      schemaJson,
      {"direction", "profit_pct", "is_signal"},
      epoch_metadata::TimeFrame(epoch_frame::factory::offset::days(1))
    );

    // Create selector
    CardSelectorFromFilter selector(transformConfig);

    // Execute transform - should return filtered DataFrame
    auto result = selector.TransformData(df);

    // Verify filtered DataFrame (only rows where is_signal is true)
    // Original has 4 rows, 3 have is_signal = true
    REQUIRE(result.num_rows() == 3);
    REQUIRE(result.num_cols() == 3);  // All columns preserved
  }
}

TEST_CASE("CardSelectorFromSQL - Transform Functionality", "[selectors][card_selector]") {

  SECTION("SQL selector returns filtered DataFrame") {
    // Create test DataFrame
    auto df = CreateTestDataFrame();

    // Create CardSchemaSQL configuration
    CardSchemaSQL schema{
      .title = "Profitable Trades",
      .sql = SqlStatement("SELECT * FROM self WHERE SLOT1 > 0"),
      .schemas = {
        CardColumnSchema{
          .column_id = "direction",
          .slot = epoch_core::CardSlot::PrimaryBadge,
          .render_type = epoch_core::CardRenderType::Badge,
          .color_map = {}
        }
      }
    };

    // Serialize schema to JSON
    std::string schemaJson = glz::write_json(schema).value_or("{}");

    // Create transform config using helper function
    auto transformConfig = epoch_metadata::transform::card_selector_sql_cfg(
      "test_selector_sql",
      schemaJson,
      {"direction", "profit_pct", "is_signal"},
      epoch_metadata::TimeFrame(epoch_frame::factory::offset::days(1))
    );

    // Create selector
    CardSelectorFromSQL selector(transformConfig);

    // Execute transform - should return filtered DataFrame
    // SLOT1 corresponds to profit_pct, filter for > 0 (rows with 10.5 and 15.3)
    auto result = selector.TransformData(df);

    // Verify filtered DataFrame
    REQUIRE(result.num_rows() == 2);  // Only 2 rows with profit > 0
    REQUIRE(result.num_cols() == 3);  // Columns renamed to SLOT0, SLOT1, SLOT2
  }

  SECTION("SQL selector with ORDER BY clause") {
    // Create test DataFrame
    auto df = CreateTestDataFrame();

    // Create CardSchemaSQL with ORDER BY
    CardSchemaSQL schema{
      .title = "Sorted Results",
      .sql = SqlStatement("SELECT * FROM self ORDER BY SLOT1 DESC"),
      .schemas = {}
    };

    // Serialize schema to JSON
    std::string schemaJson = glz::write_json(schema).value_or("{}");

    // Create transform config using helper function
    auto transformConfig = epoch_metadata::transform::card_selector_sql_cfg(
      "test_selector_ordered",
      schemaJson,
      {"direction", "profit_pct", "is_signal"},
      epoch_metadata::TimeFrame(epoch_frame::factory::offset::days(1))
    );

    // Create selector
    CardSelectorFromSQL selector(transformConfig);

    // Execute transform
    auto result = selector.TransformData(df);

    // Verify all rows returned (no filtering, just ordering)
    REQUIRE(result.num_rows() == 4);
    REQUIRE(result.num_cols() == 3);
  }
}
