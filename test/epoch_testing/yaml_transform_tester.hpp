#ifndef YAML_TRANSFORM_TESTER_HPP
#define YAML_TRANSFORM_TESTER_HPP

#include "epoch_testing/catch_transform_tester.hpp"
#include "epoch_testing/tearsheet_output.hpp"
#include "epoch_testing/transform_tester_base.hpp"
#include "epoch_testing/dataframe_tester.hpp"
#include "epoch_frame/factory/index_factory.h"
#include "epochflow/bar_attribute.h"
#include "epochflow/transforms/itransform.h"
#include "epochflow/transforms/transform_configuration.h"
#include "epochflow/transforms/transform_definition.h"
#include "epochflow/transforms/transform_registry.h"
#include "epochflow/reports/ireport.h"
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epochflow/strategy/registration.h>
#include <filesystem>
#include <vector>
#include <string>

using namespace epochflow;
using namespace epochflow::transform;

namespace epoch {
namespace test {

/**
 * YAML-based transform testing utility that provides configurable test case discovery
 * and automated test execution using Catch2.
 *
 * This class allows libraries to easily run YAML-defined transform tests with
 * customizable test case directory paths.
 */
class YamlTransformTester {
public:
    /**
     * Configuration for test case discovery and execution
     */
    struct Config {
        std::vector<std::string> testDirectories;  // Directories to search for test cases
        bool recursive = true;                     // Whether to search recursively
        std::string fileExtension = ".yaml";      // File extension to look for
        bool requireTestCasesDir = false;         // Whether to fail if no test directories exist

        // Default constructor with common test directory names
        Config() : testDirectories({"test_cases", "tests", "test"}) {}

        // Constructor with custom test directories
        explicit Config(const std::vector<std::string>& dirs) : testDirectories(dirs) {}

        // Constructor with single test directory
        explicit Config(const std::string& dir) : testDirectories({dir}) {}
    };

    /**
     * Run all YAML transform tests found in configured directories
     * @param config Configuration for test discovery
     * @param transformAdapter Function to run the transform with given input and options
     */
    static void runAllTests(
        const Config& config,
        std::function<epoch_frame::DataFrame(const epoch_frame::DataFrame&, const Options&)> transformAdapter) {

        // Register output types once
        static bool registered = false;
        if (!registered) {
            registerDataFrameType();
            registerTearsheetType();
            registered = true;
        }

        // Find all test files
        std::vector<std::string> allTestFiles = findAllTestFiles(config);

        if (allTestFiles.empty()) {
            if (config.requireTestCasesDir) {
                FAIL("No test files found in any of the configured directories");
            } else {
                WARN("No test files found in any of the configured directories");
                return;
            }
        }

        // Sort files for consistent test ordering
        std::sort(allTestFiles.begin(), allTestFiles.end());

        INFO("Found " << allTestFiles.size() << " test files across "
             << config.testDirectories.size() << " directories");

        // Process each test file
        for (const auto& testFile : allTestFiles) {
            runTestFile(testFile, transformAdapter);
        }
    }

    /**
     * Run transform tests using the standard transform registry approach
     * This is a convenience method for the common use case
     */
    static void runTransformRegistryTests(const Config& config = Config()) {
        runAllTests(config, runTransformWithConfig);
    }

    /**
     * Find all test files in the configured directories
     */
    static std::vector<std::string> findAllTestFiles(const Config& config) {
        std::vector<std::string> allTestFiles;

        for (const auto& testDir : config.testDirectories) {
            auto files = findTestFilesInDirectory(testDir, config);
            allTestFiles.insert(allTestFiles.end(), files.begin(), files.end());
        }

        return allTestFiles;
    }

private:
    /**
     * Find test files in a specific directory
     */
    static std::vector<std::string> findTestFilesInDirectory(
        const std::string& directory,
        const Config& config) {

        std::vector<std::string> testFiles;

        if (!std::filesystem::exists(directory)) {
            return testFiles;
        }

        if (config.recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == config.fileExtension) {
                    testFiles.push_back(entry.path().string());
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == config.fileExtension) {
                    testFiles.push_back(entry.path().string());
                }
            }
        }

        return testFiles;
    }

    /**
     * Run tests from a single YAML file
     */
    static void runTestFile(
        const std::string& testFile,
        std::function<epoch_frame::DataFrame(const epoch_frame::DataFrame&, const Options&)> transformAdapter) {

        // Extract a clean name for the section
        std::filesystem::path filePath(testFile);
        std::string sectionName = filePath.stem().string() + " [" +
                                 filePath.parent_path().filename().string() + "]";

        SECTION(sectionName) {
            INFO("Loading test file: " << testFile);

            // Load test cases from YAML
            std::vector<DataFrameTransformTester::TestCaseType> testCases;
            try {
                testCases = DataFrameTransformTester::loadTestsFromYAML(testFile);
            } catch (const std::exception& e) {
                FAIL("Failed to load test cases from " << testFile << ": " << e.what());
                return;
            }

            INFO("Loaded " << testCases.size() << " test cases from " << testFile);

            // Run each test case
            for (auto& testCase : testCases) {
                SECTION(testCase.title) {
                    // Convert input Table to DataFrame with timestamp and index support
                    epoch_frame::DataFrame inputDf = CatchTransformTester::tableToDataFrame(
                        testCase.input, testCase.timestamp_columns, testCase.index_column);

                    INFO("Test: " << testCase.title);
                    INFO("Input DataFrame:\n" << inputDf);
                    INFO(formatOptions(testCase.options));

                    // Unified runner that handles both transforms and reports
                    runUnifiedTest(inputDf, testCase, transformAdapter);
                }
            }
        }
    }

    /**
     * Unified test runner that handles both transforms and reports
     */
    static void runUnifiedTest(
        const epoch_frame::DataFrame& inputDf,
        DataFrameTransformTester::TestCaseType& testCase,
        std::function<epoch_frame::DataFrame(const epoch_frame::DataFrame&, const Options&)> transformAdapter) {

        // Determine test type based on expected output
        bool isReportTest = testCase.expect && testCase.expect->getType() == "tearsheet";

        if (isReportTest) {
            INFO("Running report test (tearsheet output)");
            runReportTest(inputDf, testCase);
        } else {
            INFO("Running transform test (DataFrame output)");
            runTransformTest(inputDf, testCase, transformAdapter);
        }
    }

    /**
     * Run a report test that expects tearsheet output
     */
    static void runReportTest(
        const epoch_frame::DataFrame& inputDf,
        DataFrameTransformTester::TestCaseType& testCase) {

        std::unique_ptr<TearsheetOutput> actualOutput;
        try {
            actualOutput = runReportWithConfig(inputDf, testCase.options);
        } catch (const std::exception& e) {
            FAIL("Report generation failed: " << e.what());
            return;
        }

        INFO("Generated tearsheet output");

        // Compare with expected tearsheet output
        if (testCase.expect) {
            INFO("Expected:\n" << testCase.expect->toString());
            INFO("Actual:\n" << actualOutput->toString());
            REQUIRE(actualOutput->equals(*testCase.expect));
        } else {
            REQUIRE(actualOutput == nullptr);
        }
    }

    /**
     * Run a transform test that expects DataFrame output
     */
    static void runTransformTest(
        const epoch_frame::DataFrame& inputDf,
        DataFrameTransformTester::TestCaseType& testCase,
        std::function<epoch_frame::DataFrame(const epoch_frame::DataFrame&, const Options&)> transformAdapter) {

        epoch_frame::DataFrame outputDf;
        try {
            outputDf = transformAdapter(inputDf, testCase.options);
        } catch (const std::exception& e) {
            FAIL("Transform failed: " << e.what());
            return;
        }

        INFO("Output DataFrame:\n" << outputDf);

        // Convert output to Table for comparison
        Table outputTable = CatchTransformTester::dataFrameToTable(outputDf);
        auto actualOutput = std::make_unique<DataFrameOutput>(outputTable);

        // Compare with expected output
        if (testCase.expect) {
            INFO("Expected:\n" << testCase.expect->toString());
            INFO("Actual:\n" << actualOutput->toString());
            REQUIRE(actualOutput->equals(*testCase.expect));
        } else {
            REQUIRE(outputTable.empty());
        }
    }

    /**
     * Format options for logging
     */
    static std::string formatOptions(const Options& options) {
        std::stringstream optStr;
        optStr << "Options: {";
        bool first = true;
        for (const auto& [key, value] : options) {
            if (!first) optStr << ", ";
            first = false;
            optStr << key << ": ";
            if (std::holds_alternative<bool>(value)) {
                optStr << (std::get<bool>(value) ? "true" : "false");
            } else if (std::holds_alternative<double>(value)) {
                optStr << std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                optStr << "\"" << std::get<std::string>(value) << "\"";
            }
        }
        optStr << "}";
        return optStr.str();
    }

    /**
     * Build TransformDefinition from test options using YAML
     * This is the same logic as in the original transform_test_all.cpp
     */
    static TransformDefinition buildTransformDefinition(const Options& testOptions,
                                                       const epoch_frame::DataFrame& input) {
        // Build a YAML node from the test options
        YAML::Node yamlNode;

        // Extract and set transform name (required)
        auto nameIt = testOptions.find("transform_name");
        if (nameIt != testOptions.end() && std::holds_alternative<std::string>(nameIt->second)) {
            yamlNode["type"] = std::get<std::string>(nameIt->second);
        } else {
            throw std::runtime_error("transform_name not specified in options");
        }

        // Extract and set output ID
        auto idIt = testOptions.find("output_id");
        if (idIt != testOptions.end() && std::holds_alternative<std::string>(idIt->second)) {
            yamlNode["id"] = std::get<std::string>(idIt->second);
        } else {
            yamlNode["id"] = yamlNode["type"].as<std::string>();
        }

        // Check if explicit inputs are specified in options
        auto inputsIt = testOptions.find("inputs");
        YAML::Node inputs;
        if (inputsIt != testOptions.end() && std::holds_alternative<std::string>(inputsIt->second)) {
            // Parse the inputs string as YAML
            std::string inputsStr = std::get<std::string>(inputsIt->second);
            try {
                inputs = YAML::Load(inputsStr);
            } catch (const YAML::Exception& e) {
                throw std::runtime_error("Failed to parse inputs YAML: " + std::string(e.what()));
            }
        } else {
            // Build default input mapping from DataFrame columns
            for (const auto& colName : input.column_names()) {
                inputs[colName] = colName;  // Direct 1-1 mapping
            }
        }
        yamlNode["inputs"] = inputs;

        // Convert test options to YAML options
        YAML::Node options;
        for (const auto& [key, value] : testOptions) {
            // Skip special keys
            if (key == "transform_name" || key == "output_id" ||
                key == "timeframe" || key == "session" || key == "inputs") {
                continue;
            }

            if (std::holds_alternative<bool>(value)) {
                options[key] = std::get<bool>(value);
            } else if (std::holds_alternative<double>(value)) {
                options[key] = std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                options[key] = std::get<std::string>(value);
            }
        }
        if (options.size() > 0) {
            yamlNode["options"] = options;
        }

        // Extract and set timeframe if specified
        auto tfIt = testOptions.find("timeframe");
        if (tfIt != testOptions.end() && std::holds_alternative<std::string>(tfIt->second)) {
            yamlNode["timeframe"] = std::get<std::string>(tfIt->second);
        }

        // Create TransformDefinition from YAML (this should load metadata)
        return TransformDefinition(yamlNode);
    }

    /**
     * Unified runner for reports - reports are transforms that implement IReporter
     */
    static std::unique_ptr<TearsheetOutput> runReportWithConfig(const epoch_frame::DataFrame& input,
                                                               const Options& options) {
        // Create transform instance using the unified registry approach
        auto transformPtr = createTransformFromOptions(options, input);

        // Cast to IReporter to access tearsheet functionality
        auto reporter = dynamic_cast<epochflow::reports::IReporter*>(transformPtr.get());
        if (!reporter) {
            std::string transformName = getTransformName(options);
            throw std::runtime_error("Transform '" + transformName + "' does not implement IReporter interface");
        }

        // Run the report transform to generate the dashboard
        auto outputDf = reporter->TransformData(input);

        // Extract the generated tearsheet from the reporter
        epoch_proto::TearSheet protoTearsheet = reporter->GetTearSheet();

        // Create TearsheetOutput for comparison
        auto tearsheet = std::make_unique<TearsheetOutput>();
        tearsheet->protoTearsheet = protoTearsheet;

        return tearsheet;
    }

    /**
     * Convert epoch_proto::TearSheet to test framework TearsheetOutput
     */
    static std::unique_ptr<TearsheetOutput> convertProtoTearsheetToTestOutput(
        const epoch_proto::TearSheet& protoTearsheet) {

        auto testTearsheet = std::make_unique<TearsheetOutput>();

        // Convert cards
        if (protoTearsheet.has_cards()) {
            for (const auto& cardDef : protoTearsheet.cards().cards()) {
                for (const auto& cardData : cardDef.data()) {
                    Card testCard;
                    testCard.title = cardData.title();

                    // Convert epoch_proto::Scalar to test Value
                    if (cardData.has_value()) {
                        testCard.value = convertProtoScalarToTestValue(cardData.value());
                    }

                    testTearsheet->cards.push_back(testCard);
                }
            }
        }

        // TODO: Convert charts and tables when needed

        return testTearsheet;
    }

    /**
     * Convert epoch_proto::Scalar to test framework Value
     */
    static Value convertProtoScalarToTestValue(const epoch_proto::Scalar& scalar) {
        switch (scalar.value_case()) {
            case epoch_proto::Scalar::kStringValue:
                return scalar.string_value();
            case epoch_proto::Scalar::kIntegerValue:
                return static_cast<double>(scalar.integer_value());
            case epoch_proto::Scalar::kDecimalValue:
                return scalar.decimal_value();
            case epoch_proto::Scalar::kPercentValue:
                return scalar.percent_value();
            case epoch_proto::Scalar::kBooleanValue:
                return scalar.boolean_value();
            case epoch_proto::Scalar::kTimestampMs:
                return static_cast<double>(scalar.timestamp_ms());
            case epoch_proto::Scalar::kDateValue:
                return static_cast<double>(scalar.date_value());
            case epoch_proto::Scalar::kDayDuration:
                return static_cast<double>(scalar.day_duration());
            case epoch_proto::Scalar::kMonetaryValue:
                return scalar.monetary_value();
            case epoch_proto::Scalar::kDurationMs:
                return static_cast<double>(scalar.duration_ms());
            case epoch_proto::Scalar::kNullValue:
            case epoch_proto::Scalar::VALUE_NOT_SET:
            default:
                return std::string("null"); // Return "null" string for null values
        }
    }

    /**
     * Unified transform runner using TransformConfiguration
     */
    static epoch_frame::DataFrame runTransformWithConfig(const epoch_frame::DataFrame& input,
                                                        const Options& options) {
        // Create transform instance using the unified registry approach
        auto transformPtr = createTransformFromOptions(options, input);

        // Cast to ITransform (all transforms should implement this)
        auto transform = dynamic_cast<ITransform*>(transformPtr.get());
        if (!transform) {
            std::string transformName = getTransformName(options);
            throw std::runtime_error("Transform '" + transformName + "' does not implement ITransform interface");
        }

        // Run the transform
        return transform->TransformData(input);
    }

    /**
     * Helper to create transform from options using the registry
     */
    static std::unique_ptr<ITransformBase> createTransformFromOptions(const Options& options,
                                                                      const epoch_frame::DataFrame& input) {
        // Build TransformDefinition from options
        TransformDefinition definition = buildTransformDefinition(options, input);

        // Create TransformConfiguration from definition
        TransformConfiguration config(std::move(definition));

        // Create transform using registry
        auto transformPtr = TransformRegistry::GetInstance().Get(config);
        if (!transformPtr) {
            std::string transformName = getTransformName(options);
            throw std::runtime_error("Failed to create transform: " + transformName);
        }

        return transformPtr;
    }

    /**
     * Helper to extract transform name from options
     */
    static std::string getTransformName(const Options& options) {
        auto nameIt = options.find("transform_name");
        if (nameIt != options.end() && std::holds_alternative<std::string>(nameIt->second)) {
            return std::get<std::string>(nameIt->second);
        }
        return "unknown";
    }
};

} // namespace test
} // namespace epoch

#endif // YAML_TRANSFORM_TESTER_HPP