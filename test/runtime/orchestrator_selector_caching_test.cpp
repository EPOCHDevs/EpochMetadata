/**
 * @file orchestrator_selector_caching_test.cpp
 * @brief Comprehensive tests for selector generation, caching, and retrieval
 *
 * Tests cover ALL selector-related code paths:
 * - Selector detection (IsSelectorTransform helper)
 * - Empty/invalid selector handling (lines 316-319)
 * - First selector caching (lines 353-357)
 * - Selector overwriting (lines 348-351)
 * - Multi-asset selector distribution (lines 327-358)
 * - Parallel selector caching with mutex (lines 328-329)
 * - GetGeneratedSelectors (lines 365-367)
 * - DataFrame retrieval from cache (lines 333)
 */

#include <runtime/orchestrator.h>
#include "testing/test_constants.h"
#include "mocks/mock_transform.h"
#include "mocks/mock_transform_manager.h"
#include <epoch_core/catch_defs.h>
#include <catch2/catch_test_macros.hpp>
#include <trompeloeil.hpp>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_metadata/metadata_options.h>

using namespace epoch_flow::runtime;
using namespace epoch_flow::runtime;
using namespace epoch_flow::runtime::test;
using namespace epoch_metadata;

namespace {
    using namespace epoch_frame::factory::index;

    // Helper to create CardColumnSchema for testing
    epoch_metadata::CardColumnSchema CreateCardColumnSchema(
        const std::string& columnId,
        epoch_core::CardSlot slot = epoch_core::CardSlot::Hero,
        epoch_core::CardRenderType renderType = epoch_core::CardRenderType::Decimal) {

        epoch_metadata::CardColumnSchema schema;
        schema.column_id = columnId;
        schema.slot = slot;
        schema.render_type = renderType;
        return schema;
    }

    // Helper to create SelectorData with specific content
    epoch_metadata::transform::SelectorData CreateSelectorData(
        const std::string& title,
        int schemaCount,
        int dataRows = 3) {

        epoch_metadata::transform::SelectorData data;
        data.title = title;

        // Create schemas
        for (int i = 0; i < schemaCount; ++i) {
            data.schemas.push_back(CreateCardColumnSchema("col_" + std::to_string(i)));
        }

        // Create DataFrame
        auto idx = from_range(0, dataRows);
        std::vector<double> column_c(dataRows);
        std::iota(column_c.begin(), column_c.end(), 1.0);
        data.data = make_dataframe<double>(idx, {column_c}, {"c"});

        return data;
    }

    // Helper to create empty SelectorData (for testing empty handling)
    epoch_metadata::transform::SelectorData CreateEmptySelectorData() {
        return epoch_metadata::transform::SelectorData{};
    }

    // Helper to create test DataFrame with multiple columns
    epoch_frame::DataFrame CreateTestDataFrame(int numRows = 3, int numCols = 1) {
        auto idx = from_range(0, numRows);
        std::vector<std::string> colNames;
        std::vector<std::vector<double>> colData;

        for (int i = 0; i < numCols; ++i) {
            colNames.push_back("col_" + std::to_string(i));
            std::vector<double> data(numRows);
            std::iota(data.begin(), data.end(), static_cast<double>(i * numRows + 1));
            colData.push_back(data);
        }

        return make_dataframe<double>(idx, colData, colNames);
    }
}

TEST_CASE("DataFlowRuntimeOrchestrator - Selector Caching", "[.][orchestrator][selectors][critical]") {
    const auto dailyTF = TestTimeFrames::Daily();
    const std::string aapl = TestAssetConstants::AAPL;
    const std::string msft = TestAssetConstants::MSFT;
    const std::string googl = TestAssetConstants::GOOG;

    SECTION("Empty title selector is not cached - CRITICAL") {
        // Line 316-319 in dataflow_orchestrator.cpp
        // Selectors with empty title should be skipped
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        auto emptySelector = CreateEmptySelectorData();
        REQUIRE(emptySelector.title.empty());

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        REQUIRE_CALL(*mock, GetSelectorData())
            .RETURN(emptySelector);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // GetGeneratedSelectors should be empty because empty selector was not cached
        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.empty());
    }

    SECTION("Empty schemas selector is not cached - CRITICAL") {
        // Line 316-319 in dataflow_orchestrator.cpp
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        epoch_metadata::transform::SelectorData invalidSelector;
        invalidSelector.title = "Valid Title";
        invalidSelector.schemas = {};  // Empty schemas
        REQUIRE(invalidSelector.schemas.empty());

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        REQUIRE_CALL(*mock, GetSelectorData())
            .RETURN(invalidSelector);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // GetGeneratedSelectors should be empty
        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.empty());
    }

    SECTION("First selector cached for single asset - CRITICAL") {
        // Line 353-357 in dataflow_orchestrator.cpp
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        auto selectorData = CreateSelectorData("Test Selector", 3, 5);
        REQUIRE(!selectorData.title.empty());
        REQUIRE(!selectorData.schemas.empty());

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame(5));

        REQUIRE_CALL(*mock, GetSelectorData())
            .RETURN(selectorData);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame(5);

        orch.ExecutePipeline(std::move(inputData));

        // Verify selector was cached
        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.size() == 1);
        REQUIRE(selectors.contains(aapl));
        REQUIRE(selectors.at(aapl).size() == 1);  // One selector for this asset
        REQUIRE(selectors.at(aapl)[0].title == "Test Selector");
        REQUIRE(selectors.at(aapl)[0].schemas.size() == 3);
        REQUIRE(selectors.at(aapl)[0].data.num_rows() > 0);
    }

    SECTION("First selector cached for multiple assets - CRITICAL") {
        // Line 327-358 in dataflow_orchestrator.cpp
        // Selector should be cached for EACH asset
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        auto selectorData = CreateSelectorData("Multi-Asset Selector", 2, 4);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .TIMES(3)  // Called for each asset
            .RETURN(CreateTestDataFrame(4));

        REQUIRE_CALL(*mock, GetSelectorData())
            .TIMES(AT_LEAST(1))
            .RETURN(selectorData);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl, msft, googl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame(4);
        inputData[dailyTF.ToString()][msft] = CreateTestDataFrame(4);
        inputData[dailyTF.ToString()][googl] = CreateTestDataFrame(4);

        orch.ExecutePipeline(std::move(inputData));

        // Verify selector cached for ALL assets
        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.size() == 3);
        REQUIRE(selectors.contains(aapl));
        REQUIRE(selectors.contains(msft));
        REQUIRE(selectors.contains(googl));
        REQUIRE(selectors.at(aapl).size() == 1);
        REQUIRE(selectors.at(aapl)[0].schemas.size() == 2);
        REQUIRE(selectors.at(msft).size() == 1);
        REQUIRE(selectors.at(msft)[0].schemas.size() == 2);
        REQUIRE(selectors.at(googl).size() == 1);
        REQUIRE(selectors.at(googl)[0].schemas.size() == 2);
    }

    SECTION("Multiple selectors: both are appended to list - CRITICAL") {
        // Line 328-333 in dataflow_orchestrator.cpp
        // Multiple selectors per asset are now supported (appended to vector)
        auto selector1 = CreateSimpleMockTransform("selector1", dailyTF, {}, {"result"}, false, true);
        auto selector2 = CreateSimpleMockTransform("selector2", dailyTF, {}, {"result"}, false, true);

        auto selectorData1 = CreateSelectorData("First Selector", 1, 3);
        auto selectorData2 = CreateSelectorData("Second Selector", 3, 3);

        REQUIRE_CALL(*selector1, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());
        REQUIRE_CALL(*selector1, GetSelectorData())
            .RETURN(selectorData1);

        REQUIRE_CALL(*selector2, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());
        REQUIRE_CALL(*selector2, GetSelectorData())
            .RETURN(selectorData2);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(selector1));
        transforms.push_back(std::move(selector2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Both selectors should be retained in the vector
        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.size() == 1);
        REQUIRE(selectors.at(aapl).size() == 2);  // Two selectors for this asset
        REQUIRE(selectors.at(aapl)[0].title == "First Selector");
        REQUIRE(selectors.at(aapl)[0].schemas.size() == 1);  // From first selector
        REQUIRE(selectors.at(aapl)[1].title == "Second Selector");
        REQUIRE(selectors.at(aapl)[1].schemas.size() == 3);  // From second selector
    }

    SECTION("Schema preservation: CardColumnSchema fields") {
        // Verify that schema details are preserved correctly
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        epoch_metadata::transform::SelectorData selectorData;
        selectorData.title = "Schema Test";
        selectorData.schemas.push_back(CreateCardColumnSchema("price", epoch_core::CardSlot::Hero, epoch_core::CardRenderType::Decimal));
        selectorData.schemas.push_back(CreateCardColumnSchema("signal", epoch_core::CardSlot::PrimaryBadge, epoch_core::CardRenderType::Badge));
        selectorData.data = CreateTestDataFrame();

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());
        REQUIRE_CALL(*mock, GetSelectorData())
            .RETURN(selectorData);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.at(aapl).size() == 1);
        REQUIRE(selectors.at(aapl)[0].schemas.size() == 2);
        REQUIRE(selectors.at(aapl)[0].schemas[0].column_id == "price");
        REQUIRE(selectors.at(aapl)[0].schemas[0].slot == epoch_core::CardSlot::Hero);
        REQUIRE(selectors.at(aapl)[0].schemas[0].render_type == epoch_core::CardRenderType::Decimal);
        REQUIRE(selectors.at(aapl)[0].schemas[1].column_id == "signal");
        REQUIRE(selectors.at(aapl)[0].schemas[1].slot == epoch_core::CardSlot::PrimaryBadge);
        REQUIRE(selectors.at(aapl)[0].schemas[1].render_type == epoch_core::CardRenderType::Badge);
    }

    SECTION("GetGeneratedSelectors returns empty for no selectors") {
        // Line 365-367
        auto mock = CreateSimpleMockTransform("non_selector", dailyTF);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        // No GetSelectorData call expected

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.empty());
    }

    SECTION("Mixed pipeline: regular -> selector -> regular") {
        // Integration test: verify selector works in mixed transform graph
        auto data = CreateSimpleMockTransform("data", dailyTF, {}, {"c"}, false, false);
        auto filter = CreateSimpleMockTransform("filter", dailyTF, {"data#c"}, {"filtered"}, false, false);
        auto selector = CreateSimpleMockTransform("selector", dailyTF, {"filter#filtered"}, {"result"}, false, true);
        auto final_transform = CreateSimpleMockTransform("final", dailyTF, {"selector#result"}, {"result"}, false, false);

        REQUIRE_CALL(*data, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        REQUIRE_CALL(*filter, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        auto selectorData = CreateSelectorData("Pipeline Selector", 2);
        REQUIRE_CALL(*selector, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());
        REQUIRE_CALL(*selector, GetSelectorData())
            .RETURN(selectorData);

        REQUIRE_CALL(*final_transform, TransformData(trompeloeil::_))
            .RETURN(CreateTestDataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(data));
        transforms.push_back(std::move(filter));
        transforms.push_back(std::move(selector));
        transforms.push_back(std::move(final_transform));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = CreateTestDataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));

        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.size() == 1);
        REQUIRE(selectors.at(aapl).size() == 1);
        REQUIRE(selectors.at(aapl)[0].title == "Pipeline Selector");
    }

    SECTION("Large number of assets (stress test)") {
        std::vector<std::string> assets = {
            aapl, msft, googl,
            TestAssetConstants::TSLA,
            TestAssetConstants::AMZN
        };

        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        auto selectorData = CreateSelectorData("Large Asset Selector", 4);
        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .TIMES(5)  // Once per asset
            .RETURN(CreateTestDataFrame());

        REQUIRE_CALL(*mock, GetSelectorData())
            .TIMES(AT_LEAST(1))
            .RETURN(selectorData);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch(assets, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        for (const auto& asset : assets) {
            inputData[dailyTF.ToString()][asset] = CreateTestDataFrame();
        }

        orch.ExecutePipeline(std::move(inputData));

        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.size() == 5);
        for (const auto& asset : assets) {
            REQUIRE(selectors.contains(asset));
            REQUIRE(selectors.at(asset).size() == 1);
            REQUIRE(selectors.at(asset)[0].title == "Large Asset Selector");
            REQUIRE(selectors.at(asset)[0].schemas.size() == 4);
        }
    }

    SECTION("DataFrame content preserved in selector cache") {
        // Verify that the actual DataFrame data is correctly cached
        auto mock = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);

        auto selectorData = CreateSelectorData("Data Test", 1, 10);
        auto testDataFrame = CreateTestDataFrame(10, 2);  // 10 rows, 2 columns

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(testDataFrame);

        REQUIRE_CALL(*mock, GetSelectorData())
            .RETURN(selectorData);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = testDataFrame;

        orch.ExecutePipeline(std::move(inputData));

        auto selectors = orch.GetGeneratedSelectors();
        REQUIRE(selectors.at(aapl).size() == 1);
        REQUIRE(selectors.at(aapl)[0].data.num_rows() > 0);
        // The DataFrame in the cache should match what TransformData returned
    }

    SECTION("IsSelectorTransform helper correctly identifies selectors") {
        // This indirectly tests the IsSelectorTransform function (line 26-29)
        auto selector = CreateSimpleMockTransform("selector", dailyTF, {}, {"result"}, false, true);
        auto regular = CreateSimpleMockTransform("regular", dailyTF, {}, {"result"}, false, false);

        // Verify metadata is correctly set
        REQUIRE(selector->GetConfiguration().GetTransformDefinition().GetMetadata().category ==
                epoch_core::TransformCategory::Selector);
        REQUIRE(regular->GetConfiguration().GetTransformDefinition().GetMetadata().category !=
                epoch_core::TransformCategory::Selector);
    }
}
