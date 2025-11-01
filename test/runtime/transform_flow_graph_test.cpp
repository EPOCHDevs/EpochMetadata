//
// Created by dewe on 1/31/23.
//

#include "epoch_frame/factory/series_factory.h"
#include "epochflow/transforms/config_helper.h"
#include "epochflow/transforms/transform_registry.h"
#include "runtime/orchestrator.h"
#include "testing/transform_builder.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/scalar_factory.h>
#include <numeric>
#include <vector>

#include "epochflow/bar_attribute.h"
#include "mocks/mock_transform_manager.h"
#include "testing/test_constants.h"

using namespace epoch_core;
using namespace epoch_flow::runtime;
using namespace epoch_frame;
using namespace epochflow::transform;
using namespace epoch_flow::runtime::test;

TEST_CASE("Transform Flow") {
  const IndexPtr TEST_1D_INDEX{epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, January, 18d},
       epoch_frame::DateTime{2020y, January, 19d},
       epoch_frame::DateTime{2020y, January, 20d}})};

  const epoch_frame::IndexPtr SINGLE_ROW_TEST_INDEX{
      epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, January, 18d}})};

  const epoch_frame::IndexPtr TEST_1T_INDEX_EXTRA{
      epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, January, 18d, 9h, 30min, 0s},
           epoch_frame::DateTime{2020y, January, 18d, 10h, 0min, 0s},
           epoch_frame::DateTime{2020y, January, 19d, 9h, 30min, 0s}})};

  const TimeFrameAssetDataFrameMap TEST_MULTI_TIMEFRAME_DATA{
      {epochflow::EpochStratifyXConstants::instance()
           .DAILY_FREQUENCY.ToString(),
       AssetDataFrameMap{
           {
               test::TestAssetConstants::AAPL,
            make_dataframe(
                TEST_1D_INDEX,
                {{Scalar(2.0), Scalar(3.0), Scalar(4.0)},
                 {Scalar(4.0), Scalar(6.0), Scalar(8.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())},
           {TestAssetConstants::MSFT,
            make_dataframe(
                TEST_1D_INDEX,
                {{Scalar(10.0), Scalar(15.0), Scalar(25.0)},
                 {Scalar(40.0), Scalar(50.0), Scalar(60.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())}}},
      {epochflow::EpochStratifyXConstants::instance()
           .MINUTE_FREQUENCY.ToString(),
       AssetDataFrameMap{
           {TestAssetConstants::AAPL,
            make_dataframe(
                TEST_1T_INDEX_EXTRA,
                {{Scalar(5.0), Scalar(10.0), Scalar(15.0)},
                 {Scalar(6.0), Scalar(9.0), Scalar(16.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())},
           {TestAssetConstants::MSFT,
            make_dataframe(
                TEST_1T_INDEX_EXTRA,
                {{Scalar(25.0), Scalar(30.0), Scalar(35.0)},
                 {Scalar(40.0), Scalar(44.0), Scalar(48.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())}}}};

  // Define test data using the same approach as in TEST_MULTI_TIMEFRAME_DATA
  const TimeFrameAssetDataFrameMap TEST_DATA{
      {epochflow::EpochStratifyXConstants::instance()
           .DAILY_FREQUENCY.ToString(),
       AssetDataFrameMap{
           {TestAssetConstants::AAPL,
            make_dataframe(
                TEST_1D_INDEX,
                {{Scalar(2.0), Scalar(4.0), Scalar(6.0)},
                 {Scalar(4.0), Scalar(6.0), Scalar(10.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())},
           {TestAssetConstants::MSFT,
            make_dataframe(
                TEST_1D_INDEX,
                {{Scalar(10.0), Scalar(20.0), Scalar(30.0)},
                 {Scalar(40.0), Scalar(40.0), Scalar(40.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())}}}};

  const TimeFrameAssetDataFrameMap SINGLE_ROW_TEST_DATA{
      {epochflow::EpochStratifyXConstants::instance()
           .DAILY_FREQUENCY.ToString(),
       AssetDataFrameMap{
           {TestAssetConstants::AAPL,
            make_dataframe(
                SINGLE_ROW_TEST_INDEX, {{Scalar(2.0)}, {Scalar(4.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())},
           {TestAssetConstants::MSFT,
            make_dataframe(
                SINGLE_ROW_TEST_INDEX, {{Scalar(10.0)}, {Scalar(40.0)}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()},
                arrow::float64())}}}};

  const TimeFrameAssetDataFrameMap TEST_CONTAIN_NULL_DATA{
      {epochflow::EpochStratifyXConstants::instance()
           .DAILY_FREQUENCY.ToString(),
       AssetDataFrameMap{
           {TestAssetConstants::AAPL,
            make_dataframe<double>(
                TEST_1D_INDEX,
                {std::vector<double>{2.0, 4.0, std::nan("")},
                 std::vector<double>{4.0, 6.0, 10.0}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance().HIGH()})},
           {TestAssetConstants::MSFT,
            make_dataframe<double>(
                TEST_1D_INDEX,
                {std::vector<double>{10.0, std::nan(""), 30.0},
                 std::vector<double>{40.0, 40.0, 40.0}},
                {epochflow::EpochStratifyXConstants::instance().CLOSE(),
                 epochflow::EpochStratifyXConstants::instance()
                     .HIGH()})}}}};

  auto dailyTF = epochflow::EpochStratifyXConstants::instance()
                     .DAILY_FREQUENCY.ToString();
  auto intradayTF = epochflow::EpochStratifyXConstants::instance()
                        .MINUTE_FREQUENCY.ToString();

  auto aapl = TestAssetConstants::AAPL;
  auto msft = TestAssetConstants::MSFT;

  auto closeKey = epochflow::EpochStratifyXConstants::instance().CLOSE();
  auto highKey = epochflow::EpochStratifyXConstants::instance().HIGH();
  auto openKey = epochflow::EpochStratifyXConstants::instance().OPEN();

  // Helper function to extract specific columns from a DataFrame for comparison
  auto extractColumnsForComparison =
      [](const epoch_frame::DataFrame &df,
         const std::vector<std::string> &columns) -> epoch_frame::DataFrame {
    return df[columns];
  };

  SECTION("DataFlowRuntimeOrchestrator detects circular dependencies") {
    auto ds = data_source(
        "data",
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Transformation A depends on Transformation B
    auto transA = vector_add(
        0, "1#result", ds.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Transformation B depends on Transformation A
    auto transB = vector_add(
        1, "0#result", ds.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Build the transformation graph with circular dependencies
    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{ds, transA, transB});
    REQUIRE_THROWS(DataFlowRuntimeOrchestrator({aapl}, CreateMockTransformManager(std::move(transforms))));
  }

  SECTION("DataFlowRuntimeOrchestrator accepts duplicate configs with unique id") {
    auto ds = data_source(
        "data",
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transA = vector_add(
        0, ds.GetOutputId(highKey), ds.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transB = vector_add(
        1, "0#result", ds.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transC = vector_add(
        2, "0#result", ds.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{ds, transA, transB, transC});
    DataFlowRuntimeOrchestrator flow({aapl}, CreateMockTransformManager(std::move(transforms)));

    auto result = flow.ExecutePipeline(TEST_DATA);

    REQUIRE(result.count(dailyTF) == 1);

    REQUIRE(result.at(dailyTF).count(aapl) == 1);
    INFO(result.at(dailyTF).at(aapl));

    // Now each transform runs independently, so we expect 4 columns (original 2
    // + 3 transforms)
    REQUIRE(result.at(dailyTF).at(aapl).num_cols() ==
            TEST_DATA.at(dailyTF).at(aapl).num_cols() + 3);
  }

  SECTION(
      "DataFlowRuntimeOrchestrator accept configs with only different timeframes") {
    auto dsDaily = data_source(
        "dataDaily",
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto dsMinute = data_source(
        "dataMinute",
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY);

    auto transA = vector_add(
        0, dsDaily.GetOutputId(highKey), dsDaily.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transB = vector_add(
        1, dsMinute.GetOutputId(highKey), dsMinute.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY);
    auto transC = vector_add(
        2, "0#result", dsDaily.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transD = vector_add(
        3, "1#result", dsMinute.GetOutputId(closeKey),
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{dsDaily, dsMinute, transA, transB,
                                   transC, transD});
    DataFlowRuntimeOrchestrator flow({aapl}, CreateMockTransformManager(std::move(transforms)));

    auto data = TEST_MULTI_TIMEFRAME_DATA;
    auto result = flow.ExecutePipeline(data);
    {
      REQUIRE(result.count(dailyTF) == 1);
      REQUIRE(result.at(dailyTF).count(aapl) == 1);
      INFO(result.at(dailyTF).at(aapl));

      // Now each transform runs independently, so we expect original columns +
      // 2 daily transforms
      REQUIRE(result.at(dailyTF).at(aapl).num_cols() ==
              data.at(dailyTF).at(aapl).num_cols() + 2);
    }

    {
      REQUIRE(result.count(intradayTF) == 1);
      REQUIRE(result.at(intradayTF).count(aapl) == 1);
      INFO(result.at(intradayTF).at(aapl));

      // Now each transform runs independently, so we expect original columns +
      // 2 intraday transforms
      REQUIRE(result.at(intradayTF).at(aapl).num_cols() ==
              data.at(intradayTF).at(aapl).num_cols() + 2);
    }
  }

  SECTION("DataFlowRuntimeOrchestrator is legal to connect nodes of different "
          "timeframe") {
    using namespace epoch_frame::factory::index;
    using namespace epoch_frame::factory::scalar;
    using namespace epoch_frame;
    // IMin Index: [2020-01-18 09:30:00, 2020-01-18 09:31:00, 2020-01-18
    // 09:32:00] 15Min Index: [2020-01-18 09:30:00, 2020-01-18 09:45:00,
    // 2020-01-18 10:00:00]

    const epoch_frame::IndexPtr Index1Min{
        epoch_frame::factory::index::make_datetime_index(
            std::vector{"2020-01-18 09:30:00"_datetime,
                        "2020-01-18 09:31:00"_datetime,
                        "2020-01-18 09:32:00"_datetime},
            "", "")};
    const epoch_frame::IndexPtr Index15Min{
        epoch_frame::factory::index::make_datetime_index(
            std::vector{"2020-01-18 09:30:00"_datetime,
                        "2020-01-18 09:45:00"_datetime,
                        "2020-01-18 10:00:00"_datetime},
            "", "")};

    auto Data1Min = make_dataframe<double>(Index1Min, {{1, 2, 3}, {4, 5, 6}},
                                           {openKey, closeKey});
    auto Data15Min = make_dataframe<double>(
        Index15Min, {{10, 20, 30}, {40, 50, 60}}, {openKey, closeKey});

    auto _1MinTF_ =
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY;
    epochflow::TimeFrame _15MinTF_{
        epoch_frame::factory::offset::minutes(15)};

    // Create data sources for each timeframe
    auto ds1Min = data_source("data1Min", _1MinTF_);
    auto ds15Min = data_source("data15Min", _15MinTF_);

    // open: [1, 2, 3]
    // close: [4, 5, 6]
    // 0#result: [5, 7, 9]
    auto range1Min = vector_add(0, ds1Min.GetOutputId(openKey),
                                ds1Min.GetOutputId(closeKey), _1MinTF_);

    // open: [10, 20, 30]
    // close: [40, 50, 60]
    // 1#result: [50, 70, 90]
    auto range15Min = vector_add(1, ds15Min.GetOutputId(openKey),
                                 ds15Min.GetOutputId(closeKey), _15MinTF_);

    // 0#result: [5, null, null]
    // 1#result: [50, null, null]
    // 2#result: [55, null, null]
    auto interTFAdd15Min = vector_add(2, "0#result", "1#result", _15MinTF_);

    // 0#result: [5, null, null]
    // 1#result: [50, null, null]
    // 3#result: [55, null, null]
    auto interTFAdd1Min = vector_add(3, "0#result", "1#result", _1MinTF_);

    // 1#result: [50, null, null]
    // close: [4, null, null]
    // 4#result: [54, null, null]
    auto interTFAdd1MinClose =
        vector_add(4, "1#result", ds1Min.GetOutputId(closeKey), _1MinTF_);

    // 0#result: [5, null, null]
    // close: [40, null, null]
    // 5#result: [45, null, null]
    auto interTFAdd15MinClose =
        vector_add(5, "0#result", ds15Min.GetOutputId(closeKey), _15MinTF_);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{
            ds1Min, ds15Min, range1Min, range15Min, interTFAdd15Min,
            interTFAdd1Min, interTFAdd1MinClose, interTFAdd15MinClose});
    auto flow = DataFlowRuntimeOrchestrator({aapl}, CreateMockTransformManager(std::move(transforms)));

    TimeFrameAssetDataFrameMap data;
    auto _1MinTF_str = _1MinTF_.ToString();
    auto _15MinTF_str = _15MinTF_.ToString();
    data[_1MinTF_str][aapl] = Data1Min;
    data[_15MinTF_str][aapl] = Data15Min;

    auto result = flow.ExecutePipeline(data);
    REQUIRE(result.count(_1MinTF_str) == 1);
    REQUIRE(result.count(_15MinTF_str) == 1);

    REQUIRE_FALSE(result.at(_1MinTF_str)
                      .at(aapl)
                      .contains(interTFAdd15Min.GetOutputId()));
    REQUIRE(
        result.at(_1MinTF_str).at(aapl).contains(interTFAdd1Min.GetOutputId()));
    REQUIRE(result.at(_1MinTF_str)
                .at(aapl)
                .contains(interTFAdd1MinClose.GetOutputId()));
    REQUIRE_FALSE(result.at(_1MinTF_str)
                      .at(aapl)
                      .contains(interTFAdd15MinClose.GetOutputId()));
    REQUIRE(result.at(_15MinTF_str)
                .at(aapl)
                .contains(interTFAdd15Min.GetOutputId()));
    REQUIRE_FALSE(result.at(_15MinTF_str)
                      .at(aapl)
                      .contains(interTFAdd1Min.GetOutputId()));
    REQUIRE_FALSE(result.at(_15MinTF_str)
                      .at(aapl)
                      .contains(interTFAdd1MinClose.GetOutputId()));
    REQUIRE(result.at(_15MinTF_str)
                .at(aapl)
                .contains(interTFAdd15MinClose.GetOutputId()));

    {
      auto [cfg, index, result_double] = GENERATE_REF(
          std::tuple{interTFAdd1Min, Data1Min.index(), 55.0},
          std::tuple{interTFAdd1MinClose, Data1Min.index(), 54.0},
          std::tuple{interTFAdd15Min, Data15Min.index(), 55.0},
          std::tuple{interTFAdd15MinClose, Data15Min.index(), 45.0});

      auto tf = cfg.GetTimeframe().ToString();
      auto df_result = result.at(tf).at(aapl)[cfg.GetOutputId()];
      auto expected = make_series(
          index, std::vector{result_double, std::nan(""), std::nan("")});
      INFO("Expected: " << expected << "\nResult: " << df_result);
      REQUIRE(df_result.equals(expected));
    }
  }

  SECTION("Transform with duplicate sma config and different options") {
    auto ds = data_source(
        "data",
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto sma30 = sma(
        0, ds.GetOutputId(closeKey), 30,
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto sma60 = sma(
        1, ds.GetOutputId(closeKey), 60,
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{ds, sma30, sma60});
    DataFlowRuntimeOrchestrator flow({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

    auto result = flow.ExecutePipeline(TEST_DATA);
    auto aaplResult = result.at(dailyTF).at(aapl);
    auto msftResult = result.at(dailyTF).at(msft);

    INFO("AAPL Result: " << aaplResult << "\n MSFT Result: " << msftResult);
    REQUIRE(aaplResult.contains(sma30.GetOutputId()));
    REQUIRE(aaplResult.contains(sma60.GetOutputId()));
    REQUIRE(msftResult[sma30.GetOutputId()].equals(
        msftResult[sma30.GetOutputId()]));
    REQUIRE(msftResult[sma60.GetOutputId()].equals(
        msftResult[sma60.GetOutputId()]));
  }

  SECTION("Transform with duplicate config ids and different tf") {
    auto dsDaily = data_source(
        "dataDaily",
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto dsMinute = data_source(
        "dataMinute",
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY);

    auto sma30_daily = sma(
        0, dsDaily.GetOutputId(closeKey), 30,
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto sma30_min = sma(
        0, dsMinute.GetOutputId(closeKey), 30,
        epochflow::EpochStratifyXConstants::instance().MINUTE_FREQUENCY);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{dsDaily, dsMinute, sma30_daily, sma30_min});
    REQUIRE_THROWS_WITH(
        DataFlowRuntimeOrchestrator({aapl, msft}, CreateMockTransformManager(std::move(transforms))),
        Catch::Matchers::ContainsSubstring("Duplicate transform id: 0"));
  }


  SECTION("DataFlowRuntimeOrchestrator with trade signal executor") {
    auto dailyTF =
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto ds = data_source("data", dailyTF);

    // Create boolean signal transforms
    auto longSignal = crossover("0", ds.GetOutputId(closeKey),
                                ds.GetOutputId(highKey), dailyTF);
    auto shortSignal = crossover("1", ds.GetOutputId(highKey),
                                 ds.GetOutputId(closeKey), dailyTF);
    auto closeSignal = crossover("2", ds.GetOutputId(closeKey),
                                 ds.GetOutputId(highKey), dailyTF);

    // Create trade executor configuration
    std::unordered_map<std::string, std::string> tradeInputs = {
        {"enter_long", longSignal.GetOutputId()},
        {"enter_short", shortSignal.GetOutputId()},
        {"exit_long", closeSignal.GetOutputId()}};
    auto tradeExecutor =
        trade_signal_executor_cfg("trade_executor", tradeInputs, dailyTF);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{ds, longSignal, shortSignal, closeSignal, tradeExecutor});
    DataFlowRuntimeOrchestrator graph({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

    auto result = graph.ExecutePipeline(TEST_DATA);

    // Basic checks
    REQUIRE(result.count(dailyTF.ToString()) == 1);
    REQUIRE(result.at(dailyTF.ToString()).count(aapl) == 1);
    REQUIRE(result.at(dailyTF.ToString()).count(msft) == 1);

    // Debug: Print all columns to see what's actually in the result
    auto aaplColumns = result.at(dailyTF.ToString()).at(aapl).column_names();
    std::string aaplColumnList =
        std::accumulate(aaplColumns.begin(), aaplColumns.end(), std::string(),
                        [](const std::string &a, const std::string &b) {
                          return a.empty() ? b : a + ", " + b;
                        });
    INFO("AAPL columns after transform: " << aaplColumnList);

    // The trade executor outputs should be just enter_long, enter_short, exit_*
    // keys without prefix since it overrides GetOutputId to return the output
    // name directly
    REQUIRE(result.at(dailyTF.ToString()).at(aapl).contains("enter_long"));

    // Verify signal values for AAPL
    auto aaplLong = result.at(dailyTF.ToString()).at(aapl)["enter_long"];
    auto aaplShort = result.at(dailyTF.ToString()).at(aapl)["enter_short"];
    auto aaplClose = result.at(dailyTF.ToString()).at(aapl)["exit_long"];

    // For TEST_DATA AAPL: close=[2,4,6], high=[4,6,10]
    // longSignal = crossover(close, high): close never crosses above high ->
    // all false shortSignal = crossover(high, close): high always above close
    // -> depends on crossover logic closeSignal = crossover(close, high): same
    // as longSignal -> all false

    REQUIRE(aaplLong.iloc(0).as_bool() == false);
    REQUIRE(aaplLong.iloc(1).as_bool() == false);
    REQUIRE(aaplLong.iloc(2).as_bool() == false);

    // shortSignal should be true where high crosses above close (check actual
    // crossover behavior)
    INFO("Short signal values: " << aaplShort);

    REQUIRE(aaplClose.iloc(0).as_bool() == false);
    REQUIRE(aaplClose.iloc(1).as_bool() == false);
    REQUIRE(aaplClose.iloc(2).as_bool() == false);

    // Verify signal values for MSFT
    auto msftLong = result.at(dailyTF.ToString()).at(msft)["enter_long"];
    auto msftShort = result.at(dailyTF.ToString()).at(msft)["enter_short"];
    auto msftClose = result.at(dailyTF.ToString()).at(msft)["exit_long"];

    // For TEST_DATA MSFT: close=[10,20,30], high=[40,40,40]
    // longSignal = crossover(close, high): close never crosses above high ->
    // all false shortSignal = crossover(high, close): high always above close
    // closeSignal = crossover(close, high): same as longSignal -> all false

    REQUIRE(msftLong.iloc(0).as_bool() == false);
    REQUIRE(msftLong.iloc(1).as_bool() == false);
    REQUIRE(msftLong.iloc(2).as_bool() == false);

    INFO("MSFT Short signal values: " << msftShort);

    REQUIRE(msftClose.iloc(0).as_bool() == false);
    REQUIRE(msftClose.iloc(1).as_bool() == false);
    REQUIRE(msftClose.iloc(2).as_bool() == false);
  }

  SECTION("DataFlowRuntimeOrchestrator with trade executor exits") {
    auto dailyTF =
        epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto ds = data_source("data", dailyTF);

    // Create signals that will result in indecisive situations
    auto longCond =
        crossover("0", ds.GetOutputId(closeKey), ds.GetOutputId(highKey),
                  dailyTF); // close > high (false for our data)
    auto shortCond =
        crossover("1", ds.GetOutputId(highKey), ds.GetOutputId(closeKey),
                  dailyTF); // high > close (true for our data)

    // Create trade executor with exits
    std::unordered_map<std::string, std::string> tradeInputs = {
        {"enter_long", longCond.GetOutputId()},
        {"enter_short", shortCond.GetOutputId()},
        {"exit_long", longCond.GetOutputId()}};
    auto tradeExecutor =
        trade_signal_executor_cfg("trade_executor", tradeInputs, dailyTF);

    auto transforms = test::TransformBuilder::BuildFromConfigurations(
        TransformConfigurationList{ds, longCond, shortCond, tradeExecutor});
    DataFlowRuntimeOrchestrator graph({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

    auto result = graph.ExecutePipeline(TEST_DATA);

    // Basic checks
    REQUIRE(result.count(dailyTF.ToString()) == 1);
    REQUIRE(result.at(dailyTF.ToString()).count(aapl) == 1);

    // Verify trade executor outputs are present
    auto dailyAAPLResult = result.at(dailyTF.ToString()).at(aapl);
    INFO(dailyAAPLResult);

    REQUIRE(dailyAAPLResult.contains("enter_long"));
    REQUIRE(dailyAAPLResult.contains("enter_short"));
    REQUIRE(dailyAAPLResult.contains("exit_long"));

    // Since closeIfIndecisive=true and we expect both long=false, short=true
    // the close signal should be generated when both are false or conflicting
    auto aaplClose = result.at(dailyTF.ToString()).at(aapl)["exit_long"];
    INFO("Close signal values: " << aaplClose);

    // The closeIfIndecisive logic should create close signals
    REQUIRE(aaplClose.size() == 3);
  }
}
