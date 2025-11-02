//
// EpochScript Integration Test Suite
//
// Unified integration testing framework that tests both compilation and runtime execution
// from a single test case directory structure.
//
// Test Case Structure (Categorized):
//   test_cases/
//   ├── basic/                   # Basic language features
//   ├── operators/               # Operator tests
//   ├── constants/               # Constant folding
//   ├── literals/                # Literal values
//   ├── variables/               # Variable resolution
//   ├── control_flow/            # Conditionals & selection
//   ├── tuples/                  # Tuple handling
//   ├── parameters/              # Parameter handling
//   ├── type_system/             # Type checking & casting
//   ├── transforms/              # Transform-specific tests
//   ├── graphs/                  # Graph topology
//   ├── timeframes/              # Timeframe handling
//   ├── strategies/              # Strategy examples
//   ├── reports/                 # Report generation
//   ├── runtime/                 # Full integration (script + data → output)
//   ├── errors/                  # Error/negative tests
//   ├── string_operations/       # String handling
//   ├── shared_data/             # Reusable CSV datasets
//   └── archived/                # Deprecated tests
//
// Each test directory contains:
//   category/test_name/
//   ├── input.txt                # EpochScript source code
//   ├── input_data/              # Runtime inputs (CSV files) [optional]
//   │   └── 1D_TICKER-AssetClass.csv
//   └── expected/
//       ├── graph.json           # Expected compilation output (AST)
//       ├── dataframes/          # Expected runtime dataframe outputs [optional]
//       ├── tearsheets/          # Expected runtime tearsheet outputs [optional]
//       └── selectors/           # Expected runtime selector outputs [optional]
//
// Test Types:
//   1. Compilation-Only: Has input.txt and expected/graph.json
//   2. Error Tests: Has input.txt and expected/graph.json with {"error": "..."}
//   3. Full Integration: Has input.txt, expected/graph.json, input_data/, and expected outputs
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <glaze/glaze.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;
namespace fs = std::filesystem;

// Test case structure
struct IntegrationTestCase
{
    std::string name;
    fs::path test_dir;
    fs::path input_script;
    fs::path expected_graph;
    fs::path input_data_dir;
    fs::path expected_dataframes_dir;
    fs::path expected_tearsheets_dir;
    fs::path expected_selectors_dir;

    bool has_runtime_test() const {
        return fs::exists(input_data_dir) && !fs::is_empty(input_data_dir);
    }
};

// Error case structure for tests expecting compilation errors
struct CompilerErrorCase
{
    std::string error;
};

// Helper to recursively load all test cases from test_cases directory
std::vector<IntegrationTestCase> LoadIntegrationTestCases()
{
    std::vector<IntegrationTestCase> cases;
    fs::path test_dir = fs::current_path() / "test_cases";

    if (!fs::exists(test_dir))
    {
        return cases;
    }

    // Special directories to skip
    auto skip_dir = [](const std::string& name) {
        return name == "archived" || name == "shared_data";
    };

    // Recursively scan for test cases
    for (const auto &category_entry : fs::directory_iterator(test_dir))
    {
        if (!category_entry.is_directory())
            continue;

        // Skip special directories at root level
        if (skip_dir(category_entry.path().filename().string()))
            continue;

        // Check if this is a test case directory (has input.txt)
        fs::path input = category_entry.path() / "input.txt";
        fs::path expected_dir = category_entry.path() / "expected";
        fs::path expected_graph = expected_dir / "graph.json";

        if (fs::exists(input) && fs::exists(expected_graph))
        {
            // This is a test case at the category level (no subcategories)
            IntegrationTestCase test_case;
            test_case.name = category_entry.path().filename().string();
            test_case.test_dir = category_entry.path();
            test_case.input_script = input;
            test_case.expected_graph = expected_graph;
            test_case.input_data_dir = category_entry.path() / "input_data";
            test_case.expected_dataframes_dir = expected_dir / "dataframes";
            test_case.expected_tearsheets_dir = expected_dir / "tearsheets";
            test_case.expected_selectors_dir = expected_dir / "selectors";

            cases.push_back(test_case);
        }
        else
        {
            // This is a category directory, scan its subdirectories
            for (const auto &test_entry : fs::directory_iterator(category_entry.path()))
            {
                if (!test_entry.is_directory())
                    continue;

                fs::path test_input = test_entry.path() / "input.txt";
                fs::path test_expected_dir = test_entry.path() / "expected";
                fs::path test_expected_graph = test_expected_dir / "graph.json";

                if (fs::exists(test_input) && fs::exists(test_expected_graph))
                {
                    IntegrationTestCase test_case;
                    // Include category in name: "operators/binary_operators"
                    test_case.name = category_entry.path().filename().string() + "/" +
                                    test_entry.path().filename().string();
                    test_case.test_dir = test_entry.path();
                    test_case.input_script = test_input;
                    test_case.expected_graph = test_expected_graph;
                    test_case.input_data_dir = test_entry.path() / "input_data";
                    test_case.expected_dataframes_dir = test_expected_dir / "dataframes";
                    test_case.expected_tearsheets_dir = test_expected_dir / "tearsheets";
                    test_case.expected_selectors_dir = test_expected_dir / "selectors";

                    cases.push_back(test_case);
                }
            }
        }
    }

    std::sort(cases.begin(), cases.end(),
              [](const auto &a, const auto &b)
              { return a.name < b.name; });
    return cases;
}

// Normalize result for comparison (sort by id)
static CompilationResult NormalizeResult(CompilationResult result)
{
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b)
              { return a.id < b.id; });
    return result;
}

// Read file contents
std::string ReadFile(const fs::path &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

// Generate individual test cases using Catch2's generators
TEST_CASE("EpochScript Integration Tests - Compilation + Runtime", "[integration][epoch_script]")
{
    auto test_cases = LoadIntegrationTestCases();

    REQUIRE_FALSE(test_cases.empty());

    INFO("Found " << test_cases.size() << " integration test cases");

    // Use GENERATE to create a separate test run for each test case
    auto test_case = GENERATE_COPY(from_range(test_cases));

    // Each test case now runs as a separate test instance
    SECTION(test_case.name)
    {
            // =================================================================
            // PHASE 1: COMPILATION TESTING
            // =================================================================

            // Read input script
            std::string source = ReadFile(test_case.input_script);

            // Read expected graph.json
            std::string expected_json = ReadFile(test_case.expected_graph);

            // Check if this is an error case
            if (expected_json.find("\"error\"") != std::string::npos)
            {
                // Error case: expect compilation to fail
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

                // Error cases don't have runtime testing
                return;
            }

            // Success case: compile and validate graph.json
            auto expected_result = glz::read_json<CompilationResult>(expected_json);

            if (!expected_result.has_value())
            {
                FAIL("Failed to parse expected graph.json: " << glz::format_error(expected_result.error(), expected_json));
            }

            CAPTURE(test_case.name);

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

            // Normalize both results for comparison
            auto expected_normalized = NormalizeResult(*expected_result);
            auto actual_normalized = NormalizeResult(actual_result);

            // Serialize both to JSON for comparison
            auto expected_json_output = glz::write_json(expected_normalized);
            auto actual_json_output = glz::write_json(actual_normalized);

            if (!expected_json_output.has_value())
            {
                FAIL("Failed to serialize expected result");
            }

            if (!actual_json_output.has_value())
            {
                FAIL("Failed to serialize actual result");
            }

            INFO("Expected graph.json:\n" << expected_json_output.value());
            INFO("Actual graph.json:\n" << actual_json_output.value());

            REQUIRE(expected_normalized == actual_normalized);

            // =================================================================
            // PHASE 2: RUNTIME TESTING (if input_data exists)
            // =================================================================

            if (test_case.has_runtime_test())
            {
                INFO("Runtime testing enabled for: " << test_case.name);

                // TODO: Implement runtime execution and validation
                // 1. Load input data from input_data/ directory (CSV files)
                // 2. Create orchestrator from compiled graph
                // 3. Execute pipeline with input data
                // 4. Validate output dataframes against expected/dataframes/
                // 5. Validate tearsheets against expected/tearsheets/
                // 6. Validate selectors against expected/selectors/

                WARN("Runtime testing not yet implemented");
            }
    }
}
