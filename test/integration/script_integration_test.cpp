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
//       └── event_markers/           # Expected runtime selector outputs [optional]
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
#include <epoch_frame/dataframe.h>
#include <epoch_frame/serialization.h>
#include <epoch_script/transforms/core/transform_definition.h>
#include "common/csv_data_loader.h"
#include "common/runtime_output_validator.h"
#include "transforms/compiler/ast_compiler.h"
#include "transforms/runtime/orchestrator.h"

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
    fs::path expected_event_markers_dir;

    bool has_runtime_test() const {
        return fs::exists(input_data_dir) && !fs::is_empty(input_data_dir);
    }
};

// Error case structure for tests expecting compilation errors (legacy)
// Some JSONs may contain extra keys; prefer robust extraction.

// Special directories to skip
static bool ShouldSkipDirectory(const std::string& name) {
    return name == "archived" || name == "shared_data";
}

// Recursively scan directory for test cases
static void ScanForTestCases(const fs::path& dir, const fs::path& base_dir,
                             std::vector<IntegrationTestCase>& cases)
{
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    // Check if this directory is a test case (has input.txt and expected/graph.json)
    fs::path input = dir / "input.txt";
    fs::path expected_dir = dir / "expected";
    fs::path expected_graph = expected_dir / "graph.json";

    if (fs::exists(input) && fs::exists(expected_graph))
    {
        // This is a test case directory
        IntegrationTestCase test_case;

        // Build relative path from base_dir to create hierarchical name
        fs::path relative_path = fs::relative(dir, base_dir);
        test_case.name = relative_path.string();

        test_case.test_dir = dir;
        test_case.input_script = input;
        test_case.expected_graph = expected_graph;
        test_case.input_data_dir = dir / "input_data";
        test_case.expected_dataframes_dir = expected_dir / "dataframes";
        test_case.expected_tearsheets_dir = expected_dir / "tearsheets";
        test_case.expected_event_markers_dir = expected_dir / "event_markers";

        cases.push_back(test_case);

        // Don't recurse into test case directories
        return;
    }

    // Not a test case, recurse into subdirectories
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_directory())
            continue;

        // Skip special directories
        if (ShouldSkipDirectory(entry.path().filename().string()))
            continue;

        // Recursively scan subdirectory
        ScanForTestCases(entry.path(), base_dir, cases);
    }
}

// Resolve root directory that contains integration test cases.
// Tries several common locations depending on how tests are launched.
static std::optional<fs::path> FindTestCasesRoot() {
    // 1) Running from build/bin: ./test_cases
    fs::path p1 = fs::current_path() / "test_cases";
    if (fs::exists(p1) && fs::is_directory(p1)) return p1;

    // 2) Running from repo root: ./test/integration/test_cases
    fs::path p2 = fs::current_path() / "test" / "integration" / "test_cases";
    if (fs::exists(p2) && fs::is_directory(p2)) return p2;

    // 3) Running from build root: ./bin/test_cases
    fs::path p3 = fs::current_path() / "bin" / "test_cases";
    if (fs::exists(p3) && fs::is_directory(p3)) return p3;

    return std::nullopt;
}

// Helper to recursively load all test cases from test_cases directory
std::vector<IntegrationTestCase> LoadIntegrationTestCases()
{
    std::vector<IntegrationTestCase> cases;
    auto root = FindTestCasesRoot();
    if (!root) {
        return cases;
    }

    // Start recursive scan from resolved root directory
    ScanForTestCases(*root, *root, cases);

    // Sort test cases by name for consistent ordering
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

// Load CSV files from input_data directory
runtime::TimeFrameAssetDataFrameMap LoadInputData(const fs::path& input_data_dir)
{
    runtime::test::CsvDataLoader loader;
    return loader.LoadFromDirectory(input_data_dir);
}

// Extract unique assets from input data
std::set<std::string> ExtractAssets(const runtime::TimeFrameAssetDataFrameMap& data_map)
{
    std::set<std::string> assets;

    for (const auto& [timeframe, asset_map] : data_map)
    {
        for (const auto& [asset, df] : asset_map)
        {
            assets.insert(asset);
        }
    }

    return assets;
}

// Convert AlgorithmNode vector to TransformConfigurationList
transform::TransformConfigurationList ConvertToConfigurationList(
    const std::vector<strategy::AlgorithmNode>& nodes)
{
    transform::TransformConfigurationList configs;

    for (const auto& node : nodes)
    {
        // Create TransformDefinition from AlgorithmNode
        TransformDefinition def(node, node.timeframe);
        configs.push_back(transform::TransformConfiguration{def});
    }

    return configs;
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
                // Robustly extract expected error message via regex from JSON
                auto trim = [](std::string s) {
                    auto isspace2 = [](unsigned char c){ return std::isspace(c) != 0; };
                    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](unsigned char c){ return !isspace2(c); }));
                    s.erase(std::find_if(s.rbegin(), s.rend(), [&](unsigned char c){ return !isspace2(c); }).base(), s.end());
                    if (s.rfind("Error: ", 0) == 0) s.erase(0, 7);
                    return s;
                };

                std::string expected_error_msg;
                try {
                    std::regex re(R"(\"error\"\s*:\s*\"([^\"]*)\")");
                    std::smatch m;
                    if (std::regex_search(expected_json, m, re) && m.size() > 1) {
                        expected_error_msg = trim(m[1].str());
                    }
                } catch (...) {
                    // ignore
                }

                REQUIRE_FALSE(expected_error_msg.empty());

                bool threw_expected_error = false;
                std::string actual_error;

                try
                {
                    AlgorithmAstCompiler compiler;
                    compiler.compile(source);
                }
                catch (const std::exception &e)
                {
                    actual_error = trim(e.what());
                    // Accept either direction of containment to be tolerant of prefixes
                    if (actual_error.find(expected_error_msg) != std::string::npos ||
                        expected_error_msg.find(actual_error) != std::string::npos)
                    {
                        threw_expected_error = true;
                    }
                }

                INFO("Expected error containing: " << expected_error_msg);
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
            // PHASE 2: RUNTIME TESTING (only when runtime inputs are provided)
            // =================================================================

            INFO("Runtime testing for: " << test_case.name);

            if (!test_case.has_runtime_test()) {
                INFO("No runtime inputs found. Skipping runtime phase.");
                return;
            }

            // 1. Load input data from input_data/ directory (CSV files)
            auto input_data_map = LoadInputData(test_case.input_data_dir);

            // Normalize timeframe key to compiled graph timeframe if needed
            if (!actual_result.empty() && actual_result.front().timeframe.has_value()) {
                const std::string tf_key = actual_result.front().timeframe->ToString();
                if (!input_data_map.contains(tf_key)) {
                    if (input_data_map.size() == 1) {
                        auto it = input_data_map.begin();
                        input_data_map[tf_key] = std::move(it->second);
                        input_data_map.erase(it->first);
                    }
                }
            }

            // 2. Extract unique assets from input data
            // If no input data, we still need to test with empty asset list
            auto assets = ExtractAssets(input_data_map);
            INFO("Assets: " << assets.size());

            // 3. Convert compiled graph to TransformConfigurationList
            auto config_list = ConvertToConfigurationList(actual_result);

            // 4. Create orchestrator from compiled graph
            auto orchestrator = runtime::CreateDataFlowRuntimeOrchestrator(assets, config_list);
            REQUIRE(orchestrator != nullptr);

            // 5. Execute pipeline with input data
            runtime::TimeFrameAssetDataFrameMap output_data_map;
            REQUIRE_NOTHROW(output_data_map = orchestrator->ExecutePipeline(input_data_map));

            // 6. Get generated reports and event markers
            auto reports = orchestrator->GetGeneratedReports();
            auto event_markers = orchestrator->GetGeneratedEventMarkers();

            INFO("Pipeline executed successfully");
            INFO("Output dataframes: " << output_data_map.size());
            INFO("Generated reports: " << reports.size());
            INFO("Generated event markers: " << event_markers.size());

            // 6.5. Export actual outputs to actual/ directory for review/blessing
            fs::path actual_dir = test_case.test_dir / "actual";
            fs::create_directories(actual_dir / "dataframes");
            fs::create_directories(actual_dir / "tearsheets");
            fs::create_directories(actual_dir / "event_markers");

            // Export dataframes
            for (const auto& [timeframe, asset_map] : output_data_map) {
                for (const auto& [asset, df] : asset_map) {
                    std::string filename = timeframe + "_" + asset + ".csv";
                    fs::path output_path = actual_dir / "dataframes" / filename;
                    runtime::test::CsvDataLoader::WriteCsvFile(df, output_path, true);
                }
            }

            // Export reports (tearsheets) - each asset has one TearSheet
            for (const auto& [asset, report] : reports) {
                // Save as JSON for human readability
                std::string json_filename = asset + "_report.json";
                fs::path json_path = actual_dir / "tearsheets" / json_filename;
                std::string json_str;
                google::protobuf::util::JsonPrintOptions options;
                options.add_whitespace = true;
                options.always_print_primitive_fields = true;
                google::protobuf::util::MessageToJsonString(report, &json_str, options);
                std::ofstream json_ofs(json_path);
                if (json_ofs) {
                    json_ofs << json_str;
                }

                // Also save as binary for exact comparison
                std::string bin_filename = asset + "_report.bin";
                fs::path bin_path = actual_dir / "tearsheets" / bin_filename;
                std::ofstream bin_ofs(bin_path, std::ios::binary);
                if (bin_ofs) {
                    report.SerializeToOstream(&bin_ofs);
                }
            }

            // Export event markers - each asset has a vector of EventMarkerData
            for (const auto& [asset, marker_list] : event_markers) {
                for (size_t i = 0; i < marker_list.size(); ++i) {
                    std::string filename = asset + "_event_marker_" + std::to_string(i) + ".json";
                    fs::path output_path = actual_dir / "event_markers" / filename;
                    std::string json_str;
                    auto json_result = glz::write_json(marker_list[i], json_str);
                    if (!json_result) {
                        std::ofstream ofs(output_path);
                        if (ofs) {
                            ofs << json_str;
                        }
                    }
                }
            }

            // 7. Validate output dataframes against expected/dataframes/
            auto df_result = runtime::test::RuntimeOutputValidator::ValidateDataframes(
                output_data_map, test_case.expected_dataframes_dir);
            if (!df_result.passed) {
                FAIL("Dataframe validation failed: " << df_result.message);
            }

            // 8. Validate tearsheets against expected/tearsheets/
            auto tearsheet_result = runtime::test::RuntimeOutputValidator::ValidateTearsheets(
                reports, test_case.expected_tearsheets_dir);
            if (!tearsheet_result.passed) {
                FAIL("Tearsheet validation failed: " << tearsheet_result.message);
            }

            // 9. Validate event_markers against expected/event_markers/
            auto event_marker_result = runtime::test::RuntimeOutputValidator::ValidateEventMarkers(
                event_markers, test_case.expected_event_markers_dir);
            if (!event_marker_result.passed) {
                FAIL("Event marker validation failed: " << event_marker_result.message);
            }
    }
}
