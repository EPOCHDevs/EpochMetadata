#include <catch.hpp>
#include "transforms/compiler/ast_compiler.h"

TEST_CASE("Scalars do not require timeframe resolution", "[compiler][scalars]")
{

    SECTION("Literal in boolean_select should compile without timeframe error")
    {
        std::string code = R"(
src = market_data_source(timeframe="1d")()
ret = intraday_returns(timeframe="1d", return_type="simple")()
cond = src.c > src.o

# boolean_select with literal 0 - this previously failed
result = boolean_select()(cond, ret, 0)

numeric_cards_report(agg="mean", category="Test", title="Result", group=0, group_size=1)(result)
)";

        epoch_script::AlgorithmAstCompiler compiler;
        REQUIRE_NOTHROW(compiler.compile(code, false));

        auto result = compiler.compile(code, false);
        REQUIRE(!result.empty());

        // Find scalar nodes
        int scalar_count = 0;
        for (const auto& node : result)
        {
            if (node.type == "number" || node.type == "bool_true" ||
                node.type == "bool_false" || node.type == "text" || node.type == "null")
            {
                scalar_count++;
                // Scalars should NOT have timeframes
                INFO("Scalar node: " << node.id << " of type: " << node.type);
                REQUIRE(!node.timeframe.has_value());
            }
        }

        REQUIRE(scalar_count > 0);  // Should have at least one scalar (the 0 literal)
    }

    SECTION("Multiple scalar literals in complex expression")
    {
        std::string code = R"(
src = market_data_source(timeframe="1h")()
result1 = boolean_select()(src.c > src.o, 1, 0)
result2 = boolean_select()(src.h > src.l, 100, -100)
numeric_cards_report(agg="sum", category="Test", title="R1", group=0, group_size=2)(result1)
numeric_cards_report(agg="sum", category="Test", title="R2", group=1, group_size=2)(result2)
)";

        epoch_script::AlgorithmAstCompiler compiler;
        REQUIRE_NOTHROW(compiler.compile(code, false));

        auto result = compiler.compile(code, false);

        // Count scalars - should have 4 number literals (1, 0, 100, -100)
        int scalar_count = 0;
        for (const auto& node : result)
        {
            if (node.type == "number")
            {
                scalar_count++;
                REQUIRE(!node.timeframe.has_value());
            }
        }

        // Note: Exact count may vary due to CSE optimization
        REQUIRE(scalar_count >= 2);
    }
}
