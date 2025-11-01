#ifndef CATCH_TRANSFORM_TESTER_HPP
#define CATCH_TRANSFORM_TESTER_HPP

#include "common/dataframe_tester.hpp"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/datetime.h>
#include <epoch_frame/index.h>
#include <filesystem>

namespace epoch {
namespace test {

// Adapter to convert between our test format and epoch_frame::DataFrame
class CatchTransformTester {
public:
    // Convert Table to epoch_frame::DataFrame with optional timestamp columns and custom index
    static epoch_frame::DataFrame tableToDataFrame(const Table& table,
                                                    const std::vector<std::string>& timestamp_columns = {},
                                                    const std::string& index_column = "") {
        if (table.empty()) {
            return epoch_frame::DataFrame();
        }

        // Get the number of rows from the first column
        size_t numRows = 0;
        for (const auto& [_, column] : table) {
            numRows = column.size();
            break;
        }

        // Create index - either from specified column or default range
        epoch_frame::IndexPtr index;
        if (!index_column.empty() && table.find(index_column) != table.end()) {
            // Use specified column as index
            auto indexCol = table.at(index_column);
            // Check if this is a timestamp column
            if (std::find(timestamp_columns.begin(), timestamp_columns.end(), index_column) != timestamp_columns.end()) {
                // Create timestamp index
                std::vector<int64_t> timestamps;
                for (const auto& value_opt : indexCol) {
                    if (value_opt && std::holds_alternative<std::string>(value_opt.value())) {
                        // Parse datetime string to nanoseconds using epoch_core::DateTime
                        auto str = std::get<std::string>(value_opt.value());
                        auto dt = epoch_frame::DateTime::from_str(str, "UTC", "%Y-%m-%dT%H:%M:%S");
                        timestamps.emplace_back(dt.m_nanoseconds.count());
                    } else if (value_opt && std::holds_alternative<double>(value_opt.value())) {
                        // Already in nanoseconds
                        timestamps.emplace_back(static_cast<int64_t>(std::get<double>(value_opt.value())));
                    } else {
                        timestamps.push_back(0);  // null
                    }
                }
                index = epoch_frame::factory::index::make_datetime_index(timestamps);
            } else {
                // Create numeric index from column
                index = epoch_frame::factory::index::from_range(numRows);
            }
        } else {
            // Default range index
            index = epoch_frame::factory::index::from_range(numRows);
        }

        // For now, keep the simple approach of converting everything to double for input
        // The type preservation will be handled by the transform itself
        std::vector<std::vector<epoch_frame::Scalar>> columns;
        arrow::FieldVector fields;

        for (const auto& [colName, column] : table) {
            // Skip the index column if it's being used as index
            if (!index_column.empty() && colName == index_column) {
                continue;
            }

            std::vector<epoch_frame::Scalar> columnArray;
            arrow::DataTypePtr type = arrow::null();

            // Check if this column should be converted to timestamps
            bool isTimestampCol = std::find(timestamp_columns.begin(), timestamp_columns.end(), colName) != timestamp_columns.end();

            for (const auto& value_opt : column) {
                if (!value_opt) {
                    columnArray.push_back(epoch_frame::Scalar{});
                    continue;
                }
                auto value = value_opt.value();

                if (isTimestampCol) {
                    // Convert to timestamp (timezone-naive)
                    if (std::holds_alternative<std::string>(value)) {
                        auto str = std::get<std::string>(value);
                        // Parse as UTC but create timezone-naive timestamp
                        auto dt = epoch_frame::DateTime::from_str(str, "UTC", "%Y-%m-%dT%H:%M:%S");
                        int64_t ts = dt.m_nanoseconds.count();
                        // Create timezone-naive DateTime from nanoseconds
                        columnArray.emplace_back(epoch_frame::DateTime(ts));
                    } else if (std::holds_alternative<double>(value)) {
                        // Already in nanoseconds
                        int64_t ts = static_cast<int64_t>(std::get<double>(value));
                        columnArray.emplace_back(epoch_frame::DateTime(ts));
                    } else {
                        columnArray.push_back(epoch_frame::Scalar{});
                    }
                    type = arrow::timestamp(arrow::TimeUnit::NANO);
                } else if (std::holds_alternative<double>(value)) {
                    type = arrow::float64();
                    columnArray.push_back(epoch_frame::Scalar(std::get<double>(value)));
                } else if (std::holds_alternative<bool>(value)) {
                    columnArray.push_back(epoch_frame::Scalar(std::get<bool>(value)));
                    type = arrow::boolean();
                } else {
                    columnArray.push_back(epoch_frame::Scalar(std::get<std::string>(value)));
                    type = arrow::utf8();
                }
            }
            columns.push_back(columnArray);
            fields.push_back(arrow::field(colName, type));
        }

        return epoch_frame::make_dataframe(index, columns, fields);
    }

    // Convert epoch_frame::DataFrame to Table
    static Table dataFrameToTable(const epoch_frame::DataFrame& df) {
        Table table;

        // Reset index to include it as a column for comparison
        // This ensures the index (e.g., timestamp) is validated in tests
        // Check if index has a name (non-range indices typically have names)
        epoch_frame::DataFrame df_with_index = df;
        if (!df.index()->name().empty()) {
            df_with_index = df.reset_index();
        }

        for (auto const& colName: df_with_index.column_names()) {
            Column column;

            // Get the series for this column
            auto series = df_with_index[colName];

            // Convert series values to Column
            for (size_t i = 0; i < series.size(); ++i) {
                // Get scalar value at index i
                auto scalar = series.iloc(i);

                // Handle null values properly
                if (scalar.is_null()) {
                    // Use NaN for null values in numeric columns
                    column.push_back(std::nullopt);
                } else if (arrow::is_numeric(scalar.type()->id())) {
                    column.push_back(Value(scalar.cast_double().as_double()));
                } else if (scalar.type()->id() == arrow::Type::BOOL) {
                    column.push_back(Value(scalar.as_bool()));
                } else if (arrow::is_temporal(scalar.type()->id())) {
                    // For temporal types, use the string representation
                    column.push_back(Value(scalar.repr()));
                } else {
                    column.push_back(Value(scalar.repr()));
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
                // Convert input Table to DataFrame with timestamp and index support
                epoch_frame::DataFrame inputDf = tableToDataFrame(testCase.input,
                                                                   testCase.timestamp_columns,
                                                                   testCase.index_column);

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