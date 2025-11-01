/**
 * @file orchestrator_initialization_test.cpp
 * @brief Tests for DataFlowRuntimeOrchestrator initialization
 */

#include <runtime/orchestrator.h>
#include "testing/test_constants.h"
#include "mocks/mock_transform.h"
#include "mocks/mock_transform_manager.h"
#include <epoch_core/catch_defs.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <trompeloeil.hpp>

using namespace epoch_flow::runtime;
using namespace epoch_flow::runtime;
using namespace epoch_flow::runtime::test;
using namespace epoch_metadata;

TEST_CASE("DataFlowRuntimeOrchestrator - Initialization", "[orchestrator][init]") {
    const auto dailyTF = TestTimeFrames::Daily();
    const std::string aapl = TestAssetConstants::AAPL;
    const std::string msft = TestAssetConstants::MSFT;

    SECTION("Default initialization with single transform") {
        auto mock = CreateSimpleMockTransform("transform1", dailyTF);
        ALLOW_CALL(*mock, TransformData(trompeloeil::_))
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, std::move(manager)));
    }

    SECTION("Empty transform list is valid") {
        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, std::move(manager)));
    }

    SECTION("Multiple independent transforms") {
        auto mock1 = CreateSimpleMockTransform("transform1", dailyTF);
        auto mock2 = CreateSimpleMockTransform("transform2", dailyTF);
        auto mock3 = CreateSimpleMockTransform("transform3", dailyTF);

        ALLOW_CALL(*mock1, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mock2, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mock3, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock1));
        transforms.push_back(std::move(mock2));
        transforms.push_back(std::move(mock3));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, std::move(manager)));
    }

    SECTION("Duplicate transform IDs throw exception") {
        auto mock1 = CreateSimpleMockTransform("duplicate_id", dailyTF);
        auto mock2 = CreateSimpleMockTransform("duplicate_id", dailyTF);

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock1));
        transforms.push_back(std::move(mock2));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_THROWS_WITH(
            DataFlowRuntimeOrchestrator({aapl}, std::move(manager)),
            Catch::Matchers::ContainsSubstring("Duplicate transform id: duplicate_id")
        );
    }

    SECTION("Transform with dependencies on another transform") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF, {}, {"result"});
        auto mockB = CreateSimpleMockTransform("B", dailyTF, {"A#result"});

        ALLOW_CALL(*mockA, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mockB, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, std::move(manager)));
    }

    SECTION("Invalid input handle throws during construction") {
        auto mock = CreateSimpleMockTransform("dependent", dailyTF, {"nonexistent#output"});

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        auto manager = CreateMockTransformManager(std::move(transforms));
        // Should throw because "nonexistent" transform doesn't exist
        REQUIRE_THROWS_WITH(
            DataFlowRuntimeOrchestrator({aapl}, std::move(manager)),
            Catch::Matchers::ContainsSubstring("Handle nonexistent#output was not previously hashed")
        );
    }

    SECTION("Multiple assets initialization") {
        auto mock = CreateSimpleMockTransform("transform1", dailyTF);
        ALLOW_CALL(*mock, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mock));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl, msft}, std::move(manager)));
    }

    SECTION("Complex dependency chain: A -> B -> C -> D -> E") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF);
        auto mockB = CreateSimpleMockTransform("B", dailyTF, {"A#result"});
        auto mockC = CreateSimpleMockTransform("C", dailyTF, {"B#result"});
        auto mockD = CreateSimpleMockTransform("D", dailyTF, {"C#result"});
        auto mockE = CreateSimpleMockTransform("E", dailyTF, {"D#result"});

        ALLOW_CALL(*mockA, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mockB, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mockC, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mockD, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        ALLOW_CALL(*mockE, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));
        transforms.push_back(std::move(mockC));
        transforms.push_back(std::move(mockD));
        transforms.push_back(std::move(mockE));

        auto manager = CreateMockTransformManager(std::move(transforms));
        REQUIRE_NOTHROW(DataFlowRuntimeOrchestrator({aapl}, std::move(manager)));
    }
}
