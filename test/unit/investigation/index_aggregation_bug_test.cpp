#include <catch2/catch_all.hpp>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/series.h>
#include <epoch_frame/scalar.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/compute/api_aggregate.h>
#include <iostream>
#include "test_helpers.h"

using namespace std::chrono;
using namespace investigation_test;

TEST_CASE("Index aggregation with string columns - reproduce bug", "[investigation][index][bug]") {
  SECTION("String scalar with index function - NOW WORKS") {
    // Create a string series like break_label in the failing test
    auto index = make_date_range(0, 5);
    auto series = make_string_series(index, {
      "Other",
      "Broke High",
      "Other",
      "Broke Low",
      "Broke High"
    });

    // Attempt to use index aggregation with string scalar (this is what the bug does)
    std::string target = "Broke High";

    INFO("Series dtype: " << series.dtype()->ToString());
    INFO("Looking for: " << target);

    // This is what base_card_report.cpp:121 does - creates a string scalar
    auto arrowScalar = arrow::MakeScalar(target);
    INFO("Created scalar type: " << arrowScalar->type->ToString());

    arrow::compute::IndexOptions indexOptions(arrowScalar);
    auto result = series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions);

    INFO("Result: " << result.repr());

    // String scalar with string array now works! The behavior changed.
    // Should find first occurrence at index 1
    REQUIRE(result.is_valid());
    REQUIRE(result.as_int64() == 1);
  }

  SECTION("Binary scalar with index function - KNOWN LIMITATION - FAILS") {
    // Create the same string series
    auto index = make_date_range(0, 5);
    auto series = make_string_series(index, {
      "Other",
      "Broke High",
      "Other",
      "Broke Low",
      "Broke High"
    });

    std::string target = "Broke High";

    INFO("Series dtype: " << series.dtype()->ToString());
    INFO("Looking for: " << target);

    // This is a known limitation: binary scalar doesn't work with string arrays
    // Arrow requires exact type matching (string scalar for string arrays, not binary)
    auto buffer = arrow::Buffer::FromString(target);
    auto binaryScalar = std::make_shared<arrow::BinaryScalar>(buffer);

    INFO("Created binary scalar type: " << binaryScalar->type->ToString());

    arrow::compute::IndexOptions indexOptions(binaryScalar);

    // This should throw because binary != string type
    REQUIRE_THROWS_WITH(
      series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions),
      Catch::Matchers::ContainsSubstring("Type error")
    );
  }

  SECTION("Integer series with index function - baseline test") {
    // Test that index works fine with integer series
    auto index = make_date_range(0, 5);
    auto values = epoch_frame::factory::array::make_array<int64_t>({10, 20, 30, 20, 40});
    epoch_frame::Series series(index, values);

    // Look for value 20
    auto arrowScalar = arrow::MakeScalar(static_cast<int64_t>(20));
    arrow::compute::IndexOptions indexOptions(arrowScalar);
    auto result = series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions);

    INFO("Result for integer series: " << result.repr());

    // Should find first occurrence at index 1
    REQUIRE(result.is_valid());
    REQUIRE(result.as_int64() == 1);
  }

  SECTION("Double series with index function - baseline test") {
    // Test that index works fine with double series
    auto index = make_date_range(0, 5);
    auto values = epoch_frame::factory::array::make_array<double>({1.5, 2.5, 3.5, 2.5, 4.5});
    epoch_frame::Series series(index, values);

    // Look for value 2.5
    auto arrowScalar = arrow::MakeScalar(2.5);
    arrow::compute::IndexOptions indexOptions(arrowScalar);
    auto result = series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions);

    INFO("Result for double series: " << result.repr());

    // Should find first occurrence at index 1
    REQUIRE(result.is_valid());
    REQUIRE(result.as_int64() == 1);
  }
}

TEST_CASE("Investigate Arrow string vs binary types", "[investigation][arrow][types]") {
  SECTION("Understand string and binary scalar differences") {
    std::string test_value = "Broke High";

    // Create string scalar
    auto string_scalar = arrow::MakeScalar(test_value);
    INFO("String scalar type: " << string_scalar->type->ToString());
    INFO("String scalar type ID: " << string_scalar->type->id());

    // Create binary scalar
    auto buffer = arrow::Buffer::FromString(test_value);
    auto binary_scalar = std::make_shared<arrow::BinaryScalar>(buffer);
    INFO("Binary scalar type: " << binary_scalar->type->ToString());
    INFO("Binary scalar type ID: " << binary_scalar->type->id());

    // They should have different type IDs
    REQUIRE(string_scalar->type->id() == arrow::Type::STRING);
    REQUIRE(binary_scalar->type->id() == arrow::Type::BINARY);
  }

  SECTION("Test string array compatibility with scalars") {
    // Create a string array
    arrow::StringBuilder builder;
    REQUIRE(builder.Append("Other").ok());
    REQUIRE(builder.Append("Broke High").ok());
    REQUIRE(builder.Append("Other").ok());

    std::shared_ptr<arrow::Array> string_array;
    REQUIRE(builder.Finish(&string_array).ok());

    INFO("String array type: " << string_array->type()->ToString());
    INFO("String array type ID: " << string_array->type()->id());

    REQUIRE(string_array->type()->id() == arrow::Type::STRING);
  }

  SECTION("Test boolean array - THIS IS THE ACTUAL BUG!") {
    // Create a boolean array (this is what value_picks returns!)
    auto index = make_date_range(0, 5);

    arrow::BooleanBuilder bool_builder;
    REQUIRE(bool_builder.Append(false).ok());
    REQUIRE(bool_builder.Append(true).ok());
    REQUIRE(bool_builder.Append(false).ok());
    REQUIRE(bool_builder.Append(false).ok());
    REQUIRE(bool_builder.Append(true).ok());

    std::shared_ptr<arrow::Array> bool_array;
    REQUIRE(bool_builder.Finish(&bool_array).ok());

    INFO("Boolean array type: " << bool_array->type()->ToString());
    INFO("Boolean array type ID: " << bool_array->type()->id());

    REQUIRE(bool_array->type()->id() == arrow::Type::BOOL);

    // Now try to use index aggregation on a boolean series with a STRING target
    // This is what happens in the failing test!
    auto chunked_array = std::make_shared<arrow::ChunkedArray>(bool_array);
    epoch_frame::Series bool_series(index, chunked_array);

    std::string target = "Broke High";  // User passes a string!

    INFO("Attempting index on boolean series with string target");

    try {
      auto arrowScalar = arrow::MakeScalar(target);
      arrow::compute::IndexOptions indexOptions(arrowScalar);
      auto result = bool_series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions);

      FAIL("Should have failed with type error!");

    } catch (const std::exception& e) {
      std::string error_msg = e.what();
      INFO("Got expected error: " << error_msg);

      // This should be the binary type error!
      REQUIRE((error_msg.find("binary") != std::string::npos ||
               error_msg.find("bool") != std::string::npos));
    }
  }

  SECTION("Test large_string type - another possibility") {
    // Create a large_string array
    arrow::LargeStringBuilder large_builder;
    REQUIRE(large_builder.Append("Other").ok());
    REQUIRE(large_builder.Append("Broke High").ok());
    REQUIRE(large_builder.Append("Other").ok());

    std::shared_ptr<arrow::Array> large_string_array;
    REQUIRE(large_builder.Finish(&large_string_array).ok());

    INFO("Large string array type: " << large_string_array->type()->ToString());
    INFO("Large string array type ID: " << large_string_array->type()->id());

    REQUIRE(large_string_array->type()->id() == arrow::Type::LARGE_STRING);
  }
}

TEST_CASE("Test index with large_string arrays", "[investigation][index][large_string]") {
  SECTION("Large_string with string scalar - KNOWN LIMITATION") {
    // Create a large_string series
    auto index = make_date_range(0, 5);

    arrow::LargeStringBuilder large_builder;
    REQUIRE(large_builder.Append("Other").ok());
    REQUIRE(large_builder.Append("Broke High").ok());
    REQUIRE(large_builder.Append("Other").ok());
    REQUIRE(large_builder.Append("Broke Low").ok());
    REQUIRE(large_builder.Append("Broke High").ok());

    std::shared_ptr<arrow::Array> large_string_array;
    REQUIRE(large_builder.Finish(&large_string_array).ok());

    auto chunked_array = std::make_shared<arrow::ChunkedArray>(large_string_array);
    epoch_frame::Series series(index, chunked_array);

    std::string target = "Broke High";

    INFO("Series dtype: " << series.dtype()->ToString());
    INFO("Looking for: " << target);

    // This is a known limitation: string scalar doesn't work with large_string arrays
    // Arrow requires exact type matching (large_string scalar for large_string arrays)
    auto arrowScalar = arrow::MakeScalar(target);
    INFO("Created scalar type: " << arrowScalar->type->ToString());

    arrow::compute::IndexOptions indexOptions(arrowScalar);

    // String scalar (string type) doesn't match large_string array type
    REQUIRE_THROWS_WITH(
      series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions),
      Catch::Matchers::ContainsSubstring("Type error")
    );
  }
}