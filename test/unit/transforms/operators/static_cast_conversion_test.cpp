//
// Created by Claude Code
// Comprehensive type conversion tests for StaticCast transforms
// Testing Arrow compute functions for bool↔decimal conversions
//

#include <catch2/catch_test_macros.hpp>
#include <epoch_script/core/bar_attribute.h>
#include <epoch_script/core/constants.h>
#include "epoch_script/strategy/registration.h"
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include "transforms/components/operators/static_cast.h"
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <arrow/type.h>
#include <arrow/builder.h>

using namespace epoch_core;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// === Boolean ↔ Decimal Conversion Tests ===

TEST_CASE("[static_cast_conversion] Boolean to Decimal - true/false", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d}
  });
  auto input_df = make_dataframe<bool>(index, {{true, false, true}}, {"input"});

  auto config = static_cast_to_decimal_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 3);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::DOUBLE);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_double() == 1.0);
  REQUIRE(result["static_cast_test#result"].iloc(1).as_double() == 0.0);
  REQUIRE(result["static_cast_test#result"].iloc(2).as_double() == 1.0);
}

TEST_CASE("[static_cast_conversion] Boolean to Decimal with nulls", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d}
  });

  // Create boolean array with nulls
  arrow::BooleanBuilder builder;
  (void)builder.Append(true);
  (void)builder.AppendNull();
  (void)builder.Append(false);
  auto bool_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(bool_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_decimal_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 3);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::DOUBLE);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_double() == 1.0);
  REQUIRE(result["static_cast_test#result"].iloc(1).is_null());
  REQUIRE(result["static_cast_test#result"].iloc(2).as_double() == 0.0);
}

TEST_CASE("[static_cast_conversion] Decimal to Boolean - zero and non-zero", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d},
    epoch_frame::DateTime{2024y, std::chrono::January, 4d},
    epoch_frame::DateTime{2024y, std::chrono::January, 5d}
  });
  auto input_df = make_dataframe<double>(index, {{1.5, 0.0, -2.5, 0.001, -0.0}}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 5);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == true);   // 1.5 != 0
  REQUIRE(result["static_cast_test#result"].iloc(1).as_bool() == false);  // 0.0 == 0
  REQUIRE(result["static_cast_test#result"].iloc(2).as_bool() == true);   // -2.5 != 0
  REQUIRE(result["static_cast_test#result"].iloc(3).as_bool() == true);   // 0.001 != 0
  REQUIRE(result["static_cast_test#result"].iloc(4).as_bool() == false);  // -0.0 == 0
}

TEST_CASE("[static_cast_conversion] Decimal to Boolean with nulls", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d}
  });

  // Create double array with nulls
  arrow::DoubleBuilder builder;
  (void)builder.Append(1.5);
  (void)builder.AppendNull();
  (void)builder.Append(0.0);
  auto double_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(double_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 3);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == true);
  REQUIRE(result["static_cast_test#result"].iloc(1).is_null());
  REQUIRE(result["static_cast_test#result"].iloc(2).as_bool() == false);
}

// === Integer ↔ Boolean Conversion Tests ===

TEST_CASE("[static_cast_conversion] Integer to Boolean - zero and non-zero", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d},
    epoch_frame::DateTime{2024y, std::chrono::January, 4d}
  });
  auto input_df = make_dataframe<int64_t>(index, {{1, 0, -5, 100}}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 4);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == true);   // 1 != 0
  REQUIRE(result["static_cast_test#result"].iloc(1).as_bool() == false);  // 0 == 0
  REQUIRE(result["static_cast_test#result"].iloc(2).as_bool() == true);   // -5 != 0
  REQUIRE(result["static_cast_test#result"].iloc(3).as_bool() == true);   // 100 != 0
}

TEST_CASE("[static_cast_conversion] Integer to Boolean with nulls", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d},
    epoch_frame::DateTime{2024y, std::chrono::January, 3d}
  });

  // Create int64 array with nulls
  arrow::Int64Builder builder;
  (void)builder.Append(42);
  (void)builder.AppendNull();
  (void)builder.Append(0);
  auto int_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(int_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 3);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == true);
  REQUIRE(result["static_cast_test#result"].iloc(1).is_null());
  REQUIRE(result["static_cast_test#result"].iloc(2).as_bool() == false);
}

// === Passthrough Tests (already compatible types) ===

TEST_CASE("[static_cast_conversion] Integer passthrough", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d}
  });
  auto input_df = make_dataframe<int64_t>(index, {{42, -17}}, {"input"});

  auto config = static_cast_to_integer_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 2);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::INT64);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_int64() == 42);
  REQUIRE(result["static_cast_test#result"].iloc(1).as_int64() == -17);
}

TEST_CASE("[static_cast_conversion] Decimal passthrough", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d}
  });
  auto input_df = make_dataframe<double>(index, {{3.14159, -2.71828}}, {"input"});

  auto config = static_cast_to_decimal_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 2);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::DOUBLE);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_double() == 3.14159);
  REQUIRE(result["static_cast_test#result"].iloc(1).as_double() == -2.71828);
}

TEST_CASE("[static_cast_conversion] Boolean passthrough", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d},
    epoch_frame::DateTime{2024y, std::chrono::January, 2d}
  });
  auto input_df = make_dataframe<bool>(index, {{true, false}}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 2);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == true);
  REQUIRE(result["static_cast_test#result"].iloc(1).as_bool() == false);
}

// === Error/Edge Cases ===

TEST_CASE("[static_cast_conversion] String to Integer should throw", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d}
  });

  arrow::StringBuilder builder;
  (void)builder.Append("hello");
  auto string_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(string_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_integer_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  REQUIRE_THROWS_AS(transform->TransformData(input_df), std::runtime_error);
}

TEST_CASE("[static_cast_conversion] String to Boolean should throw", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d}
  });

  arrow::StringBuilder builder;
  (void)builder.Append("true");
  auto string_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(string_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  REQUIRE_THROWS_AS(transform->TransformData(input_df), std::runtime_error);
}

TEST_CASE("[static_cast_conversion] String to Decimal should throw", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto index = epoch_frame::factory::index::make_datetime_index({
    epoch_frame::DateTime{2024y, std::chrono::January, 1d}
  });

  arrow::StringBuilder builder;
  (void)builder.Append("123.45");
  auto string_array = builder.Finish().ValueOrDie();
  auto chunked = std::make_shared<arrow::ChunkedArray>(string_array);
  auto input_df = epoch_frame::make_dataframe(index, {chunked}, {"input"});

  auto config = static_cast_to_decimal_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  REQUIRE_THROWS_AS(transform->TransformData(input_df), std::runtime_error);
}

// === Large data tests for Arrow compute stability ===

TEST_CASE("[static_cast_conversion] Boolean to Decimal - large dataset", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  std::vector<epoch_frame::DateTime> dates;
  std::vector<bool> values;
  for (int i = 0; i < 1000; ++i) {
    dates.push_back(epoch_frame::DateTime{2024y, std::chrono::January, 1d} + std::chrono::days(i));
    values.push_back(i % 2 == 0);
  }

  auto index = epoch_frame::factory::index::make_datetime_index(dates);
  auto input_df = make_dataframe<bool>(index, {values}, {"input"});

  auto config = static_cast_to_decimal_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 1000);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::DOUBLE);
  // Spot check
  REQUIRE(result["static_cast_test#result"].iloc(0).as_double() == 1.0);  // true
  REQUIRE(result["static_cast_test#result"].iloc(1).as_double() == 0.0);  // false
  REQUIRE(result["static_cast_test#result"].iloc(999).as_double() == 0.0); // odd, so false
}

TEST_CASE("[static_cast_conversion] Decimal to Boolean - large dataset", "[operators]") {
  const auto &timeframe = epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  std::vector<epoch_frame::DateTime> dates;
  std::vector<double> values;
  for (int i = 0; i < 1000; ++i) {
    dates.push_back(epoch_frame::DateTime{2024y, std::chrono::January, 1d} + std::chrono::days(i));
    values.push_back(i % 3 == 0 ? 0.0 : static_cast<double>(i));
  }

  auto index = epoch_frame::factory::index::make_datetime_index(dates);
  auto input_df = make_dataframe<double>(index, {values}, {"input"});

  auto config = static_cast_to_boolean_cfg("static_cast_test", "input", timeframe);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  auto result = transform->TransformData(input_df);

  REQUIRE(result.num_rows() == 1000);
  REQUIRE(result["static_cast_test#result"].array()->type()->id() == arrow::Type::BOOL);
  // Spot check
  REQUIRE(result["static_cast_test#result"].iloc(0).as_bool() == false);  // 0.0
  REQUIRE(result["static_cast_test#result"].iloc(1).as_bool() == true);   // 1.0
  REQUIRE(result["static_cast_test#result"].iloc(3).as_bool() == false);  // 3 % 3 == 0
}
