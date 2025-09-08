//
// Created by adesola on 5/12/25.
//

#include <catch2/catch_test_macros.hpp>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include "epoch_metadata/constants.h"
#include "transforms/src/config_helper.h"
#include <transforms/trade_executors.h>

using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace epoch_frame;

namespace {
// Helper function to create test dataframes with double values
epoch_frame::DataFrame createTestDataFrame(const std::vector<double> &values) {
  auto index = epoch_frame::factory::index::make_datetime_index(
      std::vector<epoch_frame::DateTime>(
          values.size(), epoch_frame::DateTime{2020y, January, 18d}));
  return epoch_frame::make_dataframe<double>(index, {values}, {"signal"});
}

// Helper function to create standard 3-row index
epoch_frame::IndexPtr createStandardIndex() {
  return epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, January, 18d},
       epoch_frame::DateTime{2020y, January, 19d},
       epoch_frame::DateTime{2020y, January, 20d}});
}
} // namespace

TEST_CASE("TradeExecutorAdapter") {
  SECTION("Constructor initializes correctly") {
    auto config = trade_executor_adapter_cfg(
        "test_adapter", "test_input",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    TradeExecutorAdapter adapter(config);

    REQUIRE(adapter.GetInputId() == "test_input");
    REQUIRE(adapter.GetOutputId("long") == "test_adapter#long");
    REQUIRE(adapter.GetOutputId("short") == "test_adapter#short");
  }

  SECTION("TransformData with positive values") {
    auto config = trade_executor_adapter_cfg(
        "test_adapter", "signal",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    TradeExecutorAdapter adapter(config);

    // Create test data with positive values (long signals)
    auto testData = createTestDataFrame({1.0, 2.5, 0.0, -1.5, 3.0});

    auto result = adapter.TransformData(testData);

    // The adapter should only output the long and short columns, not include
    // the original
    REQUIRE(result.num_cols() == 2);
    REQUIRE(result.contains("test_adapter#long"));
    REQUIRE(result.contains("test_adapter#short"));

    auto longColumn = result["test_adapter#long"];
    auto shortColumn = result["test_adapter#short"];

    // Check long signals (positive values)
    REQUIRE(longColumn.iloc(0).as_bool() == true);  // 1.0 > 0
    REQUIRE(longColumn.iloc(1).as_bool() == true);  // 2.5 > 0
    REQUIRE(longColumn.iloc(2).as_bool() == false); // 0.0 = 0
    REQUIRE(longColumn.iloc(3).as_bool() == false); // -1.5 < 0
    REQUIRE(longColumn.iloc(4).as_bool() == true);  // 3.0 > 0

    // Check short signals (negative values)
    REQUIRE(shortColumn.iloc(0).as_bool() == false); // 1.0 > 0
    REQUIRE(shortColumn.iloc(1).as_bool() == false); // 2.5 > 0
    REQUIRE(shortColumn.iloc(2).as_bool() == false); // 0.0 = 0
    REQUIRE(shortColumn.iloc(3).as_bool() == true);  // -1.5 < 0
    REQUIRE(shortColumn.iloc(4).as_bool() == false); // 3.0 > 0
  }

  SECTION("TransformData with all zero values") {
    auto config = trade_executor_adapter_cfg(
        "test_adapter", "signal",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    TradeExecutorAdapter adapter(config);

    auto testData = createTestDataFrame({0.0, 0.0, 0.0});
    auto result = adapter.TransformData(testData);

    auto longColumn = result["test_adapter#long"];
    auto shortColumn = result["test_adapter#short"];

    for (size_t i = 0; i < 3; ++i) {
      REQUIRE(longColumn.iloc(i).as_bool() == false);
      REQUIRE(shortColumn.iloc(i).as_bool() == false);
    }
  }
}

TEST_CASE("TradeExecutorTransform - SingleExecutor") {
  SECTION("Constructor with only long input") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}}, {"long_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_LONG_KEY));

    auto longColumn = result[TE_ENTER_LONG_KEY];
    REQUIRE(longColumn.iloc(0).as_bool() == true);
    REQUIRE(longColumn.iloc(1).as_bool() == false);
    REQUIRE(longColumn.iloc(2).as_bool() == true);
  }

  SECTION("Constructor with only short input") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_SHORT_KEY, "short_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}}, {"short_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_SHORT_KEY));

    auto shortColumn = result[TE_ENTER_SHORT_KEY];
    REQUIRE(shortColumn.iloc(0).as_bool() == true);
    REQUIRE(shortColumn.iloc(1).as_bool() == false);
    REQUIRE(shortColumn.iloc(2).as_bool() == true);
  }
}

TEST_CASE("TradeExecutorTransform - SingleExecutorWithExit") {
  SECTION("Constructor with long and close inputs") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_EXIT_LONG_KEY, "exit_long_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}, {false, true, false}},
        {"long_signal", "exit_long_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_LONG_KEY));
    REQUIRE(result.contains(TE_EXIT_LONG_KEY));

    auto longColumn = result[TE_ENTER_LONG_KEY];
    auto closeColumn = result[TE_EXIT_LONG_KEY];
    REQUIRE(longColumn.iloc(0).as_bool() == true);
    REQUIRE(closeColumn.iloc(1).as_bool() == true);
  }

  SECTION("Constructor with short and close inputs") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_SHORT_KEY, "short_signal"},
        {TE_EXIT_SHORT_KEY, "exit_short_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}, {false, true, false}},
        {"short_signal", "exit_short_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_SHORT_KEY));
    REQUIRE(result.contains(TE_EXIT_SHORT_KEY));

    auto shortColumn = result[TE_ENTER_SHORT_KEY];
    auto closeColumn = result[TE_EXIT_SHORT_KEY];
    REQUIRE(shortColumn.iloc(0).as_bool() == true);
    REQUIRE(closeColumn.iloc(1).as_bool() == true);
  }
}

TEST_CASE("TradeExecutorTransform - MultipleExecutor") {
  SECTION("Constructor with long and short inputs") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_ENTER_SHORT_KEY, "short_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}, {false, true, false}},
        {"long_signal", "short_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_LONG_KEY));
    REQUIRE(result.contains(TE_ENTER_SHORT_KEY));

    auto longColumn = result[TE_ENTER_LONG_KEY];
    auto shortColumn = result[TE_ENTER_SHORT_KEY];
    REQUIRE(longColumn.iloc(0).as_bool() == true);
    REQUIRE(shortColumn.iloc(1).as_bool() == true);
  }
}

TEST_CASE("TradeExecutorTransform - MultipleExecutorWithExit") {
  SECTION("Constructor with long, short and close inputs") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_ENTER_SHORT_KEY, "short_signal"},
        {TE_EXIT_LONG_KEY, "exit_long_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}, {false, true, false}, {true, false, true}},
        {"long_signal", "short_signal", "exit_long_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);
    REQUIRE(result.contains(TE_ENTER_LONG_KEY));
    REQUIRE(result.contains(TE_ENTER_SHORT_KEY));
    REQUIRE(result.contains(TE_EXIT_LONG_KEY));

    auto longColumn = result[TE_ENTER_LONG_KEY];
    auto shortColumn = result[TE_ENTER_SHORT_KEY];
    auto closeColumn = result[TE_EXIT_LONG_KEY];
    REQUIRE(longColumn.iloc(0).as_bool() == true);
    REQUIRE(shortColumn.iloc(1).as_bool() == true);
    REQUIRE(closeColumn.iloc(0).as_bool() == true);
  }
}

TEST_CASE("TradeExecutorTransform - Basic Pass-through") {
  SECTION("Long/Short signals pass-through without allow masking") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_ENTER_SHORT_KEY, "short_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, true, true}, {true, true, true}},
        {"long_signal", "short_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);

    // Signals pass through unchanged
    auto longResult = result[TE_ENTER_LONG_KEY];
    auto shortResult = result[TE_ENTER_SHORT_KEY];

    // Row 0: allow=true, long=true, short=true -> long=true, short=true
    REQUIRE(longResult.iloc(0).as_bool() == true);
    REQUIRE(shortResult.iloc(0).as_bool() == true);

    // Row 1: still based on input
    REQUIRE(longResult.iloc(1).as_bool() == true);
    REQUIRE(shortResult.iloc(1).as_bool() == true);

    // Row 2: allow=true, long=true, short=true -> long=true, short=true
    REQUIRE(longResult.iloc(2).as_bool() == true);
    REQUIRE(shortResult.iloc(2).as_bool() == true);
  }

  SECTION("Long/Short with exit pass-through") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_ENTER_SHORT_KEY, "short_signal"},
        {TE_EXIT_LONG_KEY, "exit_long_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testDataWithCorrectColumns = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}, {false, true, false}, {true, false, true}},
        {"long_signal", "short_signal", "exit_long_signal"});

    auto result = transform.TransformData(testDataWithCorrectColumns);

    auto longResult = result[TE_ENTER_LONG_KEY];
    auto shortResult = result[TE_ENTER_SHORT_KEY];
    auto closeResult = result[TE_EXIT_LONG_KEY];

    // Row 0: all signals preserved
    REQUIRE(longResult.iloc(0).as_bool() == true);
    REQUIRE(shortResult.iloc(0).as_bool() == false);
    REQUIRE(closeResult.iloc(0).as_bool() == true);

    // Row 1: preserved from inputs
    REQUIRE(longResult.iloc(1).as_bool() == false);
    REQUIRE(shortResult.iloc(1).as_bool() == true);
    REQUIRE(closeResult.iloc(1).as_bool() == false);

    // Row 2: all signals preserved
    REQUIRE(longResult.iloc(2).as_bool() == true);
    REQUIRE(shortResult.iloc(2).as_bool() == false);
    REQUIRE(closeResult.iloc(2).as_bool() == true);
  }

  // allow-related case removed
}

// closeIfIndecisive removed; corresponding tests dropped.

// allow signal and indecisive behavior removed; tests dropped.

TEST_CASE("TradeExecutorTransform - Error Cases") {
  SECTION("Invalid input key is ignored") {
    std::unordered_map<std::string, std::string> inputs = {
        {"invalid_key", "signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Should not throw but create a valid transform that ignores invalid keys
    REQUIRE_NOTHROW(TradeExecutorTransform(config));

    TradeExecutorTransform transform(config);
    auto testData = createTestDataFrame({1.0, 0.0, -1.0});
    auto result = transform.TransformData(testData);

    // Should return the input data unchanged when invalid keys are used
    REQUIRE_FALSE(result.contains("signal"));
  }

  SECTION("Empty inputs") {
    std::unordered_map<std::string, std::string> inputs = {};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto testData = createTestDataFrame({1.0, 0.0, -1.0});
    auto result = transform.TransformData(testData);

    // Should return the input data unchanged when no replacements
    REQUIRE_FALSE(result.contains("signal"));
  }

  SECTION("Missing input columns in data") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "missing_column"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    auto testData = epoch_frame::make_dataframe<bool>(
        index, {{true, false, true}}, {"existing_column"});

    // Should not throw but may not produce expected output
    REQUIRE_THROWS(transform.TransformData(testData));
  }
}

TEST_CASE("TradeExecutorTransform - Types selection") {
  SECTION("Single input type determines correct executor type") {
    SECTION("Long only -> SingleExecutor") {
      std::unordered_map<std::string, std::string> inputs = {
          {TE_ENTER_LONG_KEY, "long_signal"}};
      auto config = trade_signal_executor_cfg(
          "test", inputs, epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      REQUIRE_NOTHROW(TradeExecutorTransform(config));
    }

    SECTION("Short only -> SingleExecutor") {
      std::unordered_map<std::string, std::string> inputs = {
          {TE_ENTER_SHORT_KEY, "short_signal"}};
      auto config = trade_signal_executor_cfg(
          "test", inputs, epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      REQUIRE_NOTHROW(TradeExecutorTransform(config));
    }

    SECTION("Long + Close -> SingleExecutorWithExit") {
      std::unordered_map<std::string, std::string> inputs = {
          {TE_ENTER_LONG_KEY, "long_signal"},
          {TE_EXIT_LONG_KEY, "exit_long_signal"}};
      auto config = trade_signal_executor_cfg(
          "test", inputs, epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      REQUIRE_NOTHROW(TradeExecutorTransform(config));
    }

    SECTION("Long + Short -> MultipleExecutor") {
      std::unordered_map<std::string, std::string> inputs = {
          {TE_ENTER_LONG_KEY, "long_signal"},
          {TE_ENTER_SHORT_KEY, "short_signal"}};
      auto config = trade_signal_executor_cfg(
          "test", inputs, epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      REQUIRE_NOTHROW(TradeExecutorTransform(config));
    }

    SECTION("Long + Short + Close -> MultipleExecutorWithExit") {
      std::unordered_map<std::string, std::string> inputs = {
          {TE_ENTER_LONG_KEY, "long_signal"},
          {TE_ENTER_SHORT_KEY, "short_signal"},
          {TE_EXIT_LONG_KEY, "exit_long_signal"}};
      auto config = trade_signal_executor_cfg(
          "test", inputs, epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      REQUIRE_NOTHROW(TradeExecutorTransform(config));
    }
  }
}

TEST_CASE("TradeExecutorTransform - Data Type Handling") {
  SECTION("Mixed boolean and null handling") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"},
        {TE_ENTER_SHORT_KEY, "short_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    // Mix of boolean values and nulls
    auto testDataMixed = epoch_frame::make_dataframe(
        index,
        {{Scalar{true}, Scalar{}, Scalar{false}},
         {Scalar{false}, Scalar{true}, Scalar{}}},
        {"long_signal", "short_signal"}, arrow::boolean());

    auto result = transform.TransformData(testDataMixed);

    auto longResult = result[TE_ENTER_LONG_KEY];
    auto shortResult = result[TE_ENTER_SHORT_KEY];

    // Verify data types and values are preserved correctly
    REQUIRE(longResult.iloc(0).as_bool() == true);
    REQUIRE(longResult.iloc(1).is_null());
    REQUIRE(longResult.iloc(2).as_bool() == false);

    REQUIRE(shortResult.iloc(0).as_bool() == false);
    REQUIRE(shortResult.iloc(1).as_bool() == true);
    REQUIRE(shortResult.iloc(2).is_null());
  }

  SECTION("All null inputs") {
    std::unordered_map<std::string, std::string> inputs = {
        {TE_ENTER_LONG_KEY, "long_signal"}};
    auto config = trade_signal_executor_cfg(
        "test_transform", inputs,
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    TradeExecutorTransform transform(config);

    auto index = createStandardIndex();
    // All null values
    auto testDataAllNulls =
        epoch_frame::make_dataframe(index, {{Scalar{}, Scalar{}, Scalar{}}},
                                    {"long_signal"}, arrow::boolean());

    auto result = transform.TransformData(testDataAllNulls);

    auto longResult = result[TE_ENTER_LONG_KEY];

    // All values should remain null
    REQUIRE(longResult.iloc(0).is_null());
    REQUIRE(longResult.iloc(1).is_null());
    REQUIRE(longResult.iloc(2).is_null());
  }
}
