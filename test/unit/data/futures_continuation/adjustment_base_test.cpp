//
// Created by adesola on 8/2/24.
//
#include "data/futures_continuation/adjustments/adjustment_base.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>

#include "data/futures_continuation/adjustments/adjustments.h"

using namespace epoch_script;
using namespace epoch_script::futures;
using namespace epoch_frame;
using namespace epoch_script;
using namespace epoch_frame::factory;

TEST_CASE("AdjustmentMethodBase - CalculateRollIndexRanges",
          "[adjustment_base]") {
  SECTION(
      "Empty rollIndexes returns a single range covering the entire dataset") {
    std::vector<int64_t> rollIndexes = {};
    int64_t nRows = 100;

    auto result =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndexes, nRows);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == nRows);
  }

  SECTION("Single roll index creates two ranges") {
    std::vector<int64_t> rollIndexes = {50};
    int64_t nRows = 100;

    auto result =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndexes, nRows);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 50);
    REQUIRE(result[1].first == 50);
    REQUIRE(result[1].second == 50);
  }

  SECTION("Multiple roll indexes create correct ranges") {
    std::vector<int64_t> rollIndexes = {20, 40, 70};
    int64_t nRows = 100;

    auto result =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndexes, nRows);

    REQUIRE(result.size() == 4);
    // First segment: [0, 20)
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 20);

    // Second segment: [20, 40)
    REQUIRE(result[1].first == 20);
    REQUIRE(result[1].second == 20);

    // Third segment: [40, 70)
    REQUIRE(result[2].first == 40);
    REQUIRE(result[2].second == 30);

    // Fourth segment: [70, 100)
    REQUIRE(result[3].first == 70);
    REQUIRE(result[3].second == 30);
  }

  SECTION("Roll index at start creates correct ranges") {
    std::vector<int64_t> rollIndexes = {0, 50};
    int64_t nRows = 100;

    auto result =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndexes, nRows);

    REQUIRE(result.size() == 3);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 0);
    REQUIRE(result[1].first == 0);
    REQUIRE(result[1].second == 50);
    REQUIRE(result[2].first == 50);
    REQUIRE(result[2].second == 50);
  }

  SECTION("Roll index at end creates correct ranges") {
    std::vector<int64_t> rollIndexes = {50, 99};
    int64_t nRows = 100;

    auto result =
        AdjustmentMethodBase::CalculateRollIndexRanges(rollIndexes, nRows);

    REQUIRE(result.size() == 3);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 50);
    REQUIRE(result[1].first == 50);
    REQUIRE(result[1].second == 49);
    REQUIRE(result[2].first == 99);
    REQUIRE(result[2].second == 1);
  }
}

TEST_CASE("AdjustmentMethodBase - PrepareBarsContainer", "[adjustment_base]") {
  SECTION("Container initialized with correct size") {
    int64_t nRows = 100;

    auto bars = AdjustmentMethodBase::PrepareBarsContainer(nRows);

    // Check that bars has all the necessary vectors initialized with the
    // correct size
    for (BarAttribute::Type barAttributeType :
         AdjustmentMethodBase::g_adjustedAttributeType) {
      REQUIRE(bars[barAttributeType].size() == nRows);
    }
  }

  SECTION("Empty container for zero rows") {
    int64_t nRows = 0;

    auto bars = AdjustmentMethodBase::PrepareBarsContainer(nRows);

    // Check that bars has all the necessary vectors initialized with size 0
    for (BarAttribute::Type barAttributeType :
         AdjustmentMethodBase::g_adjustedAttributeType) {
      REQUIRE(bars[barAttributeType].size() == 0);
    }
  }
}

TEST_CASE("AdjustmentMethodBase - ConstructAdjustedTable",
          "[adjustment_base]") {
  SECTION("Table construction with sample data") {
    const int64_t nRows = 3;

    // Create sample data
    FuturesConstructedBars bars =
        AdjustmentMethodBase::PrepareBarsContainer(nRows);
    FuturesConstructedBars unAdjustedFrontBarData =
        AdjustmentMethodBase::PrepareBarsContainer(nRows);

    // Fill sample data for bars (adjusted prices)
    bars[BarAttribute::Type::Open] = {100.0, 101.0, 102.0};
    bars[BarAttribute::Type::High] = {105.0, 106.0, 107.0};
    bars[BarAttribute::Type::Low] = {95.0, 96.0, 97.0};
    bars[BarAttribute::Type::Close] = {103.0, 104.0, 105.0};

    // Fill sample data for unAdjustedFrontBarData (original data including
    // volume, OI, contract)
    unAdjustedFrontBarData[BarAttribute::Type::Open] = {200.0, 201.0, 202.0};
    unAdjustedFrontBarData[BarAttribute::Type::High] = {205.0, 206.0, 207.0};
    unAdjustedFrontBarData[BarAttribute::Type::Low] = {195.0, 196.0, 197.0};
    unAdjustedFrontBarData[BarAttribute::Type::Close] = {203.0, 204.0, 205.0};
    unAdjustedFrontBarData[BarAttribute::Type::Volume] = {1000.0, 1100.0,
                                                          1200.0};
    unAdjustedFrontBarData[BarAttribute::Type::OpenInterest] = {5000.0, 5100.0,
                                                                5200.0};
    unAdjustedFrontBarData.s = {"CL1", "CL2", "CL3"};

    // Construct the table
    auto table = AdjustmentMethodBase::ConstructAdjustedTable(
        bars, unAdjustedFrontBarData);

    // Verify table structure
    REQUIRE(table->num_rows() == nRows);
    REQUIRE(table->num_columns() ==
            AdjustmentMethodBase::g_adjustedAttributeType.size() +
                AdjustmentMethodBase::g_unAdjustedAttributeType.size());

    // Verify price columns from bars
    auto openCol = table->GetColumnByName(
        epoch_script::BarAttribute::fromType(BarAttribute::Type::Open));
    auto highCol = table->GetColumnByName(
        epoch_script::BarAttribute::fromType(BarAttribute::Type::High));
    auto lowCol =
        table->GetColumnByName(BarAttribute::fromType(BarAttribute::Type::Low));
    auto closeCol = table->GetColumnByName(
        epoch_script::BarAttribute::fromType(BarAttribute::Type::Close));

    REQUIRE(openCol != nullptr);
    REQUIRE(highCol != nullptr);
    REQUIRE(lowCol != nullptr);
    REQUIRE(closeCol != nullptr);

    // Verify non-price columns from unAdjustedFrontBarData
    auto volumeCol = table->GetColumnByName(
        epoch_script::BarAttribute::fromType(BarAttribute::Type::Volume));
    auto oiCol = table->GetColumnByName(epoch_script::BarAttribute::fromType(
        BarAttribute::Type::OpenInterest));
    auto contractCol = table->GetColumnByName(
        epoch_script::BarAttribute::fromType(BarAttribute::Type::Contract));

    REQUIRE(volumeCol != nullptr);
    REQUIRE(oiCol != nullptr);
    REQUIRE(contractCol != nullptr);

    // Convert to DataFrame and check values (if needed for deeper validation)
    DataFrame df(table);
    for (int i = 0; i < nRows; i++) {
      // Check adjusted price values
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::Open))
                  .as_double() == bars[BarAttribute::Type::Open][i]);
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::High))
                  .as_double() == bars[BarAttribute::Type::High][i]);
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::Low))
                  .as_double() == bars[BarAttribute::Type::Low][i]);
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::Close))
                  .as_double() == bars[BarAttribute::Type::Close][i]);

      // Check unadjusted attribute values
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::Volume))
                  .as_double() ==
              unAdjustedFrontBarData[BarAttribute::Type::Volume][i]);
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::OpenInterest))
                  .as_double() ==
              unAdjustedFrontBarData[BarAttribute::Type::OpenInterest][i]);
      REQUIRE(df.iloc(i, epoch_script::BarAttribute::fromType(
                             BarAttribute::Type::Contract))
                  .repr() == unAdjustedFrontBarData.s[i]);
    }
  }
}

// Test for the GenericAdjustmentMethod implementation
TEST_CASE("GenericAdjustmentMethod - Type getters", "[adjustment_base]") {
  // Testing the GetType() methods of different adjustment implementations
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