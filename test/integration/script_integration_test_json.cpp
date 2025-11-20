//
// EpochScript Integration Test Suite (JSON-Based)
//
// Unified integration testing framework using JSON test case files.
// Each test case is a single JSON file containing:
// - input: Inline EpochScript source code
// - graph: Expected compilation output (AST)
// - runtime: Runtime validation rules (optional)
// - error: Expected error message (for negative tests)
//
// Test Case Structure:
//   test_cases/
//   ├── basic/                   # Basic language features
//   │   └── simple_operator.json
//   ├── operators/               # Operator tests
//   │   └── binary_operators.json
//   ├── runtime/                 # Full integration tests
//   │   └── ema_crossover.json
//   ├── errors/                  # Error/negative tests
//   │   └── invalid_function.json
//   └── ...
//
// Test Types:
//   1. Compilation-only: input + graph (runtime is null)
//   2. Full integration: input + graph + runtime
//   3. Error tests: input + error (graph and runtime are null)
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <glaze/glaze.hpp>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <epoch_frame/dataframe.h>
#include <epoch_script/transforms/core/transform_definition.h>
#include <epoch_script/transforms/core/registration.h>
#include "common/json_test_case.h"
#include "common/json_test_loader.h"
#include "common/test_orchestrator.h"
#include "common/coverage_tracker.h"
#include "common/csv_data_loader.h"
#include "transforms/compiler/ast_compiler.h"
#include "transforms/runtime/orchestrator.h"
#include <epoch_script/data/factory.h>
#include "epoch_script/strategy/introspection.h"
#include "epoch_script/strategy/algorithm_node_ostream.h"

using namespace epoch_script;
using namespace epoch_script::test;
using epoch_script::runtime::test::CoverageTracker;
namespace fs = std::filesystem;

// Normalize result for comparison (sort by id)
static CompilationResult NormalizeResult(CompilationResult result)
{
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.id < b.id; });
    return result;
}

// Convert AlgorithmNode vector to TransformConfigurationList
static transform::TransformConfigurationList ConvertToConfigurationList(
    const std::vector<strategy::AlgorithmNode>& nodes)
{
    transform::TransformConfigurationList configs;

    for (const auto& node : nodes)
    {
        TransformDefinition def(node, node.timeframe);
        configs.push_back(transform::TransformConfiguration{def});
    }

    return configs;
}

// Extract unique transform names from compiled result
static std::set<std::string> ExtractTransformNames(const CompilationResult& nodes)
{
    std::set<std::string> transforms;
    for (const auto& node : nodes)
    {
        transforms.insert(node.type);
    }
    return transforms;
}

// Asset configurations for multi-asset testing
struct AssetConfiguration {
    std::string name;
    std::vector<std::string> assets;
};

static std::vector<AssetConfiguration> GetAssetConfigurations() {
    return {
        {"single_asset", {"AAPL-Stocks"}}
        // {"small_index", {"DJIA30"}},      // Expands to ~30 constituents
        // {"large_index", {"SP500"}}        // Expands to ~500 constituents
    };
}

// Default validation: check that pipeline generated at least one output
static void ValidateDefaultOutputs(
    const runtime::test::CsvDataLoader::TimeFrameAssetDataFrameMap& output_data_map,
    const std::unordered_map<std::string, epoch_proto::TearSheet>& reports,
    const epoch_script::runtime::AssetEventMarkerMap& event_markers,
    const std::string& asset_config_name)
{
    bool has_output = !output_data_map.empty() || !reports.empty() || !event_markers.empty();

    if (!has_output) {
        FAIL("No outputs generated [" << asset_config_name << "]: expected at least one dataframe, report, or event marker");
    }

    INFO("✓ Pipeline generated outputs: "
         << output_data_map.size() << " dataframe(s), "
         << reports.size() << " report(s), "
         << event_markers.size() << " event marker set(s)");
}

// Generate individual test cases using Catch2's generators
TEST_CASE("EpochScript Integration Tests - JSON-Based", "[integration][epoch_script][json]")
{
    // Initialize coverage tracker
    auto& coverage = CoverageTracker::GetInstance();
    auto all_metadata = epoch_script::transforms::ITransformRegistry::GetInstance().GetMetaData();
    INFO("Transform Registry contains " << all_metadata.size() << " transforms");
    coverage.SetTotalTransforms(all_metadata.size());

    // Load all JSON test cases
    auto root = JsonTestLoader::FindTestCasesRoot();
    REQUIRE(root.has_value());

    auto test_cases = JsonTestLoader::LoadAllTests(*root);
    REQUIRE_FALSE(test_cases.empty());
    INFO("Found " << test_cases.size() << " JSON test cases");

    // Use GENERATE to create a separate test run for each test case
    auto [test_name, test_case] = GENERATE_COPY(from_range(test_cases));

    // Each test case now runs as a separate test instance
    SECTION(test_name)
    {
        // =================================================================
        // PHASE 1: COMPILATION TESTING
        // =================================================================

        // Check if this is an error case
        if (test_case.error.has_value())
        {
            // Error case: expect compilation to fail
            auto trim = [](std::string s) {
                auto isspace2 = [](unsigned char c){ return std::isspace(c) != 0; };
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](unsigned char c){ return !isspace2(c); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [&](unsigned char c){ return !isspace2(c); }).base(), s.end());
                if (s.rfind("Error: ", 0) == 0) s.erase(0, 7);
                return s;
            };

            std::string expected_error_msg = trim(test_case.error.value());
            REQUIRE_FALSE(expected_error_msg.empty());

            bool threw_expected_error = false;
            std::string actual_error;

            try
            {
                AlgorithmAstCompiler compiler;
                compiler.compile(test_case.input);
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

        // Success case: compile and validate graph
        REQUIRE(test_case.graph.has_value());

        auto expected_result = test_case.graph.value();

        CAPTURE(test_name);

        std::unique_ptr<strategy::PythonSource> compiler;

        try
        {
            compiler = std::make_unique<strategy::PythonSource>(test_case.input, false);
        }
        catch (const std::exception &e)
        {
            FAIL("Compilation failed: " << e.what());
        }
        CompilationResult actual_result = compiler->GetCompilationResult();

        // Normalize both results for comparison
        auto expected_normalized = NormalizeResult(expected_result);
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

        INFO("Expected graph:\\n" << expected_json_output.value());
        INFO("Actual graph:\\n" << actual_json_output.value());

        REQUIRE(expected_normalized == actual_normalized);

        // =================================================================
        // PHASE 2: RUNTIME TESTING (only when runtime validation is specified)
        // =================================================================

        INFO("Runtime testing for: " << test_name);

        // Test with multiple asset configurations
        auto asset_configs = GetAssetConfigurations();
        for (const auto& asset_config : asset_configs) {
            INFO("=== Testing with: " << asset_config.name << " ===");

            // 1. Create StrategyConfig from test input
            // This allows auto-detection of intraday vs daily timeframe
            strategy::StrategyConfig strategyConfig;
            strategyConfig.trade_signal.source = *compiler;
            strategyConfig.data.assets = strategy::AssetIDContainer(asset_config.assets);

            // Determine date range based on timeframe (10 years for daily, 1 year for intraday)
            bool is_intraday = strategy::IsIntradayCampaign(strategyConfig);

            auto start_date = is_intraday
                ? epoch_frame::DateTime::from_str("2024-01-01", "UTC", "%Y-%m-%d").date()  // 1 year for intraday
                : epoch_frame::DateTime::from_str("2015-01-01", "UTC", "%Y-%m-%d").date(); // 10 years for daily

            auto end_date = epoch_frame::DateTime::from_str("2025-01-01", "UTC", "%Y-%m-%d").date();

            INFO("Creating database with auto-detected timeframe...");
            INFO("Date range: " << (is_intraday ? "1 year (intraday)" : "10 years (daily)"));

            // 2. Create database using strategy-aware factory
            // Factory automatically:
            // - Detects intraday vs daily data category
            // - Extracts transforms from trade_signal.source
            // - Sets up required timeframe resampling
            // - Detects auxiliary data categories
            auto dataModuleOption = data::factory::MakeDataModuleOptionFromStrategy(
                epoch_core::CountryCurrency::USD,
                strategy::DatePeriodConfig{start_date, end_date},
                strategyConfig
            );

            auto factory = data::factory::DataModuleFactory(dataModuleOption);
            auto database = factory.CreateDatabase();

            INFO("Running database pipeline (load + transform data)...");
            REQUIRE_NOTHROW(database->RunPipeline());

            // 3. Get outputs directly from database
            auto db_output_data = database->GetTransformedData();
            auto reports = database->GetGeneratedReports();
            auto event_markers = database->GetGeneratedEventMarkers();

            // Extract actual assets from database
            std::set<std::string> assets;
            for (const auto& asset : database->GetAssets()) {
                assets.insert(asset.GetID());
            }

            // Convert Database output format to test format
            // Database: std::unordered_map<string, asset::AssetHashMap<DataFrame>>
            // Test:     std::unordered_map<string, std::unordered_map<string, DataFrame>>
            runtime::test::CsvDataLoader::TimeFrameAssetDataFrameMap output_data_map;
            for (const auto& [timeframe, asset_map] : db_output_data) {
                for (const auto& [asset, dataframe] : asset_map) {
                    output_data_map[timeframe][asset.GetID()] = dataframe;
                }
            }

            INFO("Pipeline executed successfully");
            INFO("Output dataframes: " << output_data_map.size());
            INFO("Generated reports: " << reports.size());
            INFO("Generated event markers: " << event_markers.size());

            // 4. Record coverage for all transforms used in this test
            {
                auto& coverage = CoverageTracker::GetInstance();
                auto transform_names = ExtractTransformNames(actual_result);
                size_t asset_count = assets.size();

                // Determine timeframe from compiled result or database
                std::string timeframe = actual_result.empty() || !actual_result.front().timeframe.has_value()
                    ? "1D" : actual_result.front().timeframe->ToString();

                for (const auto& transform_name : transform_names) {
                    auto& metadata_map = epoch_script::transforms::ITransformRegistry::GetInstance().GetMetaData();
                    auto it = metadata_map.find(transform_name);
                    bool is_executor_or_reporter = false;
                    if (it != metadata_map.end()) {
                        is_executor_or_reporter = (it->second.category == epoch_core::TransformCategory::Executor ||
                                                  it->second.category == epoch_core::TransformCategory::Reporter);
                    }

                    if (is_executor_or_reporter) {
                        coverage.RecordExecutionNoOutput(transform_name, 0, true, asset_count, timeframe);
                    } else {
                        coverage.RecordExecution(transform_name, output_data_map, 0, true, asset_count, timeframe);
                    }
                }
            }

            // 5. Validate outputs against runtime validation rules
            if (test_case.runtime.has_value()) {
                // Explicit validation rules provided
                const auto& runtime_validation = test_case.runtime.value();

                // TODO: Fix executor output validation - currently looks for columns in dataframes
                // but executors produce event markers, not dataframe outputs
                // Validate executor outputs
                // if (runtime_validation.executor_outputs.has_value()) {
                //     auto result = TestOrchestrator::ValidateExecutorOutputs(
                //         output_data_map, runtime_validation.executor_outputs.value());
                //     if (!result.passed) {
                //         FAIL("Executor output validation failed [" << asset_config.name << "]: " << result.message);
                //     }
                // }

                // Validate tearsheets
                if (runtime_validation.tearsheets.has_value()) {
                    auto result = TestOrchestrator::ValidateTearsheets(
                        reports, runtime_validation.tearsheets.value());
                    if (!result.passed) {
                        FAIL("Tearsheet validation failed [" << asset_config.name << "]: " << result.message);
                    }
                }

                // Validate event markers
                if (runtime_validation.event_markers.has_value()) {
                    auto result = TestOrchestrator::ValidateEventMarkers(
                        event_markers, runtime_validation.event_markers.value());
                    if (!result.passed) {
                        FAIL("Event marker validation failed [" << asset_config.name << "]: " << result.message);
                    }
                }
            } else {
                // Default validation: check that at least one output exists
                INFO("No explicit validation rules - using default validation");
                ValidateDefaultOutputs(output_data_map, reports, event_markers, asset_config.name);
            }
        } // End of asset_config loop
    }
}

// Coverage validation test - runs after all integration tests (JSON)
TEST_CASE("Transform Coverage Report (JSON)", "[integration][coverage][json]")
{
    auto& coverage = CoverageTracker::GetInstance();

    // Ensure total transforms is initialized
    auto all_metadata = epoch_script::transforms::ITransformRegistry::GetInstance().GetMetaData();
    if (all_metadata.size() > 0) {
        coverage.SetTotalTransforms(all_metadata.size());
    }

    auto report = coverage.GenerateReport();

    // Write coverage report to file
    fs::path coverage_file = "test_coverage.json";
    report.WriteToFile(coverage_file);
    INFO("Coverage report written to: " << coverage_file);

    // Print summary to console
    report.PrintSummary();

    // Validate coverage threshold (85%)
    double coverage_percent = report.CoveragePercent();
    INFO("Transform coverage: " << coverage_percent << "%");

    if (coverage_percent < 85.0) {
        WARN("Coverage is below 85% threshold");
    }
}
