//
// Unit test for intradayOnly default timeframe behavior
// Verifies that nodes with intradayOnly=true default to 1Min when no timeframe is specified
//

#include <catch2/catch_all.hpp>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;

TEST_CASE("IntradayOnly Default Timeframe", "[compiler][validation][timeframe][intradayOnly]")
{
    SECTION("gap_returns without timeframe should default to 1Min")
    {
        std::string source = R"(
vix = indices(ticker="VIX", timeframe="1D")()
gap = gap_returns()(vix.c)
numeric_cards_report(agg="mean", category="Test", title="Gap", group=0, group_size=1)(gap.gap_return)
)";

        AlgorithmAstCompiler compiler;

        // Should compile successfully (no error about missing timeframe)
        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            // Find gap_returns node
            auto gap_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "gap"; });

            REQUIRE(gap_it != result.end());
            REQUIRE(gap_it->timeframe.has_value());
            // Should default to 1Min
            REQUIRE(gap_it->timeframe->ToString() == "1Min");
        }());
    }

    SECTION("gap_returns with explicit timeframe should use that timeframe")
    {
        std::string source = R"(
vix = indices(ticker="VIX", timeframe="5Min")()
gap = gap_returns(timeframe="5Min")(vix.c)
numeric_cards_report(agg="mean", category="Test", title="Gap", group=0, group_size=1)(gap.gap_return)
)";

        AlgorithmAstCompiler compiler;

        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            auto gap_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "gap"; });

            REQUIRE(gap_it != result.end());
            REQUIRE(gap_it->timeframe.has_value());
            // Should use explicit 5Min, not default 1Min
            REQUIRE(gap_it->timeframe->ToString() == "5Min");
        }());
    }

    SECTION("gap_classify without timeframe should default to 1Min")
    {
        std::string source = R"(
vix = indices(ticker="VIX", timeframe="1D")()
gap = gap_returns()(vix.c)
classify = gap_classify()(gap.gap_return)
numeric_cards_report(agg="mean", category="Test", title="Classification", group=0, group_size=1)(classify.classification)
)";

        AlgorithmAstCompiler compiler;

        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            auto classify_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "classify"; });

            REQUIRE(classify_it != result.end());
            REQUIRE(classify_it->timeframe.has_value());
            REQUIRE(classify_it->timeframe->ToString() == "1Min");
        }());
    }

    SECTION("session_time_window without timeframe should default to 1Min")
    {
        std::string source = R"(
vix = indices(ticker="VIX", timeframe="1Min")()
window = session_time_window(session="NewYork")(vix.c)
numeric_cards_report(agg="mean", category="Test", title="Window", group=0, group_size=1)(window.value)
)";

        AlgorithmAstCompiler compiler;

        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            auto window_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "window"; });

            REQUIRE(window_it != result.end());
            REQUIRE(window_it->timeframe.has_value());
            REQUIRE(window_it->timeframe->ToString() == "1Min");
        }());
    }

    SECTION("Non-intradayOnly node (indices) should still require explicit timeframe")
    {
        std::string source = R"(
vix = indices(ticker="VIX")()
numeric_cards_report(agg="mean", category="Test", title="VIX", group=0, group_size=1)(vix.c)
)";

        AlgorithmAstCompiler compiler;

        // Should throw error about missing timeframe (indices is not intradayOnly)
        REQUIRE_THROWS_WITH(
            compiler.compile(source),
            Catch::Matchers::ContainsSubstring("requires a 'timeframe' parameter")
        );
    }
}
