//
// Created by adesola on 8/2/24.
//
#include "data/futures_continuation/adjustments/adjustments.h"
#include <epoch_script/data/common/constants.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>

using namespace epoch_script;
using namespace epoch_script::futures;
using namespace epoch_frame;
using namespace epoch_frame::factory;

namespace {
// Helper function to create test bar data
FuturesConstructedBars createTestBars(int nRows, double basePrice) {
  FuturesConstructedBars bars;

  // Initialize OHLC data
  bars.o.resize(nRows);
  bars.h.resize(nRows);
  bars.l.resize(nRows);
  bars.c.resize(nRows);
  bars.v.resize(nRows);
  bars.oi.resize(nRows);
  bars.t.resize(nRows);
  bars.s.resize(nRows);

  for (int i = 0; i < nRows; i++) {
    // Create realistic OHLC data with a slight trend
    double dayOffset = i * 0.5; // Small upward trend
    bars.o[i] = basePrice + dayOffset;
    bars.h[i] = basePrice + dayOffset + 2.0;  // High is 2 points above open
    bars.l[i] = basePrice + dayOffset - 1.5;  // Low is 1.5 points below open
    bars.c[i] = basePrice + dayOffset + 0.25; // Close is 0.25 points above open
    bars.v[i] = 1000.0 + i * 100;             // Volume increases each day
    bars.oi[i] = 5000.0 - i * 50; // Open interest decreases each day
    bars.t[i] =
        1672531200000 + i * 86400000; // Starting from 2023-01-01, adding days
    bars.s[i] =
        "CL" + std::to_string(i % 3 + 1); // Contract symbol (CL1, CL2, CL3)
  }

  return bars;
}
} // namespace

TEST_CASE("Adjustment Methods - Basic functionality", "[futures_adjustment]") {
  const int nRows = 10;

  // Create test data
  FuturesConstructedBars frontBars = createTestBars(nRows, 100.0);
  FuturesConstructedBars backBars =
      createTestBars(nRows, 110.0); // Back contract higher

  // Define roll points
  std::vector<int64_t> rollIndexes = {3, 7};

  SECTION("BackwardPanamaMethod produces expected adjusted DataFrame") {
    BackwardPanamaMethod method;

    // Call the adjustment method
    DataFrame result = method.AdjustContracts(frontBars, backBars, rollIndexes);

    // Verify basic properties of the result
    REQUIRE(result.num_rows() == nRows);
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().OPEN()));
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().HIGH()));
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().LOW()));
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().CLOSE()));

    // In a real backward adjustment, the most recent prices are preserved
    double lastClose =
        result
            .iloc(nRows - 1,
                  epoch_script::EpochStratifyXConstants::instance().CLOSE())
            .as_double();

    // Last price should match the front contract's last price
    REQUIRE(lastClose == Catch::Approx(frontBars.c[nRows - 1]).margin(0.01));

    // Verify non-price columns are included
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().VOLUME()));
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().OPEN_INTEREST()));
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().CONTRACT()));
  }

  SECTION("ForwardPanamaMethod produces expected adjusted DataFrame") {
    ForwardPanamaMethod method;

    // Call the adjustment method
    DataFrame result = method.AdjustContracts(frontBars, backBars, rollIndexes);

    // Verify basic properties of the result
    REQUIRE(result.num_rows() == nRows);
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().CLOSE()));

    // In a real forward adjustment, the earliest prices are preserved
    double firstClose =
        result
            .iloc(0,
                  epoch_script::EpochStratifyXConstants::instance().CLOSE())
            .as_double();

    // The first price should match the front contract's first price
    REQUIRE(firstClose == Catch::Approx(frontBars.c[0]).margin(0.01));

    // Verify non-price columns are included
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().VOLUME()));
  }

  SECTION("BackwardRatioMethod produces expected adjusted DataFrame") {
    BackwardRatioMethod method;

    // Call the adjustment method
    DataFrame result = method.AdjustContracts(frontBars, backBars, rollIndexes);

    // Verify basic properties of the result
    REQUIRE(result.num_rows() == nRows);
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().CLOSE()));

    // Last price should match the front contract's last price
    double lastClose =
        result
            .iloc(nRows - 1,
                  epoch_script::EpochStratifyXConstants::instance().CLOSE())
            .as_double();
    REQUIRE(lastClose == Catch::Approx(frontBars.c[nRows - 1]).margin(0.01));
  }

  SECTION("ForwardRatioMethod produces expected adjusted DataFrame") {
    ForwardRatioMethod method;

    // Call the adjustment method
    DataFrame result = method.AdjustContracts(frontBars, backBars, rollIndexes);

    // Verify basic properties of the result
    REQUIRE(result.num_rows() == nRows);
    REQUIRE(result.contains(
        epoch_script::EpochStratifyXConstants::instance().CLOSE()));

    // First price should match the front contract's first price
    double firstClose =
        result
            .iloc(0,
                  epoch_script::EpochStratifyXConstants::instance().CLOSE())
            .as_double();
    REQUIRE(firstClose == Catch::Approx(frontBars.c[0]).margin(0.01));
  }
}

TEST_CASE("Adjustment Methods - Type Verification", "[futures_adjustment]") {
  // Verify that each method returns the correct adjustment type
  BackwardPanamaMethod backwardPanama;
  REQUIRE(backwardPanama.GetType() ==
          epoch_core::AdjustmentType::BackwardPanamaCanal);

  ForwardPanamaMethod forwardPanama;
  REQUIRE(forwardPanama.GetType() ==
          epoch_core::AdjustmentType::ForwardPanamaCanal);

  BackwardRatioMethod backwardRatio;
  REQUIRE(backwardRatio.GetType() == epoch_core::AdjustmentType::BackwardRatio);

  ForwardRatioMethod forwardRatio;
  REQUIRE(forwardRatio.GetType() == epoch_core::AdjustmentType::ForwardRatio);
}
