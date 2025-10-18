//
// EpochFlow AST Compiler Test Suite
//
// Tests Python algorithm compilation to AlgorithmNode list using JSON expected outputs
//

#include <catch.hpp>
#include <glaze/glaze.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "epochflow/compiler/ast_compiler.h"

using namespace epoch_stratifyx::epochflow;
namespace fs = std::filesystem;

// Test case structure
struct TestCase
{
    std::string name;
    std::string input_path;
    std::string expected_path;
};

// Error case structure for tests expecting compilation errors
struct CompilerErrorCase
{
    std::string error;
};

// Helper to load all test cases from test_cases directory
std::vector<TestCase> LoadTestCases()
{
    std::vector<TestCase> cases;
    fs::path test_dir = fs::path(__FILE__).parent_path() / "test_cases";

    if (!fs::exists(test_dir))
    {
        return cases;
    }

    for (const auto &entry : fs::directory_iterator(test_dir))
    {
        if (!entry.is_directory())
            continue;

        fs::path input = entry.path() / "input.txt";
        fs::path expected = entry.path() / "expected.json";

        if (fs::exists(input) && fs::exists(expected))
        {
            cases.push_back({entry.path().filename().string(),
                             input.string(),
                             expected.string()});
        }
    }

    std::sort(cases.begin(), cases.end(),
              [](const auto &a, const auto &b)
              { return a.name < b.name; });
    return cases;
}

// Normalize result for comparison (sort by id)
CompilationResult NormalizeResult(CompilationResult result)
{
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b)
              { return a.id < b.id; });
    return result;
}

// Read file contents
std::string ReadFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

// Parameterized test cases
TEST_CASE("EpochFlow Compiler: Test Cases", "[epochflow_compiler]")
{
    auto test_cases = LoadTestCases();

    if (test_cases.empty())
    {
        WARN("No test cases found in test_cases directory");
        return;
    }

    INFO("Found " << test_cases.size() << " test cases");

    for (const auto &test_case : test_cases)
    {
        DYNAMIC_SECTION(test_case.name)
        {
            // Read input Python source
            std::string source = ReadFile(test_case.input_path);

            // Read expected JSON
            std::string expected_json = ReadFile(test_case.expected_path);

            // Check if this is an error case
            if (expected_json.find("\"error\"") != std::string::npos)
            {
                // Error case: expect exception
                auto error_case = glz::read_json<CompilerErrorCase>(expected_json);
                REQUIRE(error_case.has_value());

                bool threw_expected_error = false;
                std::string actual_error;

                try
                {
                    AlgorithmAstCompiler compiler;
                    compiler.compile(source);
                }
                catch (const std::exception &e)
                {
                    actual_error = e.what();
                    if (actual_error.find(error_case->error) != std::string::npos)
                    {
                        threw_expected_error = true;
                    }
                }

                INFO("Expected error containing: " << error_case->error);
                INFO("Actual error: " << actual_error);
                REQUIRE(threw_expected_error);
            }
            else
            {
                // Success case: parse expected and compare
                auto expected_result = glz::read_json<CompilationResult>(expected_json);

                if (!expected_result.has_value())
                {
                    FAIL("Failed to parse expected.json: " << glz::format_error(expected_result.error(), expected_json));
                }

                AlgorithmAstCompiler compiler;
                CompilationResult actual_result;

                try
                {
                    actual_result = compiler.compile(source);
                }
                catch (const std::exception &e)
                {
                    FAIL("Compilation failed: " << e.what());
                }

                // Normalize both for comparison
                auto normalized_actual = NormalizeResult(std::move(actual_result));
                auto normalized_expected = NormalizeResult(std::move(*expected_result));

                // Serialize both to JSON for easy debugging
                std::string actual_json = glz::write_json(normalized_actual).value_or("{}");
                std::string expected_json_normalized = glz::write_json(normalized_expected).value_or("{}");

                INFO("Expected JSON: " << expected_json_normalized);
                INFO("Actual JSON: " << actual_json);

                REQUIRE(normalized_actual.size() == normalized_expected.size());

                for (size_t i = 0; i < normalized_actual.size(); ++i)
                {
                    const auto &actual = normalized_actual[i];
                    const auto &expected = normalized_expected[i];

                    INFO("Comparing node at index " << i);
                    INFO("Expected id: " << expected.id << ", type: " << expected.type);
                    INFO("Actual id: " << actual.id << ", type: " << actual.type);

                    REQUIRE(actual.id == expected.id);
                    REQUIRE(actual.type == expected.type);

                    // Compare options
                    REQUIRE(actual.options.size() == expected.options.size());
                    for (const auto &[key, expected_val] : expected.options)
                    {
                        REQUIRE(actual.options.contains(key));
                        // Basic value comparison (can be enhanced)
                        INFO("Comparing option: " << key);
                    }

                    // Compare inputs map
                    REQUIRE(actual.inputs.size() == expected.inputs.size());
                    for (const auto &[handle, expected_refs] : expected.inputs)
                    {
                        REQUIRE(actual.inputs.contains(handle));
                        REQUIRE(actual.inputs.at(handle).size() == expected_refs.size());

                        for (size_t j = 0; j < expected_refs.size(); ++j)
                        {
                            INFO("Comparing input " << handle << "[" << j << "]");
                            INFO("Expected: " << expected_refs[j]);
                            INFO("Actual: " << actual.inputs.at(handle)[j]);
                            REQUIRE(actual.inputs.at(handle)[j] == expected_refs[j]);
                        }
                    }

                    // Compare timeframe if present
                    REQUIRE(actual.timeframe.has_value() == expected.timeframe.has_value());
                    if (expected.timeframe.has_value())
                    {
                        REQUIRE(actual.timeframe->ToString() == expected.timeframe->ToString());
                    }

                    // Compare session if present
                    REQUIRE(actual.session.has_value() == expected.session.has_value());
                }
            }
        }
    }
}

// Manual test for basic functionality without test files
TEST_CASE("EpochFlow Compiler: Manual Basic Test", "[epochflow_compiler]")
{
    const std::string source = R"(
x = 5.0
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(source);

    REQUIRE(result.size() >= 1);

    // Find the number node
    bool found_number = false;
    for (const auto &node : result)
    {
        if (node.type == "number")
        {
            found_number = true;
            REQUIRE(node.options.contains("value"));
            break;
        }
    }

    REQUIRE(found_number);
}

// ============================================================================
// TIMEFRAME RESOLUTION TESTS
// ============================================================================

#include "epochflow/compiler/timeframe_resolver.h"
#include "epoch_frame/factory/date_offset_factory.h"

using epoch_stratifyx::epochflow::TimeframeResolver;

TEST_CASE("TimeframeResolver: Resolves from base timeframe", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));

    auto result = resolver.ResolveTimeframe("test_node", {}, baseTimeframe);

    REQUIRE(result.has_value());
    REQUIRE(result.value().ToString() == baseTimeframe.ToString());
}

TEST_CASE("TimeframeResolver: Resolves from input timeframes", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));
    auto inputTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(5));

    // Set up input node with timeframe
    resolver.nodeTimeframes["input1"] = inputTimeframe;

    auto result = resolver.ResolveTimeframe("test_node", {"input1#result"}, baseTimeframe);

    REQUIRE(result.has_value());
    REQUIRE(result.value().ToString() == inputTimeframe.ToString());
}

TEST_CASE("TimeframeResolver: Uses lowest resolution from multiple inputs", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));
    auto timeframe1Min = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));
    auto timeframe5Min = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(5));

    // Set up input nodes with different timeframes
    resolver.nodeTimeframes["input1"] = timeframe5Min; // Lower resolution (higher timeframe value)
    resolver.nodeTimeframes["input2"] = timeframe1Min; // Higher resolution (lower timeframe value)

    auto result = resolver.ResolveTimeframe("test_node", {"input1#result", "input2#result"}, baseTimeframe);

    REQUIRE(result.has_value());
    // Should pick the maximum (lowest resolution) timeframe
    REQUIRE(result.value().ToString() == timeframe5Min.ToString());
}

TEST_CASE("TimeframeResolver: Caching works correctly", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(15));
    auto inputTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(5));

    // Pre-populate cache
    resolver.nodeTimeframes["input1"] = inputTimeframe;

    // First call should resolve and cache
    auto result1 = resolver.ResolveTimeframe("test_node", {"input1#result"}, baseTimeframe);
    REQUIRE(result1.has_value());
    REQUIRE(result1.value().ToString() == inputTimeframe.ToString());

    // Verify it was cached
    REQUIRE(resolver.nodeTimeframes.contains("test_node"));
    REQUIRE(resolver.nodeTimeframes["test_node"] == result1);

    // Second call should return cached value (even with different base timeframe)
    auto differentBase = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(30));
    auto result2 = resolver.ResolveTimeframe("test_node", {"input1#result"}, differentBase);
    REQUIRE(result2.has_value());
    REQUIRE(result2.value().ToString() == result1.value().ToString());
}

TEST_CASE("TimeframeResolver: ResolveNodeTimeframe uses explicit node timeframe", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));
    auto nodeTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(5));

    epoch_metadata::strategy::AlgorithmNode node;
    node.id = "test_node";
    node.timeframe = nodeTimeframe;

    auto result = resolver.ResolveNodeTimeframe(node, baseTimeframe);

    REQUIRE(result.has_value());
    REQUIRE(result.value().ToString() == nodeTimeframe.ToString());
    REQUIRE(resolver.nodeTimeframes["test_node"] == nodeTimeframe);
}

TEST_CASE("TimeframeResolver: ResolveNodeTimeframe falls back to base timeframe", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));

    epoch_metadata::strategy::AlgorithmNode node;
    node.id = "test_node";
    // node.timeframe is not set
    // node has no inputs

    auto result = resolver.ResolveNodeTimeframe(node, baseTimeframe);

    REQUIRE(result.has_value());
    REQUIRE(result.value().ToString() == baseTimeframe.ToString());
}

TEST_CASE("TimeframeResolver: ResolveNodeTimeframe resolves from inputs", "[timeframe_resolution]")
{
    TimeframeResolver resolver;
    auto baseTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(1));
    auto inputTimeframe = epoch_metadata::TimeFrame(epoch_frame::factory::offset::minutes(15));

    // Pre-populate cache with input node timeframe
    resolver.nodeTimeframes["input_node"] = inputTimeframe;

    epoch_metadata::strategy::AlgorithmNode node;
    node.id = "test_node";
    // node.timeframe is not set - should resolve from inputs
    node.inputs["SLOT0"] = {"input_node#result"};
    node.inputs["SLOT1"] = {"input_node#result"};

    auto result = resolver.ResolveNodeTimeframe(node, baseTimeframe);

    REQUIRE(result.has_value());
    // Should resolve to input's timeframe (15min)
    REQUIRE(result.value().ToString() == inputTimeframe.ToString());
}

TEST_CASE("Compiler Integration: Timeframe resolution in compilation", "[timeframe_resolution][integration]")
{
    const std::string source = R"(
mds = market_data_source(timeframe="15Min")
sma_node = sma(period=14)(mds.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(source);

    // Find the sma node
    bool found_sma = false;
    for (const auto &node : result)
    {
        if (node.type == "sma")
        {
            found_sma = true;
            // SMA should inherit timeframe from its input (mds with 15Min)
            REQUIRE(node.timeframe.has_value());
            REQUIRE(node.timeframe->ToString() == "15Min");
            break;
        }
    }

    REQUIRE(found_sma);
}

TEST_CASE("Compiler Integration: Multiple input timeframe resolution", "[timeframe_resolution][integration]")
{
    const std::string source = R"(
mds1 = market_data_source(timeframe="1Min")
mds2 = market_data_source(timeframe="5Min")
fast_sma = sma(period=10)(mds1.c)
slow_sma = sma(period=20)(mds2.c)
cross = gt()(fast_sma, slow_sma)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(source);

    // Find the gt node (cross)
    bool found_cross = false;
    for (const auto &node : result)
    {
        if (node.type == "gt" && node.id == "cross")
        {
            found_cross = true;
            // Cross should use the lowest resolution (5Min) from its inputs
            REQUIRE(node.timeframe.has_value());
            REQUIRE(node.timeframe->ToString() == "5Min");
            break;
        }
    }

    REQUIRE(found_cross);
}
