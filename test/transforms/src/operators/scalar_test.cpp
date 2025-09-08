//
// Created by adesola on 1/26/25.
//
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/constants.h"
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/transforms/config_helper.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "transforms/src/scalar.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <numbers>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// Helper function to create a simple test dataframe
DataFrame createTestDataFrame() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d}});

  return make_dataframe<double>(index, {{10.0, 20.0, 30.0}}, {"price"});
}

// Helper function to test any scalar transform
void testScalarTransform(const std::string &transformType,
                         const std::string &transformId, double expectedValue) {
  auto input = createTestDataFrame();
  auto index = input.index();

  TransformConfiguration config =
      TransformConfiguration{TransformDefinition{YAML::Load(std::format(
          R"(
type: {}
id: {}
timeframe: {}
)",
          transformType, transformId,
          epoch_metadata::EpochStratifyXConstants::instance()
              .DAILY_FREQUENCY.Serialize()))}};

  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  DataFrame output = transform->TransformData(input);
  DataFrame expected = make_dataframe<double>(
      index, {{expectedValue, expectedValue, expectedValue}},
      {config.GetOutputId()});

  INFO("Comparing " << transformType << " values\n"
                    << output << "\n!=\n"
                    << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("Numeric Scalar Transform", "[scalar]") {
  // Test with explicit value
  auto input = createTestDataFrame();
  auto index = input.index();

  // Test the scalar number transform
  TransformConfiguration config =
      TransformConfiguration{TransformDefinition{YAML::Load(std::format(
          R"(
type: number
id: number_5
options:
  value: 5.0
timeframe: {}
)",
          epoch_metadata::EpochStratifyXConstants::instance()
              .DAILY_FREQUENCY.Serialize()))}};

  auto transformBase = MAKE_TRANSFORM(config);
  auto transform = dynamic_cast<ITransform *>(transformBase.get());

  DataFrame output = transform->TransformData(input);
  DataFrame expected =
      make_dataframe<double>(index, {{5.0, 5.0, 5.0}}, {config.GetOutputId()});

  INFO("Comparing numeric scalar values\n" << output << "\n!=\n" << expected);
  REQUIRE(output.equals(expected));
}

TEST_CASE("Mathematical Constants", "[scalar]") {
  // Test important mathematical constants
  testScalarTransform("pi", "pi_value", std::numbers::pi);
  testScalarTransform("e", "e_value", std::numbers::e);
  testScalarTransform("phi", "phi_value", std::numbers::phi);
  testScalarTransform("sqrt2", "sqrt2_value", std::sqrt(2));
  testScalarTransform("sqrt3", "sqrt3_value", std::sqrt(3));
  testScalarTransform("sqrt5", "sqrt5_value", std::sqrt(5));
}

TEST_CASE("Logarithmic Constants", "[scalar]") {
  // Test logarithmic constants
  testScalarTransform("ln2", "ln2_value", std::numbers::ln2);
  testScalarTransform("ln10", "ln10_value", std::numbers::ln10);
  testScalarTransform("log2e", "log2e_value", std::numbers::log2e);
  testScalarTransform("log10e", "log10e_value", std::numbers::log10e);
}

TEST_CASE("Common Integers", "[scalar]") {
  // Test basic integer values
  testScalarTransform("zero", "zero_value", 0.0);
  testScalarTransform("one", "one_value", 1.0);
  testScalarTransform("negative_one", "negative_one_value", -1.0);
}

TEST_CASE("Using Helper Functions", "[scalar]") {
  auto input = createTestDataFrame();
  auto index = input.index();
  const auto &timeframe =
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  // Test helper functions for different types of scalar constants
  SECTION("numeric constant") {
    auto config = number_op("num_helper_test", 42.0, timeframe);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    DataFrame output = transform->TransformData(input);
    DataFrame expected = make_dataframe<double>(index, {{42.0, 42.0, 42.0}},
                                                {config.GetOutputId()});

    INFO("Testing numeric helper function\n" << output << "\n!=\n" << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("mathematical constants") {
    // Test pi helper
    auto config = pi_op("pi_helper_test", timeframe);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    DataFrame output = transform->TransformData(input);
    DataFrame expected = make_dataframe<double>(
        index, {{std::numbers::pi, std::numbers::pi, std::numbers::pi}},
        {config.GetOutputId()});

    INFO("Testing pi helper function\n" << output << "\n!=\n" << expected);
    REQUIRE(output.equals(expected));

    // Test e helper
    config = e_op("e_helper_test", timeframe);
    transformBase = MAKE_TRANSFORM(config);
    transform = dynamic_cast<ITransform *>(transformBase.get());

    output = transform->TransformData(input);
    expected = make_dataframe<double>(
        index, {{std::numbers::e, std::numbers::e, std::numbers::e}},
        {config.GetOutputId()});

    INFO("Testing e helper function\n" << output << "\n!=\n" << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("integer constants") {
    // Test zero helper
    auto config = zero_op("zero_helper_test", timeframe);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    DataFrame output = transform->TransformData(input);
    DataFrame expected = make_dataframe<double>(index, {{0.0, 0.0, 0.0}},
                                                {config.GetOutputId()});

    INFO("Testing zero helper function\n" << output << "\n!=\n" << expected);
    REQUIRE(output.equals(expected));

    // Test one helper
    config = one_op("one_helper_test", timeframe);
    transformBase = MAKE_TRANSFORM(config);
    transform = dynamic_cast<ITransform *>(transformBase.get());

    output = transform->TransformData(input);
    expected = make_dataframe<double>(index, {{1.0, 1.0, 1.0}},
                                      {config.GetOutputId()});

    INFO("Testing one helper function\n" << output << "\n!=\n" << expected);
    REQUIRE(output.equals(expected));
  }
}
