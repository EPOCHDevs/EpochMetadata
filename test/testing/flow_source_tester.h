#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "csv_data_loader.h"
#include "epochflow/strategy/metadata.h"
#include "epoch_protos/tearsheet.pb.h"

#include <epochflow/runtime/iorchestrator.h>

namespace epoch_flow::runtime::test {

/**
 * @brief Test runner for EpochFlow Python source files
 *
 * Discovers and executes test cases in flow_source_test_cases/ directory.
 * Each test case contains:
 *   - source.py: EpochFlow source code
 *   - input/: CSV files with test data
 *   - expected/: Expected outputs (dataframes, selectors, tearsheets)
 *   - config.yaml: Optional test configuration
 */
class FlowSourceTestRunner {
public:
    using AssetDataFrameMap = CsvDataLoader::AssetDataFrameMap;
    using TimeFrameAssetDataFrameMap = CsvDataLoader::TimeFrameAssetDataFrameMap;
    using AssetSelectorMap = epoch_flow::runtime::AssetSelectorMap;
    using AssetReportMap = epoch_flow::runtime::AssetReportMap;

    struct Config {
        std::vector<std::string> testDirectories = {"flow_source_test_cases"};
        bool recursive = true;
        bool updateMode = false;  // Auto-generate expected outputs
    };

    struct TestCaseConfig {
        std::string title;
        std::string status = "APPROVED";  // or "PENDING_REVIEW"
        std::vector<std::string> assets;  // Optional override
        std::vector<std::string> timeframes;  // Optional override
        bool strict = false;
    };

    struct TestCase {
        std::string name;
        std::filesystem::path directory;
        std::filesystem::path sourceFile;
        std::filesystem::path inputDir;
        std::filesystem::path expectedDir;
        TestCaseConfig config;
    };

    struct TestOutputs {
        TimeFrameAssetDataFrameMap dataframes;
        AssetReportMap tearsheets;
        AssetSelectorMap selectors;
    };

    /**
     * @brief Run all discovered test cases
     * @param config Test runner configuration
     */
    static void RunAllTests(const Config& config);

    /**
     * @brief Discover test case directories
     * @param config Test runner configuration
     * @return Vector of test cases
     */
    static std::vector<TestCase> DiscoverTestCases(const Config& config);

    /**
     * @brief Run a single test case
     * @param testCase The test case to run
     * @param updateMode Whether to auto-generate expected outputs
     */
    static void RunTestCase(const TestCase& testCase, bool updateMode);

private:
    /**
     * @brief Load and compile Python source
     * @param sourceFile Path to source.py
     * @return Compiled PythonSource object
     */
    static epochflow::strategy::PythonSource LoadSource(const std::filesystem::path& sourceFile);

    /**
     * @brief Auto-detect assets required by source code
     * @param source Compiled PythonSource
     * @param inputDir Directory containing input CSVs
     * @return Vector of assets
     */
    static std::vector<std::string> DetectRequiredAssets(
        const epochflow::strategy::PythonSource& source,
        const std::filesystem::path& inputDir);

    /**
     * @brief Check if source requires multiple assets (cross-sectional)
     * @param source Compiled PythonSource
     * @return True if cross-sectional transforms detected
     */
    static bool IsCrossSectional(const epochflow::strategy::PythonSource& source);

    /**
     * @brief Execute test case and get outputs
     * @param source Compiled Python source
     * @param inputData Input DataFrames
     * @param assets Assets to process
     * @param baseTimeframe Base timeframe
     * @return Test outputs (dataframes, tearsheets, selectors)
     */
    static TestOutputs ExecuteTest(
        const epochflow::strategy::PythonSource& source,
        const TimeFrameAssetDataFrameMap& inputData,
        const std::vector<std::string>& assets,
        const std::optional<epochflow::TimeFrame>& baseTimeframe);

    /**
     * @brief Validate test outputs against expected
     * @param testCase The test case
     * @param outputs Actual outputs
     */
    static void ValidateOutputs(const TestCase& testCase, const TestOutputs& outputs);

    /**
     * @brief Auto-generate expected outputs
     * @param testCase The test case
     * @param outputs Actual outputs to save as expected
     */
    static void GenerateExpectedOutputs(const TestCase& testCase, const TestOutputs& outputs);

    /**
     * @brief Load test case config from config.yaml
     * @param configPath Path to config.yaml
     * @return Test case configuration
     */
    static TestCaseConfig LoadConfig(const std::filesystem::path& configPath);

    /**
     * @brief Save test case config to config.yaml
     * @param configPath Path to config.yaml
     * @param config Configuration to save
     */
    static void SaveConfig(const std::filesystem::path& configPath, const TestCaseConfig& config);
};

// Standalone function to run all tests (for use in TEST_CASE)
void FlowSourceTestRunAllTests(const FlowSourceTestRunner::Config& config);

} // namespace epoch_flow::runtime::test
