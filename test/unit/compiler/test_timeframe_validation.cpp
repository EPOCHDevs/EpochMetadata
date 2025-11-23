//
// Unit test for requiresTimeFrame validation
// Bug: Data sources with requiresTimeFrame=true should reject missing timeframe parameter
//

#include <catch2/catch_all.hpp>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;

TEST_CASE("Data Source Timeframe Validation", "[compiler][validation][timeframe]")
{
    SECTION("economic_indicator without timeframe should fail")
    {
        std::string source = R"(
fed_funds = economic_indicator(category="FedFunds")()
numeric_cards_report(agg="mean", category="Test", title="Value")(fed_funds.value)
)";

        AlgorithmAstCompiler compiler;

        // Should throw with helpful error message
        REQUIRE_THROWS_WITH(
            compiler.compile(source),
            Catch::Matchers::ContainsSubstring("requires a 'timeframe' parameter")
        );
    }

    SECTION("economic_indicator with timeframe should succeed")
    {
        std::string source = R"(
fed_funds = economic_indicator(category="FedFunds", timeframe="1D")()
numeric_cards_report(agg="mean", category="Test", title="Value")(fed_funds.value)
)";

        AlgorithmAstCompiler compiler;

        // Should compile successfully
        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            // Find fed_funds node
            auto fed_funds_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "fed_funds"; });

            REQUIRE(fed_funds_it != result.end());
            REQUIRE(fed_funds_it->timeframe.has_value());
            REQUIRE(fed_funds_it->timeframe->ToString() == "1D");
        }());
    }

    SECTION("indices() without timeframe should fail")
    {
        std::string source = R"(
vix = indices(ticker="VIX")()
numeric_cards_report(agg="mean", category="Test", title="VIX")(vix.c)
)";

        AlgorithmAstCompiler compiler;

        REQUIRE_THROWS_WITH(
            compiler.compile(source),
            Catch::Matchers::ContainsSubstring("requires a 'timeframe' parameter")
        );
    }

    SECTION("indices() with timeframe should succeed")
    {
        std::string source = R"(
vix = indices(ticker="VIX", timeframe="1D")()
numeric_cards_report(agg="mean", category="Test", title="VIX")(vix.c)
)";

        AlgorithmAstCompiler compiler;

        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);

            auto vix_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "vix"; });

            REQUIRE(vix_it != result.end());
            REQUIRE(vix_it->timeframe.has_value());
            REQUIRE(vix_it->timeframe->ToString() == "1D");
        }());
    }
}
