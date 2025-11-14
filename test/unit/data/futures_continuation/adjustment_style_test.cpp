//
// Created by dewe on 12/5/23.
//
#include "data/futures_continuation/adjustments/style/adjustment_style.h"
#include "catch.hpp"

using namespace epoch_script::futures;

TEST_CASE("PanamaCanal Initialization", "[AdjustmentStyle]") {
  PanamaCanal method;
  REQUIRE(method.GetAdjustmentFactor() == 0);
  REQUIRE(method.GetAccumulatedAdjFactor() == 0);
}

TEST_CASE("Ratio Initialization", "[AdjustmentStyle]") {
  Ratio method;
  REQUIRE(method.GetAdjustmentFactor() == 1);
  REQUIRE(method.GetAccumulatedAdjFactor() == 1);
}

TEST_CASE("ComputeAdjustmentFactor PanamaCanal", "[AdjustmentStyle]") {
  PanamaCanal method;
  method.ComputeAdjustmentFactor(100, 105);
  REQUIRE(method.GetAdjustmentFactor() == 5);
}

TEST_CASE("ComputeAdjustmentFactor Ratio", "[AdjustmentStyle]") {
  Ratio method;
  method.ComputeAdjustmentFactor(100, 110);
  REQUIRE(method.GetAdjustmentFactor() == Catch::Approx(1.1));
}

TEST_CASE("ApplyAdjustment PanamaCanal", "[AdjustmentStyle]") {
  PanamaCanal method;
  method.ComputeAdjustmentFactor(100, 105);
  REQUIRE(method.ApplyAdjustment(50) == 55);
}

TEST_CASE("ApplyAdjustment Ratio", "[AdjustmentStyle]") {
  Ratio method;
  method.ComputeAdjustmentFactor(100, 120);
  REQUIRE(method.ApplyAdjustment(50) == Catch::Approx(60));
}

TEST_CASE("ApplyCumulativeAdjustment PanamaCanal", "[AdjustmentStyle]") {
  PanamaCanal method;
  method.ComputeAdjustmentFactor(100, 105);
  method.ComputeAdjustmentFactor(105, 110);
  REQUIRE(method.ApplyCumulativeAdjustment(50) == 60);
}

TEST_CASE("ApplyCumulativeAdjustment Ratio", "[AdjustmentStyle]") {
  Ratio method;
  method.ComputeAdjustmentFactor(100, 120);
  method.ComputeAdjustmentFactor(120, 144);
  REQUIRE(method.ApplyCumulativeAdjustment(50) == Catch::Approx(72));
}
