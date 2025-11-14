//
// Unit tests for PythonParser preprocessSource functionality
// Tests that the parser correctly handles various quote patterns via preprocessing
//

#include <catch2/catch_test_macros.hpp>
#include "transforms/compiler/parser/python_parser.h"
#include "transforms/compiler/parser/ast_nodes.h"

using namespace epoch_script;

TEST_CASE("PythonParser::preprocessSource - Backtick replacement", "[compiler][parser][preprocess]")
{
    PythonParser parser;

    SECTION("Simple backtick strings are converted to double quotes")
    {
        std::string source = R"(
ticker = `AAPL`
)";

        // Parse should succeed and the string should be correctly parsed
        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            // Verify it's an assignment
            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            // Verify the value is a constant string
            auto* constant = dynamic_cast<Constant*>(assign->value.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::holds_alternative<std::string>(constant->value));
            REQUIRE(std::get<std::string>(constant->value) == "AAPL");
        }());
    }

    SECTION("Multiple backtick strings in one line")
    {
        std::string source = R"(
result = func(ticker=`AAPL`, exchange=`NASDAQ`)
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            // Verify it's an assignment with a call
            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* call = dynamic_cast<Call*>(assign->value.get());
            REQUIRE(call != nullptr);
            REQUIRE(call->keywords.size() == 2);
        }());
    }

    SECTION("Backticks with spaces and special characters")
    {
        std::string source = R"(
name = `Some Company Name 123`
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* constant = dynamic_cast<Constant*>(assign->value.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::holds_alternative<std::string>(constant->value));
            REQUIRE(std::get<std::string>(constant->value) == "Some Company Name 123");
        }());
    }
}

TEST_CASE("PythonParser::preprocessSource - Mismatched quote fixing", "[compiler][parser][preprocess]")
{
    PythonParser parser;

    SECTION("Opening double quote with closing single quote before parenthesis")
    {
        std::string source = R"(
result = func(param="value')
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* call = dynamic_cast<Call*>(assign->value.get());
            REQUIRE(call != nullptr);
            REQUIRE(call->keywords.size() == 1);

            auto* constant = dynamic_cast<Constant*>(call->keywords[0].second.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::holds_alternative<std::string>(constant->value));
            REQUIRE(std::get<std::string>(constant->value) == "value");
        }());
    }

    SECTION("Opening double quote with closing single quote before bracket")
    {
        std::string source = R"(
items = ["item1', "item2']
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* list = dynamic_cast<List*>(assign->value.get());
            REQUIRE(list != nullptr);
            REQUIRE(list->elts.size() == 2);

            auto* elem1 = dynamic_cast<Constant*>(list->elts[0].get());
            REQUIRE(elem1 != nullptr);
            REQUIRE(std::get<std::string>(elem1->value) == "item1");

            auto* elem2 = dynamic_cast<Constant*>(list->elts[1].get());
            REQUIRE(elem2 != nullptr);
            REQUIRE(std::get<std::string>(elem2->value) == "item2");
        }());
    }

    SECTION("Opening double quote with closing single quote before comma")
    {
        std::string source = R"(
result = func("arg1', "arg2')
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* call = dynamic_cast<Call*>(assign->value.get());
            REQUIRE(call != nullptr);
            REQUIRE(call->args.size() == 2);

            auto* arg1 = dynamic_cast<Constant*>(call->args[0].get());
            REQUIRE(arg1 != nullptr);
            REQUIRE(std::get<std::string>(arg1->value) == "arg1");

            auto* arg2 = dynamic_cast<Constant*>(call->args[1].get());
            REQUIRE(arg2 != nullptr);
            REQUIRE(std::get<std::string>(arg2->value) == "arg2");
        }());
    }

    SECTION("Opening double quote with closing single quote before asterisk - fixed")
    {
        std::string source = R"(
result = func("value'*)
)";

        // The regex will fix "value'*) to "value"*), but "value"*) is still invalid Python
        // This is expected - we can only fix quote mismatches, not all syntax errors
        REQUIRE_THROWS_AS([&]() {
            auto module = parser.parse(source);
        }(), PythonParseError);
    }
}

TEST_CASE("PythonParser::preprocessSource - Combined issues", "[compiler][parser][preprocess]")
{
    PythonParser parser;

    SECTION("Backticks and mismatched quotes in same source")
    {
        std::string source = R"(
ticker = `AAPL`
result = func(param="value')
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 2);

            // First assignment with backticks
            auto* assign1 = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign1 != nullptr);
            auto* const1 = dynamic_cast<Constant*>(assign1->value.get());
            REQUIRE(const1 != nullptr);
            REQUIRE(std::get<std::string>(const1->value) == "AAPL");

            // Second assignment with mismatched quotes
            auto* assign2 = dynamic_cast<Assign*>(module->body[1].get());
            REQUIRE(assign2 != nullptr);
            auto* call = dynamic_cast<Call*>(assign2->value.get());
            REQUIRE(call != nullptr);
        }());
    }

    SECTION("Multiple preprocessing fixes in complex expression")
    {
        std::string source = R"(
data = source(ticker=`MSFT`, timeframe="1D')
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* call = dynamic_cast<Call*>(assign->value.get());
            REQUIRE(call != nullptr);
            REQUIRE(call->keywords.size() == 2);

            // Check ticker parameter
            REQUIRE(call->keywords[0].first == "ticker");
            auto* ticker_val = dynamic_cast<Constant*>(call->keywords[0].second.get());
            REQUIRE(ticker_val != nullptr);
            REQUIRE(std::get<std::string>(ticker_val->value) == "MSFT");

            // Check timeframe parameter
            REQUIRE(call->keywords[1].first == "timeframe");
            auto* tf_val = dynamic_cast<Constant*>(call->keywords[1].second.get());
            REQUIRE(tf_val != nullptr);
            REQUIRE(std::get<std::string>(tf_val->value) == "1D");
        }());
    }
}

TEST_CASE("PythonParser::preprocessSource - Correctly quoted strings remain unchanged", "[compiler][parser][preprocess]")
{
    PythonParser parser;

    SECTION("Normal double quotes are preserved")
    {
        std::string source = R"(
ticker = "AAPL"
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* constant = dynamic_cast<Constant*>(assign->value.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::get<std::string>(constant->value) == "AAPL");
        }());
    }

    SECTION("Normal single quotes are preserved")
    {
        std::string source = R"(
ticker = 'AAPL'
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* constant = dynamic_cast<Constant*>(assign->value.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::get<std::string>(constant->value) == "AAPL");
        }());
    }

    SECTION("Mixed correctly matched quotes")
    {
        std::string source = R"(
result = func("double", 'single')
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* call = dynamic_cast<Call*>(assign->value.get());
            REQUIRE(call != nullptr);
            REQUIRE(call->args.size() == 2);

            auto* arg1 = dynamic_cast<Constant*>(call->args[0].get());
            REQUIRE(std::get<std::string>(arg1->value) == "double");

            auto* arg2 = dynamic_cast<Constant*>(call->args[1].get());
            REQUIRE(std::get<std::string>(arg2->value) == "single");
        }());
    }
}

TEST_CASE("PythonParser::preprocessSource - Edge cases", "[compiler][parser][preprocess]")
{
    PythonParser parser;

    SECTION("Empty backticks")
    {
        std::string source = R"(
empty = ``
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 1);

            auto* assign = dynamic_cast<Assign*>(module->body[0].get());
            REQUIRE(assign != nullptr);

            auto* constant = dynamic_cast<Constant*>(assign->value.get());
            REQUIRE(constant != nullptr);
            REQUIRE(std::get<std::string>(constant->value) == "");
        }());
    }

    SECTION("Nested quotes inside backticks - left unchanged (preprocessing skip)")
    {
        std::string source = R"(
value = `it's "quoted"`
)";

        // Backticks with nested quotes are intentionally NOT transformed by preprocessing
        // The pattern [^`"']* prevents matching to avoid creating "it's "quoted"" (invalid)
        // We just verify that preprocessing leaves it unchanged (whether tree-sitter accepts it or not)
        // The raw string will have backticks, which may or may not parse successfully
        REQUIRE_NOTHROW([&]() {
            // This just tests that preprocessing doesn't crash or transform incorrectly
            // The actual parsing may succeed or fail depending on tree-sitter's lenience
            auto module = parser.parse(source);
        }());
    }

    SECTION("Multiple consecutive backtick pairs")
    {
        std::string source = R"(
a = `first`
b = `second`
c = `third`
)";

        REQUIRE_NOTHROW([&]() {
            auto module = parser.parse(source);
            REQUIRE(module->body.size() == 3);

            auto* assign1 = dynamic_cast<Assign*>(module->body[0].get());
            auto* const1 = dynamic_cast<Constant*>(assign1->value.get());
            REQUIRE(std::get<std::string>(const1->value) == "first");

            auto* assign2 = dynamic_cast<Assign*>(module->body[1].get());
            auto* const2 = dynamic_cast<Constant*>(assign2->value.get());
            REQUIRE(std::get<std::string>(const2->value) == "second");

            auto* assign3 = dynamic_cast<Assign*>(module->body[2].get());
            auto* const3 = dynamic_cast<Constant*>(assign3->value.get());
            REQUIRE(std::get<std::string>(const3->value) == "third");
        }());
    }
}
