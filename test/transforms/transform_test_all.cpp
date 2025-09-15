//
// Generic transform tester that loads all YAML test cases
//
#include "catch_transform_tester.hpp"
#include "epoch_frame/factory/index_factory.h"
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_definition.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_metadata/strategy/registration.h>
#include <filesystem>

using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace epoch_metadata::transforms;
using namespace epoch_core;
using namespace epoch::test;

// Build TransformDefinition from test options using YAML
TransformDefinition buildTransformDefinition(const Options& testOptions,
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

// Generic transform runner using TransformConfiguration
epoch_frame::DataFrame runTransformWithConfig(const epoch_frame::DataFrame& input,
                                              const Options& options) {
    // Build TransformDefinition from options
    TransformDefinition definition = buildTransformDefinition(options, input);

    // Create TransformConfiguration from definition
    TransformConfiguration config(std::move(definition));

    // Create transform using registry
    auto transformPtr = TransformRegistry::GetInstance().Get(config);
    if (!transformPtr) {
        std::string transformName;
        auto nameIt = options.find("transform_name");
        if (nameIt != options.end() && std::holds_alternative<std::string>(nameIt->second)) {
            transformName = std::get<std::string>(nameIt->second);
        }
        throw std::runtime_error("Failed to create transform: " + transformName);
    }

    // Cast to ITransform to call TransformData
    auto transform = dynamic_cast<ITransform*>(transformPtr.get());
    if (!transform) {
        throw std::runtime_error("Transform does not implement ITransform interface");
    }

    // Run the transform
    return transform->TransformData(input);
}

TEST_CASE("All Transform Tests - YAML Based", "[Transform][YAML]") {
    // Register DataFrame type once
    static bool registered = false;
    if (!registered) {
        registerDataFrameType();
        registered = true;
    }

    // Find all YAML test files in test_cases directory
    std::string testCasesDir = "test_cases";
    std::vector<std::string> testFiles;

    if (std::filesystem::exists(testCasesDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(testCasesDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                testFiles.push_back(entry.path().string());
            }
        }
    } else {
        WARN("Test cases directory not found: " << testCasesDir);
        return;  // Skip test if no test cases directory
    }

    // Sort files for consistent test ordering
    std::sort(testFiles.begin(), testFiles.end());

    INFO("Found " << testFiles.size() << " test files");

    // Process each test file
    for (const auto& testFile : testFiles) {
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
                continue;
            }

            INFO("Loaded " << testCases.size() << " test cases from " << testFile);

            // Run each test case
            for (auto& testCase : testCases) {
                SECTION(testCase.title) {
                    // Convert input Table to DataFrame
                    epoch_frame::DataFrame inputDf = CatchTransformTester::tableToDataFrame(testCase.input);

                    INFO("Test: " << testCase.title);
                    INFO("Input DataFrame:\n" << inputDf);

                    // Log options
                    std::stringstream optStr;
                    optStr << "Options: {";
                    bool first = true;
                    for (const auto& [key, value] : testCase.options) {
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
                    INFO(optStr.str());

                    // Run transform
                    epoch_frame::DataFrame outputDf;
                    try {
                        outputDf = runTransformWithConfig(inputDf, testCase.options);
                    } catch (const std::exception& e) {
                        FAIL("Transform failed: " << e.what());
                        continue;
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
            }
        }
    }
}