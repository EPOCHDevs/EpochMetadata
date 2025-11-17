//
// Created by adesola on 1/27/25.
//
#include <epoch_script/core/bar_attribute.h>
#include "epoch_script/strategy/registration.h"
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include "transforms/components/operators/groupby_agg.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <spdlog/spdlog.h>

using namespace epoch_core;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

const auto &timeframe = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

TEST_CASE("GroupBy Numeric Agg - Sum", "[groupby][numeric][sum]") {
  // Input: 6 rows, 2 groups (A, B)
  // Group A (rows 0, 2, 4): values 10, 30, 50
  // Group B (rows 1, 3, 5): values 20, 40, 60
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B", "A", "B", "A", "B"});
  auto values_input = epoch_frame::factory::array::make_array<double>(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_sum("sum_test", "group_key", "value", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  auto output = transform->TransformData(input);

  // Expected output: 2 rows (one per group)
  // Sum aggregates raw_index too, so uses last index for each group
  // Group A: sum(10+30+50)=90, last index is row 4 (Jan 5)
  // Group B: sum(20+40+60)=120, last index is row 5 (Jan 6)
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 5d},  // A's last
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}}); // B's last

  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B"});
  auto values_expected = epoch_frame::factory::array::make_array<double>(
      {90.0, 120.0});

  auto expected = make_dataframe(index_expected,
                                 {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"),
                                  config.GetOutputId("value")});

  INFO("GroupBy Sum Output:\n" << output << "\nvs Expected:\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("GroupBy Numeric Agg - First", "[groupby][numeric][first]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B", "A", "B", "A", "B"});
  auto values_input = epoch_frame::factory::array::make_array<double>(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_first("first_test", "group_key", "value", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  auto output = transform->TransformData(input);

  // Expected: first value of each group
  // Group A: first value=10 at index row 0 (Jan 1)
  // Group B: first value=20 at index row 1 (Jan 2)
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d}});

  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B"});
  auto values_expected = epoch_frame::factory::array::make_array<double>(
      {10.0, 20.0});

  auto expected = make_dataframe(index_expected,
                                 {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"),
                                  config.GetOutputId("value")});

  INFO("GroupBy First Output:\n" << output << "\nvs Expected:\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("GroupBy Boolean Agg - AllOf", "[groupby][boolean][allof]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"X", "Y", "X", "Y"});
  auto values_input = epoch_frame::factory::array::make_array<bool>(
      {true, false, true, false});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_allof("allof_test", "group_key", "value", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  auto output = transform->TransformData(input);

  // Expected: AllOf aggregation
  // Group X: all(true, true) = true, last index row 2 (Jan 3)
  // Group Y: all(false, false) = false, last index row 3 (Jan 4)
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>(
      {"X", "Y"});
  auto values_expected = epoch_frame::factory::array::make_array<bool>(
      {true, false});

  auto expected = make_dataframe(index_expected,
                                 {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"),
                                  config.GetOutputId("value")});

  INFO("GroupBy AllOf Output:\n" << output << "\nvs Expected:\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("GroupBy Any Agg - IsEqual", "[groupby][any][isequal]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  // Group X: all same values (10, 10, 10) -> IsEqual = true
  // Group Y: different values (20, 30) -> IsEqual = false
  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"X", "X", "X", "Y", "Y", "Y"});
  auto values_input = epoch_frame::factory::array::make_array<double>(
      {10.0, 10.0, 10.0, 20.0, 30.0, 40.0});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_isequal("isequal_test", "group_key", "value", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  auto output = transform->TransformData(input);

  // Expected: IsEqual checks min==max
  // Group X: min=10, max=10 -> true, last index row 2 (Jan 3)
  // Group Y: min=20, max=40 -> false, last index row 5 (Jan 6)
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>(
      {"X", "Y"});
  auto values_expected = epoch_frame::factory::array::make_array<bool>(
      {true, false});

  auto expected = make_dataframe(index_expected,
                                 {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"),
                                  config.GetOutputId("value")});

  INFO("GroupBy IsEqual Output:\n" << output << "\nvs Expected:\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("GroupBy Any Agg - IsUnique", "[groupby][any][isunique]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  // Group X: all same (10, 10, 10) -> first=10, last=10 -> false
  // Group Y: all different (20, 30, 40) -> first=20, last=40 -> true
  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"X", "X", "X", "Y", "Y", "Y"});
  auto values_input = epoch_frame::factory::array::make_array<double>(
      {10.0, 10.0, 10.0, 20.0, 30.0, 40.0});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_isunique("isunique_test", "group_key", "value", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  REQUIRE(transform != nullptr);

  auto output = transform->TransformData(input);

  // Expected: IsUnique checks first!=last (heuristic)
  // Group X: count=3, first=10, last=10 -> first==last -> false
  // Group Y: count=3, first=20, last=40 -> first!=last -> true
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>(
      {"X", "Y"});
  auto values_expected = epoch_frame::factory::array::make_array<bool>(
      {false, true});

  auto expected = make_dataframe(index_expected,
                                 {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"),
                                  config.GetOutputId("value")});

  INFO("GroupBy IsUnique Output:\n" << output << "\nvs Expected:\n" << expected);
  REQUIRE(output.equals(expected));
}

// Test all numeric aggregations comprehensively
TEST_CASE("GroupBy Numeric Agg - All Aggregations", "[groupby][numeric][comprehensive]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B", "A", "B"});
  auto values_input = epoch_frame::factory::array::make_array<double>(
      {10.0, 5.0, 30.0, 15.0});

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  SECTION("Mean") {
    auto config = groupby_mean("mean_test", "group_key", "value", timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    auto output = transform->TransformData(input);

    // Group A: mean(10, 30) = 20.0
    // Group B: mean(5, 15) = 10.0
    auto index_expected = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
    auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});
    auto values_expected = epoch_frame::factory::array::make_array<double>({20.0, 10.0});
    auto expected = make_dataframe(index_expected, {group_keys_expected, values_expected},
                                   {config.GetOutputId("group_key"), config.GetOutputId("value")});

    INFO("Mean:\n" << output << "\nvs\n" << expected);
    REQUIRE(output.equals(expected, arrow::EqualOptions{}.atol(1e-10)));
  }

  SECTION("Count") {
    auto config = groupby_count("count_test", "group_key", "value", timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    auto output = transform->TransformData(input);

    // Both groups have 2 elements
    auto index_expected = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
    auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});
    auto values_expected = epoch_frame::factory::array::make_array<int64_t>({2, 2});
    auto expected = make_dataframe(index_expected, {group_keys_expected, values_expected},
                                   {config.GetOutputId("group_key"), config.GetOutputId("value")});

    INFO("Count:\n" << output << "\nvs\n" << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("Min/Max") {
    auto config_min = groupby_min("min_test", "group_key", "value", timeframe);
    auto config_max = groupby_max("max_test", "group_key", "value", timeframe);
      auto ptr1 = MAKE_TRANSFORM(config_min);
      auto transform_min = dynamic_cast<ITransform *>(ptr1.get());
      auto ptr2 = MAKE_TRANSFORM(config_max);
      auto transform_max = dynamic_cast<ITransform *>(ptr2.get());

    REQUIRE(transform_min != nullptr);
    REQUIRE(transform_max != nullptr);

    auto output_min = transform_min->TransformData(input);
    auto output_max = transform_max->TransformData(input);

    // Group A: min=10, max=30
    // Group B: min=5, max=15
    auto index_expected = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
    auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});

    auto values_min = epoch_frame::factory::array::make_array<double>({10.0, 5.0});
    auto values_max = epoch_frame::factory::array::make_array<double>({30.0, 15.0});

    auto expected_min = make_dataframe(index_expected, {group_keys_expected, values_min},
                                       {config_min.GetOutputId("group_key"), config_min.GetOutputId("value")});
    auto expected_max = make_dataframe(index_expected, {group_keys_expected, values_max},
                                       {config_max.GetOutputId("group_key"), config_max.GetOutputId("value")});

    INFO("Min:\n" << output_min << "\nvs\n" << expected_min);
    REQUIRE(output_min.equals(expected_min));

    INFO("Max:\n" << output_max << "\nvs\n" << expected_max);
    REQUIRE(output_max.equals(expected_max));
  }

  SECTION("Last") {
    auto config = groupby_last("last_test", "group_key", "value", timeframe);
      auto ptr = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(ptr.get());
    auto output = transform->TransformData(input);

    // Group A: last value=30 at index row 2 (Jan 3)
    // Group B: last value=15 at index row 3 (Jan 4)
    auto index_expected = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
    auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});
    auto values_expected = epoch_frame::factory::array::make_array<double>({30.0, 15.0});
    auto expected = make_dataframe(index_expected, {group_keys_expected, values_expected},
                                   {config.GetOutputId("group_key"), config.GetOutputId("value")});

    INFO("Last:\n" << output << "\nvs\n" << expected);
    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("GroupBy Boolean Agg - AnyOf", "[groupby][boolean][anyof]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B", "A", "B"});
  auto values_input = epoch_frame::factory::array::make_array<bool>(
      {false, false, true, false});  // A has [false, true], B has [false, false]

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_anyof("anyof_test", "group_key", "value", timeframe);
    auto ptr = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(ptr.get());
  auto output = transform->TransformData(input);

  // Group A: anyof(false, true) = true
  // Group B: anyof(false, false) = false
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});
  auto values_expected = epoch_frame::factory::array::make_array<bool>({true, false});
  auto expected = make_dataframe(index_expected, {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"), config.GetOutputId("value")});

  INFO("AnyOf:\n" << output << "\nvs\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("GroupBy Boolean Agg - NoneOf", "[groupby][boolean][noneof]") {
  auto index_input = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  auto group_keys_input = epoch_frame::factory::array::make_array<std::string>(
      {"A", "B", "A", "B"});
  auto values_input = epoch_frame::factory::array::make_array<bool>(
      {false, true, false, false});  // A has [false, false], B has [true, false]

  auto input = make_dataframe(index_input, {group_keys_input, values_input},
                              {"group_key", "value"});

  auto config = groupby_noneof("noneof_test", "group_key", "value", timeframe);
    auto ptr = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(ptr.get());
  auto output = transform->TransformData(input);

  // Group A: noneof(false, false) = NOT(anyof) = NOT(false) = true
  // Group B: noneof(true, false) = NOT(anyof) = NOT(true) = false
  auto index_expected = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});
  auto group_keys_expected = epoch_frame::factory::array::make_array<std::string>({"A", "B"});
  auto values_expected = epoch_frame::factory::array::make_array<bool>({true, false});
  auto expected = make_dataframe(index_expected, {group_keys_expected, values_expected},
                                 {config.GetOutputId("group_key"), config.GetOutputId("value")});

  INFO("NoneOf:\n" << output << "\nvs\n" << expected);
  REQUIRE(output.equals(expected));
}
