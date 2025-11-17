#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/scalar.h>
#include <epoch_frame/series.h>
#include "test_helpers.h"

using namespace std::chrono_literals;
using namespace epoch_frame;
using namespace investigation_test;

// Test to isolate exactly where the null handling error occurs
TEST_CASE("fill_null with string replacement", "[investigation][fill_null]") {

  SECTION("Step 1: Test fill_null on string array with nulls") {
    INFO("Creating string array with nulls");

    std::vector<std::optional<std::string>> labels = {
      std::nullopt, std::nullopt, "A", "B", std::nullopt, "C"
    };

    auto index = make_date_range(0, 6);
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    auto df = make_dataframe_with_nullable_strings(index, labels, values, "label", "value");

    INFO("Getting label column as series");
    auto label_series = df["label"];
    auto label_array = label_series.contiguous_array();

    INFO("Array has " << label_array.null_count() << " nulls");
    REQUIRE(label_array.null_count() == 3);

    INFO("Attempting fill_null with string 'null'");
    epoch_frame::Scalar replacement(std::string("null"));

    INFO("Replacement scalar type: " << replacement.value()->type->ToString());

    // THIS IS THE CRITICAL TEST - does fill_null work?
    epoch_frame::Array filled;
    try {
      filled = label_array.fill_null(replacement);
      INFO("✅ fill_null SUCCEEDED");
      INFO("Filled array has " << filled.null_count() << " nulls");
      REQUIRE(filled.null_count() == 0);
    } catch (const std::exception& e) {
      FAIL("❌ fill_null FAILED with: " << e.what());
    }
  }

  SECTION("Step 2: Test fill_null then group_by_agg") {
    INFO("Creating dataframe with null labels");

    std::vector<std::optional<std::string>> labels = {
      std::nullopt, std::nullopt, "A", "B", std::nullopt, "C", "A", "B"
    };

    auto index = make_date_range(0, 8);
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    auto df = make_dataframe_with_nullable_strings(index, labels, values, "label", "value");

    INFO("Step 2a: fill_null on label column");
    auto label_series = df["label"];
    auto label_array = label_series.contiguous_array();

    epoch_frame::Scalar replacement(std::string("null"));
    epoch_frame::Array filled;

    try {
      filled = label_array.fill_null(replacement);
      INFO("✅ fill_null succeeded, now has " << filled.null_count() << " nulls");
    } catch (const std::exception& e) {
      FAIL("❌ fill_null failed: " << e.what());
    }

    INFO("Step 2b: Create new dataframe with filled label column");
    std::vector<arrow::ChunkedArrayPtr> arrays = {
      filled.as_chunked_array(),
      df["value"].array()
    };
    auto df_filled = epoch_frame::make_dataframe(df.index(), arrays, {"label", "value"});

    INFO("DataFrame created, attempting group_by_agg");

    // THIS IS THE SECOND CRITICAL TEST - does group_by_agg work with filled labels?
    try {
      auto grouped = df_filled.group_by_agg("label")
                              .agg("count")
                              .to_series();
      INFO("✅ group_by_agg SUCCEEDED");
      INFO("Grouped result has " << grouped.size() << " groups");

      // Print the groups to see if "null" is there
      for (int64_t i = 0; i < grouped.size(); ++i) {
        INFO("Group " << i << ": " << grouped.index()->at(i).repr() << " = " << grouped.iloc(i).repr());
      }

    } catch (const std::exception& e) {
      FAIL("❌ group_by_agg FAILED with: " << e.what());
    }
  }

  SECTION("Step 3: Test with different replacement strings") {
    auto index = make_date_range(0, 4);
    std::vector<std::optional<std::string>> labels = {
      std::nullopt, "A", std::nullopt, "B"
    };
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0};
    auto df = make_dataframe_with_nullable_strings(index, labels, values, "label", "value");

    std::vector<std::string> test_replacements = {"N/A", "UNKNOWN", "MISSING", "---"};

    for (const auto& repl_str : test_replacements) {
      INFO("Testing with replacement: '" << repl_str << "'");

      auto label_array = df["label"].contiguous_array();
      epoch_frame::Scalar replacement(repl_str);

      try {
        auto filled = label_array.fill_null(replacement);
        INFO("  fill_null OK");

        std::vector<arrow::ChunkedArrayPtr> arrays = {
          filled.as_chunked_array(),
          df["value"].array()
        };
        auto df_filled = epoch_frame::make_dataframe(df.index(), arrays, {"label", "value"});

        auto grouped = df_filled.group_by_agg("label").agg("count").to_series();
        INFO("  group_by_agg OK - " << grouped.size() << " groups");

      } catch (const std::exception& e) {
        FAIL("  ❌ FAILED with '" << repl_str << "': " << e.what());
      }
    }
  }
}
