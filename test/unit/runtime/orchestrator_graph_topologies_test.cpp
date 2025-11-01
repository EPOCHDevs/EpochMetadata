/**
 * @file orchestrator_graph_topologies_test.cpp
 * @brief Tests for various DAG topologies in DataFlowRuntimeOrchestrator
 *
 * This suite tests different graph patterns to ensure:
 * - Dependency resolution works correctly
 * - Execution order respects dependencies
 * - Both cross-sectional and regular transforms work in complex graphs
 * - Parallel and serial execution produce same results
 *
 * Graph patterns tested:
 * 1. Linear chain (A -> B -> C -> D -> E)
 * 2. Diamond (A -> B,C -> D)
 * 3. Wide parallel (A, B, C, D, E all independent)
 * 4. Multi-level tree (A -> B,C -> D,E,F,G -> H)
 * 5. Cross-sectional in chain
 * 6. Multiple cross-sectionals
 * 7. Cross-sectional fan-out
 * 8. Complex realistic pipeline
 */

#include "transforms/runtime/orchestrator.h"
#include "test_constants.h"
#include "../../integration/mocks/mock_transform.h"
#include "../../integration/mocks/mock_transform_manager.h"
#include <epoch_core/catch_defs.h>
#include <catch2/catch_test_macros.hpp>
#include <trompeloeil.hpp>
#include <atomic>

using namespace epoch_script::runtime;
using namespace epoch_script::runtime;
using namespace epoch_script::runtime::test;
using namespace epoch_script;

TEST_CASE("DataFlowRuntimeOrchestrator - Graph Topologies", "[.][orchestrator][graph][topologies]") {
    const auto dailyTF = TestTimeFrames::Daily();
    const std::string aapl = TestAssetConstants::AAPL;
    const std::string msft = TestAssetConstants::MSFT;

    SECTION("Linear chain: A -> B -> C -> D -> E") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF);
        auto mockB = CreateSimpleMockTransform("B", dailyTF, {"A#result"});
        auto mockC = CreateSimpleMockTransform("C", dailyTF, {"B#result"});
        auto mockD = CreateSimpleMockTransform("D", dailyTF, {"C#result"});
        auto mockE = CreateSimpleMockTransform("E", dailyTF, {"D#result"});

        std::atomic<int> executionOrder{0};
        int aOrder = 0, bOrder = 0, cOrder = 0, dOrder = 0, eOrder = 0;

        REQUIRE_CALL(*mockA, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(aOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockB, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(bOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockC, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(cOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockD, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(dOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockE, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(eOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));
        transforms.push_back(std::move(mockC));
        transforms.push_back(std::move(mockD));
        transforms.push_back(std::move(mockE));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify strict ordering: A < B < C < D < E
        REQUIRE(aOrder < bOrder);
        REQUIRE(bOrder < cOrder);
        REQUIRE(cOrder < dOrder);
        REQUIRE(dOrder < eOrder);
    }

    SECTION("Diamond: A -> B,C -> D") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF);
        auto mockB = CreateSimpleMockTransform("B", dailyTF, {"A#result"});
        auto mockC = CreateSimpleMockTransform("C", dailyTF, {"A#result"});
        auto mockD = CreateSimpleMockTransform("D", dailyTF, {"B#result", "C#result"});

        std::atomic<int> executionOrder{0};
        int aOrder = 0, bOrder = 0, cOrder = 0, dOrder = 0;

        REQUIRE_CALL(*mockA, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(aOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockB, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(bOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockC, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(cOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockD, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(dOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));
        transforms.push_back(std::move(mockC));
        transforms.push_back(std::move(mockD));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify dependencies:
        // A must execute first
        REQUIRE(aOrder < bOrder);
        REQUIRE(aOrder < cOrder);
        // B and C can be parallel (no ordering constraint between them)
        // D must execute after both B and C
        REQUIRE(bOrder < dOrder);
        REQUIRE(cOrder < dOrder);
    }

    SECTION("Wide parallel: A, B, C, D, E all independent") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF);
        auto mockB = CreateSimpleMockTransform("B", dailyTF);
        auto mockC = CreateSimpleMockTransform("C", dailyTF);
        auto mockD = CreateSimpleMockTransform("D", dailyTF);
        auto mockE = CreateSimpleMockTransform("E", dailyTF);

        REQUIRE_CALL(*mockA, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*mockB, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*mockC, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*mockD, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*mockE, TransformData(trompeloeil::_)).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));
        transforms.push_back(std::move(mockC));
        transforms.push_back(std::move(mockD));
        transforms.push_back(std::move(mockE));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        // All independent - should succeed
        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Multi-level tree: A -> B,C -> D,E,F,G -> H") {
        auto mockA = CreateSimpleMockTransform("A", dailyTF);
        auto mockB = CreateSimpleMockTransform("B", dailyTF, {"A#result"});
        auto mockC = CreateSimpleMockTransform("C", dailyTF, {"A#result"});
        auto mockD = CreateSimpleMockTransform("D", dailyTF, {"B#result"});
        auto mockE = CreateSimpleMockTransform("E", dailyTF, {"B#result"});
        auto mockF = CreateSimpleMockTransform("F", dailyTF, {"C#result"});
        auto mockG = CreateSimpleMockTransform("G", dailyTF, {"C#result"});
        auto mockH = CreateSimpleMockTransform("H", dailyTF, {"D#result", "E#result", "F#result", "G#result"});

        std::atomic<int> executionOrder{0};
        int aOrder = 0, bOrder = 0, cOrder = 0;
        int dOrder = 0, eOrder = 0, fOrder = 0, gOrder = 0, hOrder = 0;

        REQUIRE_CALL(*mockA, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(aOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockB, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(bOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockC, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(cOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockD, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(dOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockE, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(eOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockF, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(fOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockG, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(gOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*mockH, TransformData(trompeloeil::_))
            .LR_SIDE_EFFECT(hOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(mockA));
        transforms.push_back(std::move(mockB));
        transforms.push_back(std::move(mockC));
        transforms.push_back(std::move(mockD));
        transforms.push_back(std::move(mockE));
        transforms.push_back(std::move(mockF));
        transforms.push_back(std::move(mockG));
        transforms.push_back(std::move(mockH));

        DataFlowRuntimeOrchestrator orch({aapl}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify dependency constraints
        REQUIRE(aOrder < bOrder);
        REQUIRE(aOrder < cOrder);
        REQUIRE(bOrder < dOrder);
        REQUIRE(bOrder < eOrder);
        REQUIRE(cOrder < fOrder);
        REQUIRE(cOrder < gOrder);
        REQUIRE(dOrder < hOrder);
        REQUIRE(eOrder < hOrder);
        REQUIRE(fOrder < hOrder);
        REQUIRE(gOrder < hOrder);
    }

    SECTION("Cross-sectional in chain: regular -> cs -> regular") {
        auto regular1 = CreateSimpleMockTransform("reg1", dailyTF, {}, {"result"}, false);
        auto cs = CreateSimpleMockTransform("cs", dailyTF, {"reg1#result"}, {"result"}, true);
        auto regular2 = CreateSimpleMockTransform("reg2", dailyTF, {"cs#result"}, {"result"}, false);

        // regular1: per-asset (2x)
        REQUIRE_CALL(*regular1, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(epoch_frame::DataFrame());

        // cs: cross-sectional (1x)
        REQUIRE_CALL(*cs, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(epoch_frame::DataFrame());

        // regular2: per-asset (2x)
        REQUIRE_CALL(*regular2, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(regular1));
        transforms.push_back(std::move(cs));
        transforms.push_back(std::move(regular2));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Multiple cross-sectionals: cs1 -> regular -> cs2 -> regular") {
        auto cs1 = CreateSimpleMockTransform("cs1", dailyTF, {}, {"result"}, true);
        auto reg1 = CreateSimpleMockTransform("reg1", dailyTF, {"cs1#result"}, {"result"}, false);
        auto cs2 = CreateSimpleMockTransform("cs2", dailyTF, {"reg1#result"}, {"result"}, true);
        auto reg2 = CreateSimpleMockTransform("reg2", dailyTF, {"cs2#result"}, {"result"}, false);

        REQUIRE_CALL(*cs1, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*reg1, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*cs2, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*reg2, TransformData(trompeloeil::_))
            .TIMES(2)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs1));
        transforms.push_back(std::move(reg1));
        transforms.push_back(std::move(cs2));
        transforms.push_back(std::move(reg2));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Cross-sectional fan-out: cs -> reg1, reg2, reg3") {
        auto cs = CreateSimpleMockTransform("cs", dailyTF, {}, {"result"}, true);
        auto reg1 = CreateSimpleMockTransform("reg1", dailyTF, {"cs#result"}, {"result"}, false);
        auto reg2 = CreateSimpleMockTransform("reg2", dailyTF, {"cs#result"}, {"result"}, false);
        auto reg3 = CreateSimpleMockTransform("reg3", dailyTF, {"cs#result"}, {"result"}, false);

        REQUIRE_CALL(*cs, TransformData(trompeloeil::_))
            .TIMES(1)
            .RETURN(epoch_frame::DataFrame());

        REQUIRE_CALL(*reg1, TransformData(trompeloeil::_)).TIMES(2).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reg2, TransformData(trompeloeil::_)).TIMES(2).RETURN(epoch_frame::DataFrame());
        REQUIRE_CALL(*reg3, TransformData(trompeloeil::_)).TIMES(2).RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(cs));
        transforms.push_back(std::move(reg1));
        transforms.push_back(std::move(reg2));
        transforms.push_back(std::move(reg3));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();

        REQUIRE_NOTHROW(orch.ExecutePipeline(std::move(inputData)));
    }

    SECTION("Complex realistic pipeline: data -> roc -> cs_momentum -> top_k -> signal") {
        // This simulates a real-world quant pipeline
        auto data = CreateSimpleMockTransform("data", dailyTF, {}, {"c"}, false);
        auto roc = CreateSimpleMockTransform("roc", dailyTF, {"data#c"}, {"result"}, false);
        auto cs_mom = CreateSimpleMockTransform("cs_mom", dailyTF, {"roc#result"}, {"result"}, true);
        auto top_k = CreateSimpleMockTransform("top_k", dailyTF, {"cs_mom#result"}, {"result"}, true);
        auto signal = CreateSimpleMockTransform("signal", dailyTF, {"top_k#result"}, {"result"}, false);

        std::atomic<int> executionOrder{0};
        int dataOrder = 0, rocOrder = 0, csMomOrder = 0, topKOrder = 0, signalOrder = 0;

        // data: per-asset (2x)
        REQUIRE_CALL(*data, TransformData(trompeloeil::_))
            .TIMES(2)
            .LR_SIDE_EFFECT(dataOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        // roc: per-asset (2x)
        REQUIRE_CALL(*roc, TransformData(trompeloeil::_))
            .TIMES(2)
            .LR_SIDE_EFFECT(rocOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        // cs_mom: cross-sectional (1x)
        REQUIRE_CALL(*cs_mom, TransformData(trompeloeil::_))
            .TIMES(1)
            .LR_SIDE_EFFECT(csMomOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        // top_k: cross-sectional (1x)
        REQUIRE_CALL(*top_k, TransformData(trompeloeil::_))
            .TIMES(1)
            .LR_SIDE_EFFECT(topKOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        // signal: per-asset (2x)
        REQUIRE_CALL(*signal, TransformData(trompeloeil::_))
            .TIMES(2)
            .LR_SIDE_EFFECT(signalOrder = ++executionOrder)
            .RETURN(epoch_frame::DataFrame());

        std::vector<std::unique_ptr<epoch_script::transform::ITransformBase>> transforms;
        transforms.push_back(std::move(data));
        transforms.push_back(std::move(roc));
        transforms.push_back(std::move(cs_mom));
        transforms.push_back(std::move(top_k));
        transforms.push_back(std::move(signal));

        DataFlowRuntimeOrchestrator orch({aapl, msft}, CreateMockTransformManager(std::move(transforms)));

        TimeFrameAssetDataFrameMap inputData;
        inputData[dailyTF.ToString()][aapl] = epoch_frame::DataFrame();
        inputData[dailyTF.ToString()][msft] = epoch_frame::DataFrame();

        orch.ExecutePipeline(std::move(inputData));

        // Verify execution order constraints
        // Note: dataOrder and rocOrder might be interleaved for different assets in parallel mode
        // but cross-sectional transforms must come after all regular transforms complete
        REQUIRE(csMomOrder < topKOrder);  // cs_mom before top_k
        REQUIRE(topKOrder < signalOrder);  // top_k before signal
    }
}
