//
// Created by adesola on 1/26/25.
//
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/transforms/config_helper.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "transforms/src/agg.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// Helper function to create a multi-column test DataFrame
epoch_frame::DataFrame MakeMultiColumnTestData() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  return make_dataframe<double>(index,
                                {
                                    {10.0, 20.0, 30.0, 40.0}, // col_1
                                    {5.0, 15.0, 25.0, 35.0},  // col_2
                                    {2.0, 4.0, 6.0, 8.0}      // col_3
                                },
                                {"col_1", "col_2", "col_3"});
}

// Helper function to create a boolean test DataFrame
epoch_frame::DataFrame MakeBooleanTestData() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  return make_dataframe<bool>(index,
                              {
                                  {true, false, true, true},  // bool_1
                                  {true, true, false, true},  // bool_2
                                  {false, false, true, false} // bool_3
                              },
                              {"bool_1", "bool_2", "bool_3"});
}

TEST_CASE("Aggregate Transforms - Numeric Operations", "[aggregate]") {
  auto input = MakeMultiColumnTestData();
  std::vector<std::string> columns = {"col_1", "col_2", "col_3"};
  const auto &timeframe =
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Sum Aggregate") {
    // col_1 + col_2 + col_3 = [17, 39, 61, 83]
    std::vector<double> expectedSum = {17.0, 39.0, 61.0, 83.0};

    // Test using the new helper function
    auto config = agg_sum("sum_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<double>(input.index(), {expectedSum},
                                           {config.GetOutputId()});

    INFO("Testing sum aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("Average Aggregate") {
    // (col_1 + col_2 + col_3) / 3 = [5.67, 13.0, 20.33, 27.67]
    std::vector<double> expectedAvg = {5.67, 13.0, 20.33, 27.67};

    // Test using the helper function
    auto config = agg_mean("avg_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<double>(input.index(), {expectedAvg},
                                           {config.GetOutputId()});

    INFO("Testing average aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected, arrow::EqualOptions{}.atol(1e-2)));
  }

  SECTION("Min Aggregate") {
    // min(col_1, col_2, col_3) = [2.0, 4.0, 6.0, 8.0]
    std::vector<double> expectedMin = {2.0, 4.0, 6.0, 8.0};

    // Test using the helper function
    auto config = agg_min("min_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<double>(input.index(), {expectedMin},
                                           {config.GetOutputId()});

    INFO("Testing min aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("Max Aggregate") {
    // max(col_1, col_2, col_3) = [10.0, 20.0, 30.0, 40.0]
    std::vector<double> expectedMax = {10.0, 20.0, 30.0, 40.0};

    // Test using the helper function
    auto config = agg_max("max_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<double>(input.index(), {expectedMax},
                                           {config.GetOutputId()});

    INFO("Testing max aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("Aggregate Transforms - Boolean Operations", "[aggregate]") {
  auto input = MakeBooleanTestData();
  std::vector<std::string> columns = {"bool_1", "bool_2", "bool_3"};
  const auto &timeframe =
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("AllOf Aggregate") {
    // bool_1 && bool_2 && bool_3 = [false, false, false, false]
    std::vector<bool> expectedAllOf = {false, false, false, false};

    // Test with helper function
    auto config = agg_all_of("allof_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<bool>(input.index(), {expectedAllOf},
                                         {config.GetOutputId()});

    INFO("Testing allof aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("AnyOf Aggregate") {
    // bool_1 || bool_2 || bool_3 = [true, true, true, true]
    std::vector<bool> expectedAnyOf = {true, true, true, true};

    // Test with helper function
    auto config = agg_any_of("anyof_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<bool>(input.index(), {expectedAnyOf},
                                         {config.GetOutputId()});

    INFO("Testing anyof aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("NoneOf Aggregate") {
    // !(bool_1 || bool_2 || bool_3) = [false, false, false, false]
    std::vector<bool> expectedNoneOf = {false, false, false, false};

    // Test with helper function
    auto config = agg_none_of("noneof_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<bool>(input.index(), {expectedNoneOf},
                                         {config.GetOutputId()});

    INFO("Testing noneof aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("Aggregate Transforms - Comparison Operations", "[aggregate]") {
  auto input = MakeMultiColumnTestData();
  std::vector<std::string> columns = {"col_1", "col_2"};
  const auto &timeframe =
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("IsEqual Aggregate") {
    // col_1 == col_2 = [false, false, false, false]
    std::vector<bool> expectedIsEqual = {false, false, false, false};

    // Test with helper function
    auto config = agg_all_equal("isequal_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<bool>(input.index(), {expectedIsEqual},
                                         {config.GetOutputId()});

    INFO("Testing isequal aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("IsUnique Aggregate") {
    // col_1 != col_2 = [true, true, true, true]
    std::vector<bool> expectedIsUnique = {true, true, true, true};

    // Test with helper function
    auto config = agg_all_unique("isunique_test", columns, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto output = transform->TransformData(input);
    auto expected = make_dataframe<bool>(input.index(), {expectedIsUnique},
                                         {config.GetOutputId()});

    INFO("Testing isunique aggregate with helper function\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }
}