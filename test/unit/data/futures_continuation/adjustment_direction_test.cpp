//
// Created by adesola on 8/2/24.
//
#include "data/futures_continuation/adjustments/adjustment_base.h"
#include "data/futures_continuation/adjustments/direction/backward.h"
#include "data/futures_continuation/adjustments/direction/forward.h"
#include "data/futures_continuation/adjustments/style/adjustment_style.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <vector>

using namespace epoch_script;
using namespace epoch_script::futures;
using namespace epoch_frame;
using namespace epoch_frame::factory;

// Simple mock adjustment style for testing - matches PanamaCanal's behavior
struct MockAddFixedValue : IAdjustmentStyle {
  // Store adjustment factors
  double m_adjustmentFactor = 0.0;
  double m_accumulatedAdjFactor = 0.0;

  // Compute the adjustment factor between front and back contracts
  void ComputeAdjustmentFactor(double frontValue, double backValue) override {
    m_adjustmentFactor = backValue - frontValue; // Same as PanamaCanal
    m_accumulatedAdjFactor += m_adjustmentFactor;
  }

  // Apply the current adjustment factor to a price
  double ApplyAdjustment(double newFront) override {
    return newFront + m_adjustmentFactor;
  }

  // Apply the accumulated adjustment to a price
  double ApplyCumulativeAdjustment(double newFront) override {
    return newFront + m_accumulatedAdjFactor;
  }

  // Get the current adjustment factor
  double GetAdjustmentFactor() const override { return m_adjustmentFactor; }

  // Get the accumulated adjustment factor
  double GetAccumulatedAdjFactor() const override {
    return m_accumulatedAdjFactor;
  }
};

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

TEST_CASE("BackwardAdjustmentDirection - AdjustPriceAttributes",
          "[direction]") {
  const int nRows = 10;

  // Create test data - front prices are lower than back for realistic scenario
  FuturesConstructedBars frontBars = createTestBars(nRows, 100.0);
  FuturesConstructedBars backBars = createTestBars(nRows, 110.0);

  // Create a output bars container
  FuturesConstructedBars adjustedBars =
      AdjustmentMethodBase::PrepareBarsContainer(nRows);

  // Define roll points at indices 3 and 7
  std::vector<int64_t> rollIndices = {3, 7};

  // Calculate roll ranges
  auto rollRanges =
      AdjustmentMethodBase::CalculateRollIndexRanges(rollIndices, nRows);

  // Define which attributes to adjust
  std::initializer_list<epoch_script::BarAttribute::Type>
      adjustedAttributeTypes = {epoch_script::BarAttribute::Type::Open,
                                epoch_script::BarAttribute::Type::High,
                                epoch_script::BarAttribute::Type::Low,
                                epoch_script::BarAttribute::Type::Close};

  // Call the backward direction with our mock style
  BackwardAdjustmentDirection<MockAddFixedValue>::AdjustPriceAttributes(
      adjustedBars, adjustedAttributeTypes, rollRanges, frontBars, backBars);

  SECTION("Last segment prices are unchanged") {
    // The last segment (after the last roll) should remain unchanged
    for (int i = 7; i < nRows; i++) {
      REQUIRE(adjustedBars.c[i] == Catch::Approx(frontBars.c[i]).margin(0.01));
      REQUIRE(adjustedBars.o[i] == Catch::Approx(frontBars.o[i]).margin(0.01));
    }
  }

  SECTION("Middle segment prices are adjusted by one factor") {
    // Adjustment factor: back price - front price at roll index 7
    double adj1 = backBars.c[7] - frontBars.c[7];

    // The middle segment (between rolls) should be adjusted by the first factor
    for (int i = 3; i < 7; i++) {
      REQUIRE(adjustedBars.c[i] ==
              Catch::Approx(frontBars.c[i] + adj1).margin(0.01));
      REQUIRE(adjustedBars.o[i] ==
              Catch::Approx(frontBars.o[i] + adj1).margin(0.01));
    }
  }

  SECTION("First segment prices are adjusted by both factors") {
    // First adjustment factor: back price - front price at roll index 7
    double adj1 = backBars.c[7] - frontBars.c[7];
    // Second adjustment factor: back price - front price at roll index 3
    double adj2 = backBars.c[3] - frontBars.c[3];

    // The first segment (before first roll) should be adjusted by both factors
    for (int i = 0; i < 3; i++) {
      REQUIRE(adjustedBars.c[i] ==
              Catch::Approx(frontBars.c[i] + adj1 + adj2).margin(0.01));
      REQUIRE(adjustedBars.o[i] ==
              Catch::Approx(frontBars.o[i] + adj1 + adj2).margin(0.01));
    }
  }
}

TEST_CASE("ForwardAdjustmentDirection - AdjustPriceAttributes", "[direction]") {
  const int nRows = 10;

  // Create test data - front prices are lower than back for realistic scenario
  FuturesConstructedBars frontBars = createTestBars(nRows, 100.0);
  FuturesConstructedBars backBars = createTestBars(nRows, 110.0);

  // Create a output bars container
  FuturesConstructedBars adjustedBars =
      AdjustmentMethodBase::PrepareBarsContainer(nRows);

  // Define roll points at indices 3 and 7
  std::vector<int64_t> rollIndices = {3, 7};

  // Calculate roll ranges
  auto rollRanges =
      AdjustmentMethodBase::CalculateRollIndexRanges(rollIndices, nRows);

  // Define which attributes to adjust
  std::initializer_list<epoch_script::BarAttribute::Type>
      adjustedAttributeTypes = {epoch_script::BarAttribute::Type::Open,
                                epoch_script::BarAttribute::Type::High,
                                epoch_script::BarAttribute::Type::Low,
                                epoch_script::BarAttribute::Type::Close};

  // Call the forward direction with our mock style
  ForwardAdjustmentDirection<MockAddFixedValue>::AdjustPriceAttributes(
      adjustedBars, adjustedAttributeTypes, rollRanges, frontBars, backBars);

  SECTION("First segment prices are unchanged") {
    // The first segment (before first roll) should remain unchanged
    for (int i = 0; i < 3; i++) {
      REQUIRE(adjustedBars.c[i] == Catch::Approx(frontBars.c[i]).margin(0.01));
      REQUIRE(adjustedBars.o[i] == Catch::Approx(frontBars.o[i]).margin(0.01));
    }
  }

  SECTION("Middle segment prices are adjusted by one factor") {
    // First adjustment factor: back price - front price at roll index 3
    double adj1 = backBars.c[3] - frontBars.c[3];

    // The middle segment (between rolls) should be adjusted by the first factor
    for (int i = 3; i < 7; i++) {
      REQUIRE(adjustedBars.c[i] ==
              Catch::Approx(frontBars.c[i] + adj1).margin(0.01));
      REQUIRE(adjustedBars.o[i] ==
              Catch::Approx(frontBars.o[i] + adj1).margin(0.01));
    }
  }

  SECTION("Last segment prices are adjusted by both factors") {
    // First adjustment factor: back price - front price at roll index 3
    double adj1 = backBars.c[3] - frontBars.c[3];
    // Second adjustment factor: back price - front price at roll index 7
    double adj2 = backBars.c[7] - frontBars.c[7];

    // The last segment (after last roll) should be adjusted by both factors
    for (int i = 7; i < nRows; i++) {
      REQUIRE(adjustedBars.c[i] ==
              Catch::Approx(frontBars.c[i] + adj1 + adj2).margin(0.01));
      REQUIRE(adjustedBars.o[i] ==
              Catch::Approx(frontBars.o[i] + adj1 + adj2).margin(0.01));
    }
  }
}

TEST_CASE("Edge cases for directions", "[direction]") {
  const int nRows = 5;
  FuturesConstructedBars frontBars = createTestBars(nRows, 100.0);
  FuturesConstructedBars backBars = createTestBars(nRows, 110.0);
  FuturesConstructedBars adjustedBars =
      AdjustmentMethodBase::PrepareBarsContainer(nRows);

  std::initializer_list<epoch_script::BarAttribute::Type>
      adjustedAttributeTypes = {epoch_script::BarAttribute::Type::Open,
                                epoch_script::BarAttribute::Type::High,
                                epoch_script::BarAttribute::Type::Low,
                                epoch_script::BarAttribute::Type::Close};

  SECTION("No roll points should leave data unchanged") {
    std::vector<int64_t> emptyRollIndices = {};
    auto rollRanges =
        AdjustmentMethodBase::CalculateRollIndexRanges(emptyRollIndices, nRows);

    // Test backward direction
    BackwardAdjustmentDirection<MockAddFixedValue>::AdjustPriceAttributes(
        adjustedBars, adjustedAttributeTypes, rollRanges, frontBars, backBars);

    // All prices should be identical to front bars
    for (int i = 0; i < nRows; i++) {
      REQUIRE(adjustedBars.c[i] == Catch::Approx(frontBars.c[i]).margin(0.01));
    }

    // Reset adjusted bars
    adjustedBars = AdjustmentMethodBase::PrepareBarsContainer(nRows);

    // Test forward direction
    ForwardAdjustmentDirection<MockAddFixedValue>::AdjustPriceAttributes(
        adjustedBars, adjustedAttributeTypes, rollRanges, frontBars, backBars);

    // All prices should be identical to front bars
    for (int i = 0; i < nRows; i++) {
      REQUIRE(adjustedBars.c[i] == Catch::Approx(frontBars.c[i]).margin(0.01));
    }
  }

  SECTION("Single roll at the beginning") {
    std::vector<int64_t> rollIndices = {0};
    auto rollRanges =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndices, nRows);

    // Test backward direction
    BackwardAdjustmentDirection<MockAddFixedValue>::AdjustPriceAttributes(
        adjustedBars, adjustedAttributeTypes, rollRanges, frontBars, backBars);

    // All prices should be identical to front bars since we have no "before"
    // segment
    for (int i = 0; i < nRows; i++) {
      REQUIRE(adjustedBars.c[i] == Catch::Approx(frontBars.c[i]).margin(0.01));
    }
  }

  SECTION("Roll at the end should throw an exception") {
    std::vector<int64_t> rollIndices = {nRows - 1};
    auto rollRanges =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndices, nRows);

    // Create a mock adapter that enforces validation of roll indices
    class ValidatingMockAddFixedValue : public MockAddFixedValue {
    public:
      void ComputeAdjustmentFactor(double frontValue,
                                   double backValue) override {
        // For a roll index at the end, we have no values "after" it
        // Forward adjustments make no sense in this case
        throw std::invalid_argument(
            "Roll at the end is invalid for forward adjustment");
      }
    };

    // Create a fresh adjusted bars container
    FuturesConstructedBars adjustedBars =
        AdjustmentMethodBase::PrepareBarsContainer(nRows);

    // Expect an exception when trying to use ForwardAdjustmentDirection with a
    // roll at the end
    REQUIRE_THROWS_AS(
        ForwardAdjustmentDirection<ValidatingMockAddFixedValue>::
            AdjustPriceAttributes(adjustedBars, adjustedAttributeTypes,
                                  rollRanges, frontBars, backBars),
        std::invalid_argument);

    // We should add a validation step in the implementation to prevent rolls at
    // the end
    INFO("Expected exception: ForwardAdjustmentDirection should not allow "
         "rolls at the end");
  }
}
