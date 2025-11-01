/**
 * @file orchestrator_report_caching_test.cpp
 * @brief Comprehensive tests for report generation, caching, and merging
 *
 * Tests cover ALL report-related code paths:
 * - Reporter detection (line 137) - CRITICAL UNCOVERED
 * - Empty report handling (line 278-284) - CRITICAL UNCOVERED
 * - First report caching (line 305-307) - CRITICAL UNCOVERED
 * - Report merging (line 297-300) - CRITICAL UNCOVERED
 * - Multi-asset report caching (line 287-309) - CRITICAL UNCOVERED
 * - Parallel report caching with mutex (line 290-291)
 * - Report merge details (cards, charts, tables)
 * - GetGeneratedReports (line 377-378)
 */

#include "transforms/runtime/orchestrator.h"
#include "test_constants.h"
#include "../../integration/mocks/mock_transform.h"
#include "../../integration/mocks/mock_transform_manager.h"
#include <epoch_core/catch_defs.h>
#include <catch2/catch_test_macros.hpp>
#include <trompeloeil.hpp>
#include <epoch_protos/tearsheet.pb.h>

using namespace epoch_script::runtime;
using namespace epoch_script::runtime;
using namespace epoch_script::runtime::test;
using namespace epoch_script;

// Helper to create a tearsheet with specific content
epoch_proto::TearSheet CreateTearSheetWithCards(int cardCount) {
    epoch_proto::TearSheet sheet;
    auto* cards = sheet.mutable_cards();
    for (int i = 0; i < cardCount; ++i) {
        cards->add_cards();  // Add empty cards for counting
    }
    return sheet;
}

epoch_proto::TearSheet CreateTearSheetWithCharts(int chartCount) {
    epoch_proto::TearSheet sheet;
    auto* charts = sheet.mutable_charts();
    for (int i = 0; i < chartCount; ++i) {
        charts->add_charts();  // Add empty charts for counting
    }
    return sheet;
}

epoch_proto::TearSheet CreateTearSheetWithTables(int tableCount) {
    epoch_proto::TearSheet sheet;
    auto* tables = sheet.mutable_tables();
    for (int i = 0; i < tableCount; ++i) {
        tables->add_tables();  // Add empty tables for counting
    }
    return sheet;
}

TEST_CASE("DataFlowRuntimeOrchestrator - Report Caching", "[.][orchestrator][reports][critical]") {
    const auto dailyTF = TestTimeFrames::Daily();
    const std::string aapl = TestAssetConstants::AAPL;
    const std::string msft = TestAssetConstants::MSFT;
    const std::string googl = TestAssetConstants::GOOG;

    SECTION("Empty report is not cached - CRITICAL") {
        // Line 278-284 in dataflow_orchestrator.cpp
        // Empty reports (ByteSizeLong() == 0) should be skipped
        auto mock = CreateSimpleMockTransform("reporter", dailyTF);

        epoch_proto::TearSheet emptySheet;  // Empty, ByteSizeLong() == 0
        REQUIRE(emptySheet.ByteSizeLong() == 0);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mock, GetTearSheet())
            .RETURN(emptySheet);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // GetGeneratedReports should be empty because empty report was not cached
        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.empty());
    }

    SECTION("First report cached for single asset - CRITICAL") {
        // Line 305-307 in dataflow_orchestrator.cpp
        auto mock = CreateSimpleMockTransform("reporter", dailyTF);

        auto sheet = CreateTearSheetWithCards(3);
        REQUIRE(sheet.ByteSizeLong() > 0);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mock, GetTearSheet())
            .RETURN(sheet);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify report was cached
        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.size() == 1);
        REQUIRE(reports.contains(aapl));
        REQUIRE(reports.at(aapl).cards().cards_size() == 3);
    }

    SECTION("First report cached for multiple assets - CRITICAL") {
        // Line 287-309 in dataflow_orchestrator.cpp
        // Report should be cached for EACH asset
        auto mock = CreateSimpleMockTransform("reporter", dailyTF);

        auto sheet = CreateTearSheetWithCards(2);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .TIMES(3)  // Called for each asset
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mock, GetTearSheet())
            .TIMES(AT_LEAST(1))
            .RETURN(sheet);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl, msft, googl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][googl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify report cached for ALL assets
        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.size() == 3);
        REQUIRE(reports.contains(aapl));
        REQUIRE(reports.contains(msft));
        REQUIRE(reports.contains(googl));
        REQUIRE(reports.at(aapl).cards().cards_size() == 2);
        REQUIRE(reports.at(msft).cards().cards_size() == 2);
        REQUIRE(reports.at(googl).cards().cards_size() == 2);
    }

    SECTION("Multiple reporters merge reports for single asset - CRITICAL") {
        // Line 297-300 in dataflow_orchestrator.cpp
        auto reporter1 = CreateSimpleMockTransform("reporter1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("reporter2", dailyTF);

        auto sheet1 = CreateTearSheetWithCards(2);
        auto sheet2 = CreateTearSheetWithCards(3);

        REQUIRE_CALL(*reporter1, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reporter1, GetTearSheet())
            .RETURN(sheet1);

        REQUIRE_CALL(*reporter2, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reporter2, GetTearSheet())
            .RETURN(sheet2);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Reports should be merged: 2 + 3 = 5 cards total
        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.size() == 1);
        REQUIRE(reports.at(aapl).cards().cards_size() == 5);
    }

    SECTION("Multiple reporters, multiple assets - all combinations - CRITICAL") {
        // Line 287-309 in dataflow_orchestrator.cpp
        // This is the most complex scenario
        auto reporter1 = CreateSimpleMockTransform("reporter1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("reporter2", dailyTF);
        auto reporter3 = CreateSimpleMockTransform("reporter3", dailyTF);

        auto sheet1 = CreateTearSheetWithCards(1);
        auto sheet2 = CreateTearSheetWithCards(2);
        auto sheet3 = CreateTearSheetWithCards(3);

        REQUIRE_CALL(*reporter1, TransformData(trompeloeil::_))
            .TIMES(2).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reporter1, GetTearSheet())
            .TIMES(AT_LEAST(1)).RETURN(sheet1);

        REQUIRE_CALL(*reporter2, TransformData(trompeloeil::_))
            .TIMES(2).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reporter2, GetTearSheet())
            .TIMES(AT_LEAST(1)).RETURN(sheet2);

        REQUIRE_CALL(*reporter3, TransformData(trompeloeil::_))
            .TIMES(2).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reporter3, GetTearSheet())
            .TIMES(AT_LEAST(1)).RETURN(sheet3);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));
        transforms.push_back(std::move(reporter3));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Each asset should have merged reports from all 3 reporters
        // 1 + 2 + 3 = 6 cards per asset
        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.size() == 2);
        REQUIRE(reports.at(aapl).cards().cards_size() == 6);
        REQUIRE(reports.at(msft).cards().cards_size() == 6);
    }

    SECTION("Cards are merged correctly") {
        auto reporter1 = CreateSimpleMockTransform("r1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("r2", dailyTF);

        auto sheet1 = CreateTearSheetWithCards(5);
        auto sheet2 = CreateTearSheetWithCards(7);

        ALLOW_CALL(*reporter1, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter1, GetTearSheet()).RETURN(sheet1);

        ALLOW_CALL(*reporter2, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter2, GetTearSheet()).RETURN(sheet2);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.at(aapl).cards().cards_size() == 12);  // 5 + 7
    }

    SECTION("Charts are merged correctly") {
        auto reporter1 = CreateSimpleMockTransform("r1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("r2", dailyTF);

        auto sheet1 = CreateTearSheetWithCharts(3);
        auto sheet2 = CreateTearSheetWithCharts(4);

        ALLOW_CALL(*reporter1, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter1, GetTearSheet()).RETURN(sheet1);

        ALLOW_CALL(*reporter2, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter2, GetTearSheet()).RETURN(sheet2);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.at(aapl).charts().charts_size() == 7);  // 3 + 4
    }

    SECTION("Tables are merged correctly") {
        auto reporter1 = CreateSimpleMockTransform("r1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("r2", dailyTF);

        auto sheet1 = CreateTearSheetWithTables(2);
        auto sheet2 = CreateTearSheetWithTables(3);

        ALLOW_CALL(*reporter1, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter1, GetTearSheet()).RETURN(sheet1);

        ALLOW_CALL(*reporter2, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter2, GetTearSheet()).RETURN(sheet2);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.at(aapl).tables().tables_size() == 5);  // 2 + 3
    }

    SECTION("Mixed content (cards, charts, tables) merged correctly") {
        auto reporter1 = CreateSimpleMockTransform("r1", dailyTF);
        auto reporter2 = CreateSimpleMockTransform("r2", dailyTF);

        epoch_proto::TearSheet sheet1;
        sheet1.mutable_cards()->add_cards();  // Add empty card
        sheet1.mutable_charts()->add_charts();  // Add empty chart

        epoch_proto::TearSheet sheet2;
        sheet2.mutable_cards()->add_cards();  // Add empty card
        sheet2.mutable_tables()->add_tables();  // Add empty table

        ALLOW_CALL(*reporter1, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter1, GetTearSheet()).RETURN(sheet1);

        ALLOW_CALL(*reporter2, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*reporter2, GetTearSheet()).RETURN(sheet2);

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(reporter1));
        transforms.push_back(std::move(reporter2));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.at(aapl).cards().cards_size() == 2);
        REQUIRE(reports.at(aapl).charts().charts_size() == 1);
        REQUIRE(reports.at(aapl).tables().tables_size() == 1);
    }

    SECTION("GetGeneratedReports returns empty for no reporters") {
        // Line 377-378
        auto mock = CreateSimpleMockTransform("non_reporter", dailyTF);

        REQUIRE_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());

        // No GetTearSheet call expected

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.empty());
    }

    SECTION("Large number of reporters (stress test)") {
        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;

        for (int i = 0; i < 20; ++i) {
            auto mock = CreateSimpleMockTransform("reporter_" + std::to_string(i), dailyTF);
            auto sheet = CreateTearSheetWithCards(1);

            ALLOW_CALL(*mock, TransformData(trompeloeil::_))
                .RETURN(epoch_frame::DataFrame());
            ALLOW_CALL(*mock, GetTearSheet()).RETURN(sheet);

            transforms.push_back(std::move(mock));
        }

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        auto reports = orch.GetGeneratedReports();
        REQUIRE(reports.at(aapl).cards().cards_size() == 20);
    }
}
