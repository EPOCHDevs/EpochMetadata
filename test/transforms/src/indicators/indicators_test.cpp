//
// Created by adesola on 12/3/24.
//
#include "epoch_metadata/strategy/registration.h"
#include "transforms/src/config_helper.h"
#include "transforms/indicators/bband_variant.h" // includes BollingerBandsPercent, BollingerBandsWidth
#include "transforms/indicators/gap_returns.h"
#include "transforms/indicators/moving_average.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono;
using namespace epoch_frame;

TEST_CASE("Indicator Transforms") {

  SECTION("BBandPercentBModel") {
    // Build transform config
    TransformConfiguration config =
        bbands_percent("bbands_pct", "bbands_lower", "bbands_upper",
                       epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Use registry to create the transform
    auto transformBase = MAKE_TRANSFORM(config);
    auto model = dynamic_cast<ITransform *>(transformBase.get());

    // Make a DataFrame for testing
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, January, 1d},
         epoch_frame::DateTime{2020y, January, 2d},
         epoch_frame::DateTime{2020y, January, 3d}});

    epoch_frame::DataFrame input =
        make_dataframe<double>(index,
                               {
                                   {0.0, 1.0, 5.0}, // CLOSE
                                   {1.0, 2.0, 3.0}, // bbands_lower
                                   {4.0, 5.0, 6.0}  // bbands_upper
                               },
                               {epoch_metadata::EpochStratifyXConstants::instance().CLOSE(),
                                "bbands_lower", "bbands_upper"});

    // (bars[CLOSE()] - lower) / (upper - lower)
    // Row0 => (0 - 1)/(4-1) = -1/3 => -0.3333...
    // Row1 => (1 - 2)/(5-2) = -1/3 => -0.3333...
    // Row2 => (5 - 3)/(6-3) = 2/3
    epoch_frame::DataFrame expected = make_dataframe<double>(
        index, {{-1.0 / 3.0, -1.0 / 3.0, 2.0 / 3.0}}, {config.GetOutputId()});

    auto result = model->TransformData(input);
    INFO("Comparing output with expected values\n"
         << result << "\n!=\n"
         << expected);
    REQUIRE(result.equals(expected));
  }

  SECTION("BBandWidthModel") {
    TransformConfiguration config = bbands_width(
        "bbands_width", "bbands_lower", "bbands_middle", "bbands_upper",
        epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Use registry to create the transform
    auto transformBase = MAKE_TRANSFORM(config);
    auto model = dynamic_cast<ITransform *>(transformBase.get());

    // Make a DataFrame for testing
    // width = (upper - lower) / middle
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, January, 1d},
         epoch_frame::DateTime{2020y, January, 2d},
         epoch_frame::DateTime{2020y, January, 3d}});

    epoch_frame::DataFrame input = make_dataframe<double>(
        index,
        {
            {10.0, 20.0, 30.0}, // bbands_lower
            {15.0, 25.0, 35.0}, // bbands_middle
            {20.0, 30.0, 40.0}  // bbands_upper
        },
        {"bbands_lower", "bbands_middle", "bbands_upper"});

    // Evaluate row by row:
    // row0 => (20 - 10)/15 = 10/15 = 0.6667
    // row1 => (30 - 20)/25 = 10/25 = 0.4
    // row2 => (40 - 30)/35 = 10/35 ~ 0.2857
    epoch_frame::DataFrame expected = make_dataframe<double>(
        index, {{0.6667, 0.4, 0.2857}}, {config.GetOutputId()});

    auto result = model->TransformData(input);
    INFO("Comparing output with expected values\n"
         << result << "\n!=\n"
         << expected);
    REQUIRE(result.equals(expected, arrow::EqualOptions{}.atol(1e-2)));
  }

  SECTION("MovingAverage") {
    // Suppose we define a transform config for an SMA (or EMA, etc.)
    // We'll pretend the "type" is "SMA", "period"=2, "input"=CLOSE
    TransformConfiguration config{TransformDefinition{YAML::Load(R"(
type: ma
id: 1
timeframe: {interval: 1, type: day}
inputs: { "SLOT": c }
options: { period: 2, type: sma }
)")}};

    // Use registry to create the transform
    auto transformBase = MAKE_TRANSFORM(config);
    auto movAvg = dynamic_cast<ITransform *>(transformBase.get());

    // We'll define a small input with a "close" column
    // We expect a 2-day simple moving average.
    // close => [10, 20, 30, 40]
    // SMA(2) => [NaN, (10+20)/2, (20+30)/2, (30+40)/2] => [NaN,15,25,35]
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, January, 1d},
         epoch_frame::DateTime{2020y, January, 2d},
         epoch_frame::DateTime{2020y, January, 3d},
         epoch_frame::DateTime{2020y, January, 4d}});

    epoch_frame::DataFrame input =
        make_dataframe<double>(index, {{10.0, 20.0, 30.0, 40.0}},
                               {epoch_metadata::EpochStratifyXConstants::instance().CLOSE()});

    // We'll define the expected output column name from config.GetOutputId()
    // The transform internally uses TulipIndicatorModel =>
    // But we just check final rename => config ID => "202#result"
    // First value is NaN but will be dropped, so we expect values starting from
    // index 1
    std::vector<double> expectedMA{15.0, 25.0, 35.0};

    auto expectedIndex = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, January, 2d},
         epoch_frame::DateTime{2020y, January, 3d},
         epoch_frame::DateTime{2020y, January, 4d}});

    epoch_frame::DataFrame expected = make_dataframe<double>(
        expectedIndex, {expectedMA}, {config.GetOutputId()});

    // Test the transform
    auto output = movAvg->TransformData(input);
    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("GapReturns") {
    TransformConfiguration config =
        gap_returns("gap", epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Use registry to create the transform
    auto transformBase = MAKE_TRANSFORM(config);
    auto model = dynamic_cast<ITransform *>(transformBase.get());

    SECTION("Daily Data") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2018y, January, 1d},
           epoch_frame::DateTime{2018y, January, 2d},
           epoch_frame::DateTime{2018y, January, 3d}});

      epoch_frame::DataFrame input =
          make_dataframe<double>(index,
                                 {
                                     {100.0, 102.0, 108.0}, // CLOSE
                                     {101.0, 105.0, 110.0}  // OPEN
                                 },
                                 {epoch_metadata::EpochStratifyXConstants::instance().CLOSE(),
                                  epoch_metadata::BarsConstants::instance().OPEN()});

      std::vector<double> gapExpected{
          std::numeric_limits<double>::quiet_NaN(), // row0 => no previous row
                                                    // => NaN
          (105.0 - 100.0) / 100.0,                  // 0.05
          (110.0 - 102.0) / 102.0                   // ~0.0784314
      };

      epoch_frame::DataFrame expected =
          make_dataframe<double>(index, {gapExpected}, {config.GetOutputId()});

      auto result = model->TransformData(input);
      INFO("Comparing output with expected values\n"
           << result << "\n!=\n"
           << expected);
      REQUIRE(result.equals(expected));
    }

    SECTION("IntraDay Data") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2024y, September, 1d, 9h, 0min, 0s},
           epoch_frame::DateTime{2024y, September, 1d, 10h, 0min, 0s},
           epoch_frame::DateTime{2024y, September, 2d, 9h, 0min, 0s},
           epoch_frame::DateTime{2024y, September, 2d, 10h, 0min, 0s}});

      epoch_frame::DataFrame input =
          make_dataframe<double>(index,
                                 {
                                     {100.0, 101.0, 105.0, 110.0}, // CLOSE
                                     {101.0, 103.0, 106.0, 108.0}  // OPEN
                                 },
                                 {epoch_metadata::EpochStratifyXConstants::instance().CLOSE(),
                                  epoch_metadata::BarsConstants::instance().OPEN()});

      std::vector<double> gapExpected{
          std::numeric_limits<double>::quiet_NaN(), // row0 => no previous =>
                                                    // NaN
          std::numeric_limits<double>::quiet_NaN(), // row1 => same date => NaN
          (106.0 - 101.0) / 101.0,                  // 0.04950495
          std::numeric_limits<double>::quiet_NaN()  // row3 => same date => NaN
      };

      epoch_frame::DataFrame expected =
          make_dataframe<double>(index, {gapExpected}, {config.GetOutputId()});

      auto result = model->TransformData(input);
      INFO("Comparing output with expected values\n"
           << result << "\n!=\n"
           << expected);
      REQUIRE(result.equals(expected));
    }
  }
}
