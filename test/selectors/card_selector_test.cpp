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
  // Timestamps in nanoseconds (original milliseconds * 1,000,000)
  std::vector<int64_t> timestamps = {1609459200000000000LL, 1609545600000000000LL, 1609632000000000000LL, 1609718400000000000LL};

  // Create index from timestamps
  auto index = factory::index::make_datetime_index(timestamps, "index","UTC");

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
    REQUIRE(metadata.name == "Card Selector");
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

    // Create transform config using helper function - pass object directly
    auto transformConfig = epoch_metadata::transform::card_selector_filter_cfg(
      "test_selector",
      schema,
      {"direction", "profit_pct", "is_signal"},
      epoch_metadata::TimeFrame(epoch_frame::factory::offset::days(1))
    );

    // Create selector
    CardSelectorFromFilter selector(transformConfig);

    // Execute transform - should return filtered DataFrame
    auto result = selector.TransformData(df);

    // Verify filtered DataFrame (only rows where is_signal is true)
    // Original has 4 rows: [true, true, false, true]
    // Filtered should have 3 rows (indices 0, 1, 3)
    REQUIRE(result.num_rows() == 3);
    REQUIRE(result.num_cols() == 4);  // direction, profit_pct, is_signal, index

    // Verify column names are preserved
    REQUIRE(result.contains("direction"));
    REQUIRE(result.contains("profit_pct"));
    REQUIRE(result.contains("is_signal"));

    // Verify data content - should have rows 0, 1, and 3 from original
    auto direction_col = result["direction"];
    REQUIRE(direction_col.iloc(0).repr() == "BUY");   // original row 0
    REQUIRE(direction_col.iloc(1).repr() == "SELL");  // original row 1
    REQUIRE(direction_col.iloc(2).repr() == "SELL");  // original row 3

    auto profit_col = result["profit_pct"];
    REQUIRE(profit_col.iloc(0).as_double() == Approx(10.5));   // original row 0
    REQUIRE(profit_col.iloc(1).as_double() == Approx(-5.2));   // original row 1
    REQUIRE(profit_col.iloc(2).as_double() == Approx(-8.1));   // original row 3

    auto is_signal_col = result["is_signal"];
    REQUIRE(is_signal_col.iloc(0).as_bool() == true);  // original row 0
    REQUIRE(is_signal_col.iloc(1).as_bool() == true);  // original row 1
    REQUIRE(is_signal_col.iloc(2).as_bool() == true);  // original row 3

    // Verify timestamps (index values in nanoseconds)
    REQUIRE(result["pivot"].iloc(0).timestamp().value == 1609459200000000000LL);  // original row 0
    REQUIRE(result["pivot"].iloc(1).timestamp().value == 1609545600000000000LL);  // original row 1
    REQUIRE(result["pivot"].iloc(2).timestamp().value == 1609718400000000000LL);  // original row 3
  }
}
