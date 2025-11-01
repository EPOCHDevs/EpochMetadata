#include "flow_graph_tester.hpp"
#include <../../include/epochflow/transforms/core/config_helper.h>
#include <../../include/epochflow/transforms/core/transform_definition.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/scalar_factory.h>
#include <sstream>
#include <filesystem>

#include "google/protobuf/json/json.h"

namespace epoch_stratifyx {
namespace test {

using namespace epoch_core;
using namespace epoch_frame;
using namespace epochflow;

bool FlowGraphOutput::equals(const IOutputType& other) const {
    const auto* otherFlowGraph = dynamic_cast<const FlowGraphOutput*>(&other);
    if (!otherFlowGraph) {
        return false;
    }

    // Compare dataframes
    if (dataframes.size() != otherFlowGraph->dataframes.size()) {
        return false;
    }

    for (const auto& [timeframe, assetDataMap] : dataframes) {
        auto otherTfIt = otherFlowGraph->dataframes.find(timeframe);
        if (otherTfIt == otherFlowGraph->dataframes.end()) {
            return false;
        }

        if (assetDataMap.size() != otherTfIt->second.size()) {
            return false;
        }

        for (const auto& [asset, dataframe] : assetDataMap) {
            auto otherAssetIt = otherTfIt->second.find(asset);
            if (otherAssetIt == otherTfIt->second.end()) {
                return false;
            }

            if (!dataframe.equals(otherAssetIt->second)) {
                return false;
            }
        }
    }

    // Compare reports (basic comparison for now)
    if (reports.size() != otherFlowGraph->reports.size()) {
        return false;
    }

    for (const auto& [asset, report] : reports) {
        auto otherAssetIt = otherFlowGraph->reports.find(asset);
        if (otherAssetIt == otherFlowGraph->reports.end()) {
            return false;
        }

        // Basic report comparison - could be enhanced
        if (report.cards().cards_size() != otherAssetIt->second.cards().cards_size()) {
            return false;
        }
        if (report.charts().charts_size() != otherAssetIt->second.charts().charts_size()) {
            return false;
        }
        if (report.tables().tables_size() != otherAssetIt->second.tables().tables_size()) {
            return false;
        }
    }

    return true;
}

std::string FlowGraphOutput::toString() const {
    std::stringstream ss;
    ss << "FlowGraphOutput:\n";

    ss << "  Dataframes (" << dataframes.size() << " timeframes):\n";
    for (const auto& [timeframe, assetDataMap] : dataframes) {
        ss << "    " << timeframe << " (" << assetDataMap.size() << " assets):\n";
        for (const auto& [asset, dataframe] : assetDataMap) {
            ss << "      " << asset << ": \n" << dataframe << "\n";
        }
    }

    ss << "  Reports (" << reports.size() << " assets):\n";
    for (const auto& [asset, report] : reports) {
        ss << "    " << asset << ": \n";
        std::string jsonString;
        (void)google::protobuf::json::MessageToJsonString(report, &jsonString);
        ss << jsonString << "\n";
    }

    return ss.str();
}

std::unique_ptr<IOutputType> FlowGraphOutput::fromYAML(const YAML::Node& node) {
    auto output = std::make_unique<FlowGraphOutput>();

    // Load dataframes section
    if (node["dataframes"]) {
        const auto& dataframesNode = node["dataframes"];
        for (const auto& timeframeNode : dataframesNode) {
            std::string timeframe = timeframeNode.first.as<std::string>();
            const auto& assetsNode = timeframeNode.second;

            for (const auto& assetNode : assetsNode) {
                std::string assetSymbol = assetNode.first.as<std::string>();
                const auto& dataframeNode = assetNode.second;

                // Create asset using MakeAsset with proper format
                std::string asset = assetSymbol + "-Stock";

                // Load dataframe from YAML
                epoch_frame::DataFrame df = loadDataFrameFromYAML(dataframeNode);

                output->dataframes[timeframe][asset] = df;
            }
        }
    }

    // Load reports section (simplified for now)
    if (node["reports"]) {
        const auto& reportsNode = node["reports"];
        for (const auto& assetNode : reportsNode) {
            std::string assetSymbol = assetNode.first.as<std::string>();
            const auto& reportNode = assetNode.second;

            // Create asset using MakeAsset with proper format
            std::string asset = assetSymbol + "-Stock";

            // Create basic report (could be enhanced)
            epoch_proto::TearSheet report;

            if (reportNode["cards"]) {
                // Load cards
                for (const auto& cardNode : reportNode["cards"]) {
                    auto* card = report.mutable_cards()->add_cards();
                    if (cardNode["data"]) {
                        for (const auto& dataNode : cardNode["data"]) {
                            auto* data = card->add_data();
                            if (dataNode["title"]) {
                                data->set_title(dataNode["title"].as<std::string>());
                            }
                            if (dataNode["value"]) {
                                // Create a variant value (TearSheet cards use protobuf Any type)
                                // For now, just set title - value handling can be enhanced later
                                // data->set_value(dataNode["value"].as<double>());
                            }
                        }
                    }
                }
            }

            output->reports[asset] = report;
        }
    }

    return output;
}

TimeFrameAssetDataFrameMap loadDataFromYAML(const YAML::Node& node) {
    TimeFrameAssetDataFrameMap result;

    if (node["data_sources"] && node["data_sources"]["price_data"]) {
        // Load from external file
        std::string dataFile = node["data_sources"]["price_data"].as<std::string>();
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::string fullPath = (currentPath / "flow_graph_test_cases" / dataFile).string();

        YAML::Node dataNode = YAML::LoadFile(fullPath);
        return loadDataFromYAML(dataNode);
    }

    // Load inline data
    for (const auto& timeframeNode : node) {
        std::string timeframe = timeframeNode.first.as<std::string>();
        const auto& assetsNode = timeframeNode.second;

        for (const auto& assetNode : assetsNode) {
            std::string assetSymbol = assetNode.first.as<std::string>();
            const auto& dataNode = assetNode.second;

            // Create asset using MakeAsset with proper format
            std::string asset = assetSymbol + "-Stock";

            // Load dataframe
            epoch_frame::DataFrame df = loadDataFrameFromYAML(dataNode);

            result[timeframe][asset] = df;
        }
    }

    return result;
}

std::vector<std::string> loadAssetsFromYAML(const YAML::Node& node) {
    std::vector<std::string> assets;

    for (const auto& assetNode : node) {
        std::string assetSymbol = assetNode.as<std::string>();
        std::string asset = assetSymbol + "-Stock";
        assets.push_back(asset);
    }

    return assets;
}

epochflow::transform::TransformConfigurationList loadConfigurationFromYAML(const YAML::Node& node) {
    epochflow::transform::TransformConfigurationList configurations;

    if (!node["transforms"]) {
        return configurations;
    }

    for (const auto& transformNode : node["transforms"]) {
        // Create TransformConfiguration from YAML node using TransformDefinition
        epochflow::TransformDefinition definition(transformNode);
        epochflow::transform::TransformConfiguration config(std::move(definition));
        configurations.push_back(config);
    }

    return configurations;
}

epoch_frame::DataFrame loadDataFrameFromYAML(const YAML::Node& node) {
    // Handle different data formats
    if (node["columns"] && node["values"]) {
        // Standard format with columns and values
        std::vector<std::string> columns;
        for (const auto& col : node["columns"]) {
            columns.push_back(col.as<std::string>());
        }

        // Convert YAML values to Scalar data, preserving original types
        // Note: Our YAML format has column-major data (each array is a column)
        std::vector<std::vector<epoch_frame::Scalar>> columnData;
        for (const auto& column : node["values"]) {
            std::vector<epoch_frame::Scalar> columnValues;
            for (const auto& value : column) {
                if (value.IsNull()) {
                    columnValues.push_back(epoch_frame::Scalar{std::numeric_limits<double>::quiet_NaN()});
                } else {
                    // Handle different YAML types by preserving their native type
                    try {
                        // Try as boolean first to preserve boolean semantics
                        if (value.IsScalar() && (value.as<std::string>() == "true" || value.as<std::string>() == "false")) {
                            columnValues.push_back(epoch_frame::Scalar{value.as<bool>()});
                        } else {
                            // Try as double
                            columnValues.push_back(epoch_frame::Scalar{value.as<double>()});
                        }
                    } catch (const YAML::BadConversion& e) {
                        // Fallback: try as boolean if double conversion fails
                        try {
                            columnValues.push_back(epoch_frame::Scalar{value.as<bool>()});
                        } catch (const YAML::BadConversion& e2) {
                            // If all else fails, set to NaN
                            columnValues.push_back(epoch_frame::Scalar{std::numeric_limits<double>::quiet_NaN()});
                        }
                    }
                }
            }
            columnData.push_back(columnValues);
        }

        // Create index (assume datetime for now)
        epoch_frame::IndexPtr index;
        if (node["timestamps"]) {
            std::vector<epoch_frame::DateTime> timestamps;
            for (const auto& ts : node["timestamps"]) {
                timestamps.push_back(epoch_frame::DateTime::from_str(ts.as<std::string>(), "UTC", "%Y-%m-%dT%H:%M:%S"));
            }
            index = epoch_frame::factory::index::make_datetime_index(timestamps);
        } else {
            // Create simple range index with timestamps for each row
            // Use the number of values in the first column to determine number of rows
            size_t numRows = columnData.empty() ? 0 : columnData[0].size();
            std::vector<epoch_frame::DateTime> defaultTimestamps;
            for (size_t i = 0; i < numRows; ++i) {
                defaultTimestamps.push_back(
                    epoch_frame::DateTime::from_str("2020-01-01T00:00:00", "UTC", "%Y-%m-%dT%H:%M:%S") +
                    std::chrono::days(i)
                );
            }
            index = epoch_frame::factory::index::make_datetime_index(defaultTimestamps);
        }

        // Data is already in column-major format, no need to transpose

        // Create Arrow fields for Scalar columns
        arrow::FieldVector fields;
        for (size_t i = 0; i < columns.size() && i < columnData.size(); ++i) {
            // Use the type of the first non-null value in the column to determine field type
            auto fieldType = arrow::null();
            for (const auto& scalar : columnData[i]) {
                if (!scalar.is_null()) {
                    fieldType = scalar.type();
                    break;
                }
            }
            fields.push_back(arrow::field(columns[i], fieldType));
        }

        return epoch_frame::make_dataframe(index, columnData, fields);
    }

    // Direct column format (like existing tests)
    std::vector<std::string> columns;
    std::vector<std::vector<epoch_frame::Scalar>> columnData;

    for (const auto& columnNode : node) {
        std::string columnName = columnNode.first.as<std::string>();
        columns.push_back(columnName);

        // Convert YAML values to Scalar data, preserving types
        std::vector<epoch_frame::Scalar> data;
        for (const auto& value : columnNode.second) {
            if (value.IsNull()) {
                data.push_back(epoch_frame::Scalar{std::numeric_limits<double>::quiet_NaN()});
            } else {
                // Handle different YAML types by preserving their native type
                try {
                    // Try as boolean first to preserve boolean semantics
                    if (value.IsScalar() && (value.as<std::string>() == "true" || value.as<std::string>() == "false")) {
                        data.push_back(epoch_frame::Scalar{value.as<bool>()});
                    } else {
                        // Try as double
                        data.push_back(epoch_frame::Scalar{value.as<double>()});
                    }
                } catch (const YAML::BadConversion& e) {
                    // Fallback: try as boolean if double conversion fails
                    try {
                        data.push_back(epoch_frame::Scalar{value.as<bool>()});
                    } catch (const YAML::BadConversion& e2) {
                        // If all else fails, set to NaN
                        data.push_back(epoch_frame::Scalar{std::numeric_limits<double>::quiet_NaN()});
                    }
                }
            }
        }
        columnData.push_back(data);
    }

    // Create index
    size_t numRows = columnData.empty() ? 0 : columnData[0].size();
    std::vector<epoch_frame::DateTime> defaultTimestamps;
    for (size_t i = 0; i < numRows; ++i) {
        defaultTimestamps.push_back(
            epoch_frame::DateTime::from_str("2020-01-01T00:00:00", "UTC", "%Y-%m-%dT%H:%M:%S") +
            std::chrono::days(i)
        );
    }
    auto index = epoch_frame::factory::index::make_datetime_index(defaultTimestamps);

    // Create Arrow fields for Scalar columns
    arrow::FieldVector fields;
    for (size_t i = 0; i < columns.size() && i < columnData.size(); ++i) {
        // Use the type of the first non-null value in the column to determine field type
        auto fieldType = arrow::null();
        for (const auto& scalar : columnData[i]) {
            if (!scalar.is_null()) {
                fieldType = scalar.type();
                break;
            }
        }
        fields.push_back(arrow::field(columns[i], fieldType));
    }

    return epoch_frame::make_dataframe(index, columnData, fields);
}

std::vector<FlowGraphTestCase> loadFlowGraphTestsFromYAML(const std::string& filePath) {
    std::vector<FlowGraphTestCase> testCases;

    YAML::Node root = YAML::LoadFile(filePath);

    if (!root["tests"]) {
        throw std::runtime_error("No 'tests' section found in YAML file");
    }

    // Load shared data sources if present
    TimeFrameAssetDataFrameMap sharedData;
    if (root["data_sources"]) {
        sharedData = loadDataFromYAML(root);
    }

    for (const auto& testNode : root["tests"]) {
        FlowGraphTestCase testCase;

        // Load basic info
        testCase.title = testNode["title"].as<std::string>();

        // Load assets
        if (testNode["assets"]) {
            testCase.assets = loadAssetsFromYAML(testNode["assets"]);
        }

        // Load timeframes
        if (testNode["timeframes"]) {
            for (const auto& tf : testNode["timeframes"]) {
                testCase.timeframes.push_back(tf.as<std::string>());
            }
        }

        // Load input data (use shared data or inline data)
        if (testNode["input"]) {
            testCase.inputData = loadDataFromYAML(testNode["input"]);
        } else if (!sharedData.empty()) {
            testCase.inputData = sharedData;
        }

        // Filter input data to only include timeframes used in this test case
        TimeFrameAssetDataFrameMap filteredInputData;
        for (const auto& timeframe : testCase.timeframes) {
            if (testCase.inputData.find(timeframe) != testCase.inputData.end()) {
                filteredInputData[timeframe] = testCase.inputData[timeframe];
            }
        }
        testCase.inputData = filteredInputData;

        // Load configuration
        if (testNode["configuration"]) {
            testCase.configuration = loadConfigurationFromYAML(testNode["configuration"]);
        }

        // Load expected output
        if (testNode["expect"]) {
            testCase.expect = FlowGraphOutput::fromYAML(testNode["expect"]);
        }

        testCases.push_back(std::move(testCase));
    }

    return testCases;
}

} // namespace test
} // namespace epoch_stratifyx