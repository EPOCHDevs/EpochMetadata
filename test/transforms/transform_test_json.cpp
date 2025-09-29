//
// JSON-based transform tester using glaze
//
#include <epoch_testing/json_transform_tester.hpp>
#include <epoch_testing/catch_transform_tester.hpp>
#include <epoch_testing/tearsheet_output.hpp>
#include <epoch_testing/dataframe_tester.hpp>
#include <epoch_metadata/transforms/itransform.h>
#include <epoch_metadata/transforms/transform_registry.h>
#include <epoch_metadata/transforms/transform_definition.h>
#include <epoch_metadata/transforms/transform_configuration.h>
#include <epoch_metadata/reports/ireport.h>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <algorithm>

using namespace epoch::test;
using namespace epoch::test::json;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;

namespace {

// Build TransformDefinition from test options
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

    // Create TransformDefinition from YAML
    return TransformDefinition(yamlNode);
}

// Run transform with configuration
epoch_frame::DataFrame runTransformWithConfig(const epoch_frame::DataFrame& input,
                                             const Options& options) {
    // Build TransformDefinition from options
    TransformDefinition definition = buildTransformDefinition(options, input);

    // Create TransformConfiguration from definition
    TransformConfiguration config(std::move(definition));

    // Create transform using registry
    auto transformPtr = TransformRegistry::GetInstance().Get(config);
    if (!transformPtr) {
        throw std::runtime_error("Failed to create transform");
    }

    // Cast to ITransform
    auto transform = dynamic_cast<ITransform*>(transformPtr.get());
    if (!transform) {
        throw std::runtime_error("Transform does not implement ITransform interface");
    }

    // Run the transform
    return transform->TransformData(input);
}

// Convert JSON test case to DataFrame tester format
DataFrameTransformTester::TestCaseType convertJsonToTestCase(const json::TestCase& jsonTest) {
    DataFrameTransformTester::TestCaseType testCase;

    testCase.title = jsonTest.title;
    testCase.timestamp_columns = jsonTest.timestamp_columns;
    testCase.index_column = jsonTest.index_column.value_or("");

    // Convert input data from ColumnData to Table format
    for (const auto& [colName, colData] : jsonTest.input) {
        Column column;
        for (const auto& value : colData) {
            if (std::holds_alternative<double>(value)) {
                column.push_back(std::get<double>(value));
            } else if (std::holds_alternative<int64_t>(value)) {
                column.push_back(static_cast<double>(std::get<int64_t>(value)));
            } else if (std::holds_alternative<bool>(value)) {
                column.push_back(std::get<bool>(value));
            } else if (std::holds_alternative<std::string>(value)) {
                column.push_back(std::get<std::string>(value));
            } else {
                column.push_back(std::nullopt);
            }
        }
        testCase.input[colName] = column;
    }

    // Convert options
    for (const auto& [key, value] : jsonTest.options) {
        if (std::holds_alternative<bool>(value)) {
            testCase.options[key] = std::get<bool>(value);
        } else if (std::holds_alternative<double>(value)) {
            testCase.options[key] = std::get<double>(value);
        } else if (std::holds_alternative<int64_t>(value)) {
            testCase.options[key] = static_cast<double>(std::get<int64_t>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            testCase.options[key] = std::get<std::string>(value);
        }
    }

    // Handle expected output
    if (jsonTest.expect.has_value()) {
        const auto& expectVar = jsonTest.expect.value();

        if (std::holds_alternative<TearsheetExpect>(expectVar)) {
            // Create tearsheet output
            auto tearsheet = std::make_unique<TearsheetOutput>();

            const auto& tearsheetExpect = std::get<TearsheetExpect>(expectVar);

            // Handle cards if present
            if (tearsheetExpect.cards.has_value()) {
                const auto& cardsList = tearsheetExpect.cards.value();
                epoch_proto::CardDefList* protoCardsList = tearsheet->protoTearsheet.mutable_cards();

                for (const auto& card : cardsList.cards) {
                    epoch_proto::CardDef* protoCard = protoCardsList->add_cards();
                    protoCard->set_category(card.category);
                    protoCard->set_group_size(card.group_size);

                    // Map card type string to proto type
                    if (card.type == "WidgetCard") {
                        protoCard->set_type(epoch_proto::WidgetCard);
                    }

                    // Add card data
                    for (const auto& data : card.data) {
                        epoch_proto::CardData* protoData = protoCard->add_data();
                        protoData->set_title(data.title);
                        protoData->set_group(data.group);

                        // Map data type string to proto type
                        if (data.type == "TypeDecimal") {
                            protoData->set_type(epoch_proto::TypeDecimal);
                        } else if (data.type == "TypeInteger") {
                            protoData->set_type(epoch_proto::TypeInteger);
                        } else if (data.type == "TypeString") {
                            protoData->set_type(epoch_proto::TypeString);
                        } else if (data.type == "TypeBoolean") {
                            protoData->set_type(epoch_proto::TypeBoolean);
                        } else if (data.type == "TypePercent") {
                            protoData->set_type(epoch_proto::TypePercent);
                        } else if (data.type == "TypeMonetary") {
                            protoData->set_type(epoch_proto::TypeMonetary);
                        } else if (data.type == "TypeDate") {
                            protoData->set_type(epoch_proto::TypeDate);
                        }

                        // Set value using correct scalar variant based on type
                        epoch_proto::Scalar* protoValue = protoData->mutable_value();
                        if (std::holds_alternative<double>(data.value)) {
                            double val = std::get<double>(data.value);
                            // Use the correct variant based on the type field
                            if (data.type == "TypePercent") {
                                protoValue->set_percent_value(val);
                            } else if (data.type == "TypeMonetary") {
                                protoValue->set_monetary_value(val);
                            } else {
                                protoValue->set_decimal_value(val);
                            }
                        } else if (std::holds_alternative<int64_t>(data.value)) {
                            int64_t val = std::get<int64_t>(data.value);
                            if (data.type == "TypeDate") {
                                protoValue->set_date_value(val);
                            } else {
                                protoValue->set_integer_value(val);
                            }
                        } else if (std::holds_alternative<bool>(data.value)) {
                            protoValue->set_boolean_value(std::get<bool>(data.value));
                        } else if (std::holds_alternative<std::string>(data.value)) {
                            protoValue->set_string_value(std::get<std::string>(data.value));
                        }
                    }
                }
            }

            // Create tables in the tearsheet if any
            if (!tearsheetExpect.tables.empty()) {
                epoch_proto::TableList* tableList = tearsheet->protoTearsheet.mutable_tables();
                for (const auto& table : tearsheetExpect.tables) {
                // Add table to TableList
                epoch_proto::Table* protoTable = tableList->add_tables();
                protoTable->set_title(table.title);
                protoTable->set_category(table.category);
                protoTable->set_type(epoch_proto::WidgetDataTable);

                // Add columns
                for (const auto& col : table.columns) {
                    auto* protoCol = protoTable->add_columns();
                    protoCol->set_name(col.name);
                    // Map type string to proto type
                    if (col.type == "TypeDecimal") {
                        protoCol->set_type(epoch_proto::TypeDecimal);
                    } else if (col.type == "TypeInteger") {
                        protoCol->set_type(epoch_proto::TypeInteger);
                    } else if (col.type == "TypeString") {
                        protoCol->set_type(epoch_proto::TypeString);
                    } else if (col.type == "TypePercent") {
                        protoCol->set_type(epoch_proto::TypePercent);
                    } else if (col.type == "TypeMonetary") {
                        protoCol->set_type(epoch_proto::TypeMonetary);
                    } else if (col.type == "TypeDate") {
                        protoCol->set_type(epoch_proto::TypeDate);
                    } else if (col.type == "TypeBoolean") {
                        protoCol->set_type(epoch_proto::TypeBoolean);
                    }
                }

                // Get mutable TableData
                epoch_proto::TableData* tableData = protoTable->mutable_data();

                // Add rows
                for (const auto& row : table.data.rows) {
                    epoch_proto::TableRow* protoRow = tableData->add_rows();
                    for (size_t colIdx = 0; colIdx < row.size(); ++colIdx) {
                        const auto& cell = row[colIdx];
                        auto* scalar = protoRow->add_values();

                        // Use the column type to determine which scalar variant to use
                        std::string colType = (colIdx < table.columns.size()) ? table.columns[colIdx].type : "TypeDecimal";

                        if (std::holds_alternative<double>(cell)) {
                            double val = std::get<double>(cell);
                            if (colType == "TypePercent") {
                                scalar->set_percent_value(val);
                            } else if (colType == "TypeMonetary") {
                                scalar->set_monetary_value(val);
                            } else {
                                scalar->set_decimal_value(val);
                            }
                        } else if (std::holds_alternative<int64_t>(cell)) {
                            int64_t val = std::get<int64_t>(cell);
                            if (colType == "TypeDate") {
                                scalar->set_date_value(val);
                            } else {
                                scalar->set_integer_value(val);
                            }
                        } else if (std::holds_alternative<std::string>(cell)) {
                            scalar->set_string_value(std::get<std::string>(cell));
                        } else if (std::holds_alternative<std::nullptr_t>(cell)) {
                            scalar->set_null_value(epoch_proto::NULL_VALUE);
                        }
                    }
                }
            }
            }

            // Charts will be compared properly if specified in test expectations

            testCase.expect = std::move(tearsheet);

        } else if (std::holds_alternative<DataFrameExpect>(expectVar)) {
            // Create dataframe output
            const auto& dfExpect = std::get<DataFrameExpect>(expectVar);
            Table outputTable;

            for (const auto& [colName, colData] : dfExpect.columns) {
                Column column;
                for (const auto& value : colData) {
                    if (std::holds_alternative<double>(value)) {
                        column.push_back(std::get<double>(value));
                    } else if (std::holds_alternative<int64_t>(value)) {
                        column.push_back(static_cast<double>(std::get<int64_t>(value)));
                    } else if (std::holds_alternative<bool>(value)) {
                        column.push_back(std::get<bool>(value));
                    } else if (std::holds_alternative<std::string>(value)) {
                        column.push_back(std::get<std::string>(value));
                    } else {
                        column.push_back(std::nullopt);
                    }
                }
                outputTable[colName] = column;
            }

            testCase.expect = std::make_unique<DataFrameOutput>(outputTable);
        }
    }

    return testCase;
}

// Run tests from a JSON file
void runJsonTestFile(const std::string& testFile) {
    std::filesystem::path filePath(testFile);
    std::string sectionName = filePath.stem().string() + " [JSON]";

    SECTION(sectionName) {
        INFO("Loading JSON test file: " << testFile);

        std::vector<json::TestCase> jsonTests;
        try {
            // Use dynamic loader for better compatibility
            jsonTests = JsonTransformTester::loadTestsFromJSONDynamic(testFile);
        } catch (const std::exception& e) {
            FAIL("Failed to load JSON tests from " << testFile << ": " << e.what());
            return;
        }

        INFO("Loaded " << jsonTests.size() << " test cases");

        for (const auto& jsonTest : jsonTests) {
            SECTION(jsonTest.title) {
                // Convert to standard test case format
                auto testCase = convertJsonToTestCase(jsonTest);

                // Keep inputs SLOT as-is - individual nodes handle sanitization if needed

                // Convert input to DataFrame with normalized column names
                epoch_frame::DataFrame inputDf = CatchTransformTester::tableToDataFrame(
                    testCase.input, testCase.timestamp_columns, testCase.index_column);

                INFO("Test: " << testCase.title);
                INFO("Input DataFrame:\n" << inputDf);

                // Run the test using existing infrastructure
                bool isReportTest = testCase.expect && testCase.expect->getType() == "tearsheet";

                if (isReportTest) {
                    // Report test - run directly
                    INFO("Running report test");

                    std::unique_ptr<TearsheetOutput> actualOutput;
                    try {
                        // Create transform instance
                        auto definition = buildTransformDefinition(testCase.options, inputDf);
                        TransformConfiguration config(std::move(definition));
                        auto transformPtr = TransformRegistry::GetInstance().Get(config);

                        if (!transformPtr) {
                            FAIL("Failed to create transform");
                            return;
                        }

                        // Cast to reporter
                        auto reporter = dynamic_cast<epoch_metadata::reports::IReporter*>(transformPtr.get());
                        if (!reporter) {
                            FAIL("Transform does not implement IReporter interface");
                            return;
                        }

                        // Run the report
                        auto outputDf = reporter->TransformData(inputDf);

                        // Get tearsheet
                        epoch_proto::TearSheet protoTearsheet = reporter->GetTearSheet();

                        actualOutput = std::make_unique<TearsheetOutput>();
                        actualOutput->protoTearsheet = protoTearsheet;

                    } catch (const std::exception& e) {
                        FAIL("Report generation failed: " + std::string(e.what()));
                    }

                    // Compare outputs
                    if (testCase.expect) {
                        INFO("Expected:\n" << testCase.expect->toString());
                        INFO("Actual:\n" << actualOutput->toString());
                        REQUIRE(actualOutput->equals(*testCase.expect));
                    }

                } else {
                    // Transform test
                    INFO("Running transform test");

                    epoch_frame::DataFrame outputDf;
                    try {
                        outputDf = runTransformWithConfig(inputDf, testCase.options);
                    } catch (const std::exception& e) {
                        FAIL("Transform failed: " << e.what());
                        return;
                    }

                    INFO("Output DataFrame:\n" << outputDf);

                    // Convert output for comparison
                    Table outputTable = CatchTransformTester::dataFrameToTable(outputDf);
                    auto actualOutput = std::make_unique<DataFrameOutput>(outputTable);

                    // Compare
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

} // anonymous namespace

TEST_CASE("All Transform Tests - JSON Based", "[Transform][JSON]") {
    // Register output types
    static bool registered = false;
    if (!registered) {
        registerDataFrameType();
        registerTearsheetType();
        registered = true;
    }

    // Find all JSON test files
    std::vector<std::string> jsonFiles;
    std::filesystem::path testDir("transforms_test_cases");

    if (std::filesystem::exists(testDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(testDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path().string());
            }
        }
    }

    if (jsonFiles.empty()) {
        WARN("No JSON test files found");
        return;
    }

    // Sort for consistent ordering
    std::sort(jsonFiles.begin(), jsonFiles.end());

    INFO("Found " << jsonFiles.size() << " JSON test files");

    // Run each test file
    for (const auto& testFile : jsonFiles) {
        runJsonTestFile(testFile);
    }
}