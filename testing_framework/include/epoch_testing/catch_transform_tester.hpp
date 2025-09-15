#ifndef CATCH_TRANSFORM_TESTER_HPP
#define CATCH_TRANSFORM_TESTER_HPP

#include "epoch_testing/dataframe_tester.hpp"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include <epoch_core/catch_defs.h>
#include <filesystem>

namespace epoch {
namespace test {

// Adapter to convert between our test format and epoch_frame::DataFrame
class CatchTransformTester {
public:
    // Convert Table to epoch_frame::DataFrame
    static epoch_frame::DataFrame tableToDataFrame(const Table& table) {
        if (table.empty()) {
            return epoch_frame::DataFrame();
        }

        // Get the number of rows from the first column
        size_t numRows = 0;
        for (const auto& [_, column] : table) {
            numRows = column.size();
            break;
        }

        // Create index
        auto index = epoch_frame::factory::index::from_range(numRows);

        // Prepare columns and column names
        std::vector<std::vector<double>> columns;
        std::vector<std::string> columnNames;

        for (const auto& [colName, column] : table) {
            std::vector<double> doubleColumn;
            for (const auto& value : column) {
                if (std::holds_alternative<double>(value)) {
                    doubleColumn.push_back(std::get<double>(value));
                } else if (std::holds_alternative<bool>(value)) {
                    doubleColumn.push_back(std::get<bool>(value) ? 1.0 : 0.0);
                } else if (std::holds_alternative<std::string>(value)) {
                    // Try to parse string as double
                    try {
                        doubleColumn.push_back(std::stod(std::get<std::string>(value)));
                    } catch (...) {
                        doubleColumn.push_back(std::nan(""));
                    }
                } else {
                    doubleColumn.push_back(std::nan(""));
                }
            }
            columns.push_back(doubleColumn);
            columnNames.push_back(colName);
        }

        return epoch_frame::make_dataframe<double>(index, columns, columnNames);
    }

    // Convert epoch_frame::DataFrame to Table
    static Table dataFrameToTable(const epoch_frame::DataFrame& df) {
        Table table;

        for (auto const& colName: df.column_names()) {
            Column column;

            // Get the series for this column
            auto series = df[colName];

            // Convert series values to Column
            for (size_t i = 0; i < series.size(); ++i) {
                // Get scalar value at index i
                auto scalar = series.iloc(i);

                // For now, we assume all values are doubles (since transforms work with numeric data)
                // This simplification works for most transform tests
                if (scalar.is_null()) {
                    column.push_back(Value(std::numeric_limits<double>::quiet_NaN()));
                } else {
                    try {
                        column.push_back(Value(scalar.as_double()));
                    } catch (...) {
                        // If conversion fails, use NaN
                        column.push_back(Value(std::numeric_limits<double>::quiet_NaN()));
                    }
                }
            }

            table[colName] = column;
        }

        return table;
    }

    // Run tests from YAML file using Catch2
    static void runTestsWithCatch(
        const std::string& yamlFilePath,
        std::function<epoch_frame::DataFrame(const epoch_frame::DataFrame&, const Options&)> transformAdapter) {

        // Register DataFrame type
        static bool registered = false;
        if (!registered) {
            registerDataFrameType();
            registered = true;
        }

        // Load test cases
        std::vector<DataFrameTransformTester::TestCaseType> testCases;
        try {
            testCases = DataFrameTransformTester::loadTestsFromYAML(yamlFilePath);
        } catch (const std::exception& e) {
            FAIL("Failed to load test cases from " << yamlFilePath << ": " << e.what());
            return;
        }

        // Run each test case in a SECTION
        for (auto& testCase : testCases) {
            SECTION(testCase.title) {
                // Convert input Table to DataFrame
                epoch_frame::DataFrame inputDf = tableToDataFrame(testCase.input);

                // Log input for debugging
                INFO("Input DataFrame:\n" << inputDf);
                INFO("Options: " << optionsToString(testCase.options));

                // Run transform
                epoch_frame::DataFrame outputDf;
                try {
                    outputDf = transformAdapter(inputDf, testCase.options);
                } catch (const std::exception& e) {
                    FAIL("Transform threw exception: " << e.what());
                    return;
                }

                // Convert output DataFrame to Table
                Table outputTable = dataFrameToTable(outputDf);
                auto actualOutput = std::make_unique<DataFrameOutput>(outputTable);

                // Log output for debugging
                INFO("Output DataFrame:\n" << outputDf);
                INFO("Expected Output:\n" << (testCase.expect ? testCase.expect->toString() : "null"));
                INFO("Actual Output:\n" << actualOutput->toString());

                // Check results
                if (testCase.expect) {
                    CHECK(actualOutput->equals(*testCase.expect));
                } else {
                    CHECK(outputTable.empty());
                }
            }
        }
    }

    // Helper to find test case files in a directory
    static std::vector<std::string> findTestFiles(const std::string& directory) {
        std::vector<std::string> testFiles;

        if (std::filesystem::exists(directory)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                    testFiles.push_back(entry.path().string());
                }
            }
        }

        return testFiles;
    }

private:
    static std::string optionsToString(const Options& options) {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& [key, value] : options) {
            if (!first) ss << ", ";
            first = false;
            ss << key << ": ";
            if (std::holds_alternative<bool>(value)) {
                ss << (std::get<bool>(value) ? "true" : "false");
            } else if (std::holds_alternative<double>(value)) {
                ss << std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                ss << "\"" << std::get<std::string>(value) << "\"";
            }
        }
        ss << "}";
        return ss.str();
    }
};

} // namespace test
} // namespace epoch

#endif // CATCH_TRANSFORM_TESTER_HPP