#ifndef YAML_REPORT_TESTER_HPP
#define YAML_REPORT_TESTER_HPP

#include "epoch_testing/tearsheet_output.hpp"
#include "epoch_testing/catch_transform_tester.hpp"
#include <epoch_core/catch_defs.h>
#include <filesystem>
#include <vector>
#include <string>

namespace epoch {
namespace test {

/**
 * YAML-based report testing utility that provides configurable test case discovery
 * and automated test execution using Catch2.
 *
 * This class allows libraries to easily run YAML-defined report tests with
 * customizable test case directory paths.
 */
class YamlReportTester {
public:
    /**
     * Configuration for test case discovery and execution
     */
    struct Config {
        std::vector<std::string> testDirectories;  // Directories to search for test cases
        bool recursive = true;                     // Whether to search recursively
        std::string fileExtension = ".yaml";       // File extension to look for
        bool requireTestCasesDir = false;          // Whether to fail if no test directories exist

        // Default constructor with common test directory names
        Config() : testDirectories({"report_test_cases", "test_cases", "tests"}) {}

        // Constructor with custom test directories
        explicit Config(const std::vector<std::string>& dirs) : testDirectories(dirs) {}

        // Constructor with single test directory
        explicit Config(const std::string& dir) : testDirectories({dir}) {}
    };

    /**
     * Run all YAML report tests found in configured directories
     * @param config Configuration for test discovery
     * @param reportAdapter Function to run the report with given input and options
     */
    static void runAllTests(
        const Config& config,
        std::function<std::unique_ptr<TearsheetOutput>(const epoch_frame::DataFrame&, const Options&)> reportAdapter) {

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
            runTestFile(testFile, reportAdapter);
        }
    }

    /**
     * Run report tests using a custom report runner
     * This should be implemented in the actual test file
     */
    template<typename ReportRunner>
    static void runReportTests(const Config& config, ReportRunner runner) {
        runAllTests(config, [runner](const epoch_frame::DataFrame& input, const Options& options) {
            return runner(input, options);
        });
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
        std::function<std::unique_ptr<TearsheetOutput>(const epoch_frame::DataFrame&, const Options&)> reportAdapter) {

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
                    // Convert input Table to DataFrame
                    epoch_frame::DataFrame inputDf = CatchTransformTester::tableToDataFrame(testCase.input);

                    INFO("Test: " << testCase.title);
                    INFO("Input DataFrame:\n" << inputDf);

                    // Log options
                    INFO(formatOptions(testCase.options));

                    // Run report generation
                    std::unique_ptr<TearsheetOutput> actualOutput;
                    try {
                        actualOutput = reportAdapter(inputDf, testCase.options);
                    } catch (const std::exception& e) {
                        FAIL("Report generation failed: " << e.what());
                        return;
                    }

                    // Compare with expected output
                    if (testCase.expect) {
                        INFO("Expected:\n" << testCase.expect->toString());
                        INFO("Actual:\n" << actualOutput->toString());

                        REQUIRE(actualOutput->equals(*testCase.expect));
                    } else {
                        REQUIRE(actualOutput == nullptr);
                    }
                }
            }
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
     * Helper to convert protobuf tearsheet to TearsheetOutput (if needed)
     * This can be implemented by the user based on their specific tearsheet format
     */
    template<typename TearSheetType>
    static std::unique_ptr<TearsheetOutput> convertTearsheetToOutput(TearSheetType* tearsheet) {
        if (!tearsheet) return nullptr;

        auto output = std::make_unique<TearsheetOutput>();

        // Convert cards
        for (int i = 0; i < tearsheet->cards_size(); ++i) {
            const auto& protoCard = tearsheet->cards(i);
            for (int j = 0; j < protoCard.data_size(); ++j) {
                const auto& cardData = protoCard.data(j);
                Card card;
                card.title = cardData.title();

                // Convert value based on type
                if (cardData.has_value_double()) {
                    card.value = Value(cardData.value_double());
                } else if (cardData.has_value_int()) {
                    card.value = Value(static_cast<double>(cardData.value_int()));
                } else if (cardData.has_value_string()) {
                    card.value = Value(cardData.value_string());
                }

                output->cards.push_back(card);
            }
        }

        // Convert charts
        for (int i = 0; i < tearsheet->charts_size(); ++i) {
            const auto& protoChart = tearsheet->charts(i);
            Chart chart;
            chart.type = protoChart.type();
            chart.title = protoChart.title();

            // Convert chart data
            for (int j = 0; j < protoChart.series_size(); ++j) {
                const auto& series = protoChart.series(j);
                ChartData data;
                data.name = series.name();

                // Convert values
                for (int k = 0; k < series.values_size(); ++k) {
                    data.values.push_back(series.values(k));
                }

                chart.data.push_back(data);
            }

            // Convert categories if present
            if (protoChart.has_x_axis()) {
                const auto& xAxis = protoChart.x_axis();
                for (int j = 0; j < xAxis.categories_size(); ++j) {
                    chart.categories.push_back(xAxis.categories(j));
                }
            }

            output->charts.push_back(chart);
        }

        // Convert tables
        for (int i = 0; i < tearsheet->tables_size(); ++i) {
            const auto& protoTable = tearsheet->tables(i);
            ReportTable table;
            table.title = protoTable.title();

            // Convert columns
            for (int j = 0; j < protoTable.columns_size(); ++j) {
                const auto& col = protoTable.columns(j);
                TableColumn column;
                column.name = col.name();
                column.type = col.type();
                table.columns.push_back(column);
            }

            // Convert rows
            for (int j = 0; j < protoTable.rows_size(); ++j) {
                const auto& protoRow = protoTable.rows(j);
                TableRow row;

                for (int k = 0; k < protoRow.values_size(); ++k) {
                    const auto& val = protoRow.values(k);

                    // Convert based on column type
                    if (k < table.columns.size()) {
                        const std::string& colType = table.columns[k].type;
                        if (colType == "integer" || colType == "percent" || colType == "double") {
                            row.values.push_back(Value(val.double_value()));
                        } else if (colType == "boolean") {
                            row.values.push_back(Value(val.bool_value()));
                        } else {
                            row.values.push_back(Value(val.string_value()));
                        }
                    } else {
                        row.values.push_back(Value(val.string_value()));
                    }
                }

                table.rows.push_back(row);
            }

            output->tables.push_back(table);
        }

        return output;
    }

};

} // namespace test
} // namespace epoch

#endif // YAML_REPORT_TESTER_HPP