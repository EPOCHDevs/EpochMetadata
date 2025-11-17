//
// Created by Claude Code
// Unit tests for ternary operator type specialization
//
// Verifies that ternary expressions (if...else) are correctly compiled
// to the appropriate type-specialized boolean_select_* variant based on
// the types of the true/false branches.
//

#include <catch.hpp>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;

// Helper to compile code and return the compiled result
// skip_sink_validation=true allows compilation without an executor
static CompilationResult CompileCode(const std::string& code) {
    AlgorithmAstCompiler compiler;
    return compiler.compile(code, true);
}

// Helper to find node type in compiled result by prefix
static std::string FindNodeType(const CompilationResult& result, const std::string& node_type_prefix) {
    for (const auto& algo : result) {
        if (algo.type.find(node_type_prefix) == 0) {
            return algo.type;
        }
    }
    return "";
}

TEST_CASE("Ternary operator compiles to type-specialized boolean_select variants", "[compiler][ternary]") {

    SECTION("String ternary uses boolean_select_string") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
label = "High" if high_price else "Low"
# No executor needed for testing
)";
        auto graph = CompileCode(code);
        auto variant = FindNodeType(graph, "boolean_select");

        REQUIRE(variant == "boolean_select_string");
    }

    SECTION("Numeric ternary uses boolean_select_number") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
signal = 1 if high_price else 0
# No executor needed for testing
)";
        auto graph = CompileCode(code);
        auto variant = FindNodeType(graph, "boolean_select");

        REQUIRE(variant == "boolean_select_number");
    }

    SECTION("Boolean ternary uses boolean_select_boolean") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
high_volume = gt()(src.v, 1000000)
result = high_volume if high_price else high_price
# No executor needed for testing
)";
        auto graph = CompileCode(code);
        auto variant = FindNodeType(graph, "boolean_select");

        REQUIRE(variant == "boolean_select_boolean");
    }

    SECTION("Mixed numeric types use boolean_select_number") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
signal = 1.5 if high_price else 0
# No executor needed for testing
)";
        auto graph = CompileCode(code);
        auto variant = FindNodeType(graph, "boolean_select");

        REQUIRE(variant == "boolean_select_number");
    }

    SECTION("Nested ternary expressions work correctly") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
low_price = lt()(src.c, 50)
regime = "High" if high_price else "Low" if low_price else "Neutral"
# No executor needed for testing
)";
        auto result = CompileCode(code);

        // Should have two boolean_select nodes, both string type
        int string_count = 0;
        for (const auto& algo : result) {
            if (algo.type == "boolean_select_string") {
                string_count++;
            }
        }

        REQUIRE(string_count == 2);
    }

    SECTION("String priority - string mixed with number uses boolean_select_string") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
mixed = "High" if high_price else 0
# No executor needed for testing
)";
        auto graph = CompileCode(code);
        auto variant = FindNodeType(graph, "boolean_select");

        // String has priority, should use boolean_select_string
        REQUIRE(variant == "boolean_select_string");
    }
}

TEST_CASE("Ternary operator validates input counts", "[compiler][ternary][validation]") {

    SECTION("Ternary with compatible types compiles successfully") {
        const char* code = R"(
src = market_data_source(timeframe="1D")()
high_price = gt()(src.c, 100)
label = "High" if high_price else "Low"
# No executor needed for testing
)";

        REQUIRE_NOTHROW(CompileCode(code));
    }
}
