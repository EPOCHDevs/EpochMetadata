//
// EpochScript Boolean Type Casting Test
//
// Tests that boolean operations (and/or) automatically cast non-boolean operands
//

#include <catch.hpp>
#include "transforms/compiler/ast_compiler.h"
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace epoch_script;

TEST_CASE("Boolean operations with numeric operands require type casting", "[boolean_cast]")
{
    SECTION("logical_and with double and bool should auto-cast double to bool")
    {
        // Python code: result = 1.0 and True
        // Should compile with automatic type cast
        std::string python_code = R"(
result = 1.0 and True
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should succeed and contain a cast node (bool_to_num_cast or num_to_bool_cast)
            bool has_cast_node = false;
            for (const auto& node : result)
            {
                if (node.type == "neq" || node.type == "boolean_select")
                {
                    has_cast_node = true;
                    break;
                }
            }

            REQUIRE(has_cast_node);
            INFO("Compilation succeeded with type cast");
        }
        catch (const std::exception& e)
        {
            FAIL("Compilation should succeed with automatic type cast, but failed with: " << e.what());
        }
    }

    SECTION("logical_or with int64 and bool should auto-cast int64 to bool")
    {
        // Python code: result = 5 or False
        std::string python_code = R"(
result = 5 or False
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should succeed and contain a cast node
            bool has_cast_node = false;
            for (const auto& node : result)
            {
                if (node.type == "neq" || node.type == "boolean_select")
                {
                    has_cast_node = true;
                    break;
                }
            }

            REQUIRE(has_cast_node);
            INFO("Compilation succeeded with type cast");
        }
        catch (const std::exception& e)
        {
            FAIL("Compilation should succeed with automatic type cast, but failed with: " << e.what());
        }
    }

    SECTION("logical_and with bool and number should auto-cast number")
    {
        // Python code: result = True and 1
        // Boolean and number - number needs to be cast
        std::string python_code = R"(
result = True and 1
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should succeed and contain a cast node for the number
            bool has_cast_node = false;
            for (const auto& node : result)
            {
                if (node.type == "neq" || node.type == "boolean_select")
                {
                    has_cast_node = true;
                    break;
                }
            }

            REQUIRE(has_cast_node);
            INFO("Compilation succeeded with type cast");
        }
        catch (const std::exception& e)
        {
            FAIL("Compilation should succeed with automatic type cast, but failed with: " << e.what());
        }
    }

    SECTION("logical_or with multiple numeric operands should auto-cast all")
    {
        // Python code: result = 1 or 2 or 3
        std::string python_code = R"(
result = 1 or 2 or 3
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should succeed and contain multiple cast nodes
            int cast_count = 0;
            for (const auto& node : result)
            {
                if (node.type == "neq" || node.type == "boolean_select")
                {
                    cast_count++;
                }
            }

            // Should have at least 3 cast nodes (one for each operand)
            REQUIRE(cast_count >= 3);
            INFO("Compilation succeeded with multiple type casts");
        }
        catch (const std::exception& e)
        {
            FAIL("Compilation should succeed with automatic type cast, but failed with: " << e.what());
        }
    }

    SECTION("logical_and with string should fail (incompatible type)")
    {
        // Python code: result = "hello" and True
        // Strings cannot be cast to bool
        std::string python_code = R"(
result = "hello" and True
)";

        AlgorithmAstCompiler compiler;
        REQUIRE_THROWS_WITH(
            compiler.compile(python_code, true),  // skip_sink_validation=true
            Catch::Matchers::ContainsSubstring("Cannot use type String")
        );
    }
}

TEST_CASE("Boolean type casting preserves logical semantics", "[boolean_cast]")
{
    SECTION("number 0 should cast to false")
    {
        // In Python, 0 is falsy
        std::string python_code = R"(
result = 0 and True
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should compile successfully with cast using neq(0, 0) which evaluates to false
            bool has_logical_and = false;
            for (const auto& node : result)
            {
                if (node.type == "logical_and")
                {
                    has_logical_and = true;
                    break;
                }
            }

            REQUIRE(has_logical_and);
        }
        catch (const std::exception& e)
        {
            FAIL("Should compile: " << e.what());
        }
    }

    SECTION("non-zero number should cast to true")
    {
        // In Python, non-zero numbers are truthy
        std::string python_code = R"(
result = 42 or False
)";

        try
        {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(python_code, true);  // skip_sink_validation=true

            // Should compile successfully with cast using neq(42, 0) which evaluates to true
            bool has_logical_or = false;
            for (const auto& node : result)
            {
                if (node.type == "logical_or")
                {
                    has_logical_or = true;
                    break;
                }
            }

            REQUIRE(has_logical_or);
        }
        catch (const std::exception& e)
        {
            FAIL("Should compile: " << e.what());
        }
    }
}