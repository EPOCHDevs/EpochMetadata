//
// Created by adesola on 10/31/25.
//

#include <epoch_script/core/bar_attribute.h>
#include <epoch_script/core/constants.h>
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_core;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// Helper function to create a test dataframe with strings
DataFrame createStringTestDataFrame() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d}});

  return make_dataframe<std::string>(
      index,
      {{"HELLO", "world", "Hello World", "  trimme  ", "123abc"}},
      {"text"});
}

TEST_CASE("String Case Transform", "[string][string_case]") {
  auto input = createStringTestDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Uppercase") {
    auto config = string_case_cfg("test_upper", "upper", "text", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<std::string>(
        index,
        {{"HELLO", "WORLD", "HELLO WORLD", "  TRIMME  ", "123ABC"}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }

  SECTION("Lowercase") {
    auto config = string_case_cfg("test_lower", "lower", "text", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<std::string>(
        index,
        {{"hello", "world", "hello world", "  trimme  ", "123abc"}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("String Trim Transform", "[string][string_trim]") {
  auto input = createStringTestDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Trim whitespace") {
    auto config = string_trim_cfg("test_trim", "trim", "text", "", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<std::string>(
        index,
        {{"HELLO", "world", "Hello World", "trimme", "123abc"}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("String Contains Transform", "[string][string_contains]") {
  auto input = createStringTestDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Contains pattern") {
    auto config = string_contains_cfg("test_contains", "contains", "text", "o", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<bool>(
        index,
        {{false, true, true, false, false}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }

  SECTION("Starts with pattern") {
    auto config = string_contains_cfg("test_starts", "starts_with", "text", "H", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<bool>(
        index,
        {{true, false, true, false, false}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("String Check Transform", "[string][string_check]") {
  auto input = make_dataframe<std::string>(
      epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d},
           epoch_frame::DateTime{2020y, std::chrono::January, 3d},
           epoch_frame::DateTime{2020y, std::chrono::January, 4d}}),
      {{"abc", "123", "ABC", " "}},
      {"text"});

  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Is alpha") {
    auto config = string_check_cfg("test_check", "is_alpha", "text", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<bool>(
        index,
        {{true, false, true, false}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }

  SECTION("Is digit") {
    auto config = string_check_cfg("test_digit", "is_digit", "text", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    DataFrame output = transform->TransformData(input);

    DataFrame expected = make_dataframe<bool>(
        index,
        {{false, true, false, false}},
        {config.GetOutputId()});

    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("String Replace Transform", "[string][string_replace]") {
  auto input = createStringTestDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto config = string_replace_cfg("test_replace", "text", "l", "L", tf);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  DataFrame output = transform->TransformData(input);

  DataFrame expected = make_dataframe<std::string>(
      index,
      {{"HELLO", "worLd", "HeLLo WorLd", "  trimme  ", "123abc"}},
      {config.GetOutputId()});

  REQUIRE(output.equals(expected));
}

TEST_CASE("String Length Transform", "[string][string_length]") {
  auto input = createStringTestDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto config = string_length_cfg("test_length", "text", tf);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  DataFrame output = transform->TransformData(input);

  DataFrame expected = make_dataframe<int32_t>(
      index,
      {{5, 5, 11, 10, 6}},
      {config.GetOutputId()});

  REQUIRE(output.equals(expected));
}

TEST_CASE("String Reverse Transform", "[string][string_reverse]") {
  auto input = make_dataframe<std::string>(
      epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d}}),
      {{"HELLO", "world"}},
      {"text"});

  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  auto config = string_reverse_cfg("test_reverse", "text", tf);
  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());
  DataFrame output = transform->TransformData(input);

  DataFrame expected = make_dataframe<std::string>(
      index,
      {{"OLLEH", "dlrow"}},
      {config.GetOutputId()});

  REQUIRE(output.equals(expected));
}
