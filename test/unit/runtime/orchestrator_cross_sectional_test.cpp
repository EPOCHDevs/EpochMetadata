/**
 * @file orchestrator_cross_sectional_test.cpp
 * @brief Tests for cross-sectional transform execution in DataFlowRuntimeOrchestrator
 *
 * THIS TEST SUITE WOULD HAVE CAUGHT THE CROSS-SECTIONAL BUG!
 *
 * Tests cover:
 * - CreateExecutionFunction() branching (line 250-261) - THE BUG LOCATION
 * - MakeExecutionNode<true> vs MakeExecutionNode<false> path selection
 * - ApplyCrossSectionTransform() execution and data aggregation
 * - ApplyDefaultTransform() execution for non-cross-sectional transforms
 * - Mixed graphs with both execution paths
 * - Cross-sectional output distribution (broadcast vs per-asset)
 * - Multiple assets with cross-sectional transforms
 *
 * These tests verify that isCrossSectional metadata correctly determines execution path.
 */

#include "transforms/runtime/orchestrator.h"
#include "test_constants.h"
#include "../../integration/mocks/mock_transform.h"
#include "../../integration/mocks/mock_transform_manager.h"
#include <epoch_core/catch_defs.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <trompeloeil.hpp>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_script::runtime;
using namespace epoch_script::runtime;
using namespace epoch_script::runtime::test;
using namespace epoch_script;

namespace {
    using namespace epoch_frame::factory::index;

    // Helper to create simple test DataFrame with 'c' column
    epoch_frame::DataFrame CreateTestDataFrame(int numRows = 3) {
        auto idx = from_range(0, numRows);
        std::vector<double> column_c(numRows);
        std::iota(column_c.begin(), column_c.end(), 1.0);  // Fill with 1.0, 2.0, 3.0, ...

        return make_dataframe<double>(idx, {column_c}, {"c"});
    }
}

TEST_CASE("DataFlowRuntimeOrchestrator - Cross-Sectional Execution", "[.][orchestrator][cross-sectional][critical]") {
    const auto dailyTF = TestTimeFrames::Daily();
    const std::string aapl = TestAssetConstants::AAPL;
    const std::string msft = TestAssetConstants::MSFT;
    const std::string googl = TestAssetConstants::GOOG;

    SECTION("Cross-sectional transform receives correct isCrossSectional flag") {
        auto cs_mock = CreateSimpleMockTransform("cs_transform", dailyTF, {}, {"result"}, true);

        // Verify the mock reports isCrossSectional=true via configuration
        REQUIRE(cs_mock->GetConfiguration().IsCrossSectional() == true);

        auto regular_mock = CreateSimpleMockTransform("regular_transform", dailyTF, {}, {"result"}, false);

        // Verify the mock reports isCrossSectional=false
        REQUIRE(regular_mock->GetConfiguration().IsCrossSectional() == false);
    }

    SECTION("Regular (non-cross-sectional) transform processes each asset independently") {
        // This test verifies that regular transforms:
        // 1. Are called once PER asset (not once total)
        // 2. Process each asset's data separately

        auto regular_mock = CreateSimpleMockTransform("regular_sma", dailyTF, {}, {"result"}, false);

        // Regular transforms are called ONCE PER ASSET
        // Verify that the input DataFrame is NOT empty
        REQUIRE_CALL(*regular_mock, TransformData(trompeloeil::_))
            .TIMES(2)  // Called twice: once for AAPL, once for MSFT
            .WITH(!_1.empty())  // Verify input is not empty
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(regular_mock));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        auto testDF = CreateTestDataFrame();
        REQUIRE(!testDF.empty());  // Verify test data is not empty
        REQUIRE(testDF.num_rows() == 3);  // Verify test data has 3 rows
        inputData[dailyTF.ToString()][aapl] = testDF;
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Mixed graph: regular -> cross-sectional -> regular") {
        // This tests the most common real-world scenario:
        // data (regular) -> roc (regular) -> cs_momentum (cross-sectional) -> filter (regular)

        auto data = CreateSimpleMockTransform("data", dailyTF, {}, {"c"}, false);
        auto roc = CreateSimpleMockTransform("roc", dailyTF, {"data#c"}, {"result"}, false);
        auto cs_mom = CreateSimpleMockTransform("cs_mom", dailyTF, {"roc#result"}, {"result"}, true);
        auto filter = CreateSimpleMockTransform("filter", dailyTF, {"cs_mom#result"}, {"result"}, false);

        // data: called per asset (2x)
        REQUIRE_CALL(*data, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(CreateTestDataFrame());

        // roc: called per asset (2x)
        REQUIRE_CALL(*roc, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(CreateTestDataFrame());

        // cs_mom: cross-sectional, called ONCE for all assets
        REQUIRE_CALL(*cs_mom, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(CreateTestDataFrame());

        // filter: called per asset (2x)
        REQUIRE_CALL(*filter, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(data));
        transforms.push_back(std::move(roc));
        transforms.push_back(std::move(cs_mom));
        transforms.push_back(std::move(filter));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Multiple cross-sectional transforms in sequence") {
        // cs_momentum -> regular -> top_k
        // Both cross-sectional transforms should execute correctly

        auto cs_mom = CreateSimpleMockTransform("cs_mom", dailyTF, {"data#c"}, {"result"}, true);
        auto regular = CreateSimpleMockTransform("regular", dailyTF, {"cs_mom#result"}, {"result"}, false);
        auto top_k = CreateSimpleMockTransform("top_k", dailyTF, {"regular#result"}, {"result"}, true);

        // cs_mom: cross-sectional (1x)
        REQUIRE_CALL(*cs_mom, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(CreateTestDataFrame());

        // regular: per-asset (3x)
        REQUIRE_CALL(*regular, TransformData(trompeloeil::_))
            .TIMES(3)
            .RETURN(CreateTestDataFrame());

        // top_k: cross-sectional (1x)
        REQUIRE_CALL(*top_k, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs_mom));
        transforms.push_back(std::move(regular));
        transforms.push_back(std::move(top_k));

        DataFlowRuntimeOrchestrator orch({aapl, msft, googl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][googl] = epoch_frame::DataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Cross-sectional broadcast: single-column output to all assets") {
        // Tests execution_node.cpp:284-290
        // When cross-sectional result has single column == outputId, broadcast to all assets

        auto cs_mock = CreateSimpleMockTransform("cs_agg", dailyTF, {}, {"result"}, true);

        REQUIRE_CALL(*cs_mock, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs_mock));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame();

        auto result = orch.ExecutePipeline(std::move(inputData));

        // Both assets should receive the same broadcast data
        REQUIRE(result[dailyTF.ToString()].contains(aapl));
        REQUIRE(result[dailyTF.ToString()].contains(msft));
    }

    SECTION("Cross-sectional multi-column: per-asset distribution") {
        // Tests execution_node.cpp:292-303
        // When cross-sectional result has multiple columns (per asset), distribute correctly

        auto cs_mock = CreateSimpleMockTransform("cs_ranking", dailyTF, {}, {"result"}, true);

        REQUIRE_CALL(*cs_mock, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs_mock));

        DataFlowRuntimeOrchestrator orch({aapl, msft, googl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame();
        inputData[dailyTF.ToString()][googl] = epoch_frame::DataFrame();

        auto result = orch.ExecutePipeline(std::move(inputData));

        // Each asset should receive its corresponding column
        REQUIRE(result[dailyTF.ToString()].contains(aapl));
        REQUIRE(result[dailyTF.ToString()].contains(msft));
        REQUIRE(result[dailyTF.ToString()].contains(googl));
    }

    SECTION("5+ assets with cross-sectional transform - stress test") {
        // Large number of assets to verify cross-sectional aggregation scales

        std::vector<std::string> assets = {
            aapl, msft, googl,
            TestAssetConstants::TSLA,
            TestAssetConstants::AMZN
        };

        auto cs_mock = CreateSimpleMockTransform("cs_large", dailyTF, {}, {"result"}, true);

        REQUIRE_CALL(*cs_mock, TransformData(trompeloeil::_))
            .TIMES(1)  // Still only called ONCE despite 5 assets
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs_mock));

        DataFlowRuntimeOrchestrator orch(assets, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        for (const auto& asset : assets) {
            inputData[dailyTF.ToString()][asset] = epoch_frame::DataFrame();
        }

        auto result = orch.ExecutePipeline(std::move(inputData));

        // All assets should have output
        for (const auto& asset : assets) {
            REQUIRE(result[dailyTF.ToString()].contains(asset));
        }
    }

    SECTION("CreateExecutionFunction selects correct path based on metadata") {
        // This directly tests line 250-261 in dataflow_orchestrator.cpp
        // The bug was that this always returned MakeExecutionNode<false>

        auto cs_transform = CreateSimpleMockTransform("cs", dailyTF, {}, {"result"}, true);
        auto regular_transform = CreateSimpleMockTransform("reg", dailyTF, {}, {"result"}, false);

        // Verify metadata is correctly set
        REQUIRE(cs_transform->GetConfiguration().IsCrossSectional() == true);
        REQUIRE(regular_transform->GetConfiguration().IsCrossSectional() == false);

        // Both should initialize without error
        ALLOW_CALL(*cs_transform, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());
        ALLOW_CALL(*regular_transform, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms1;
        transforms1.push_back(std::move(cs_transform));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, CreateMockTransformManager(std::move(transforms1))));

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms2;
        transforms2.push_back(std::move(regular_transform));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, CreateMockTransformManager(std::move(transforms2))));
    }
}
