#include "dataframe_tester.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace epoch {
namespace test {

// Helper to convert Value to string (reuse from base)
static std::string valueToString(const Value& v) {
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (std::isnan(d)) {
            return "nan";
        }
        std::stringstream ss;
        ss << std::setprecision(10) << d;
        return ss.str();
    } else if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else {
        return "null";
    }
}

bool DataFrameOutput::equals(const IOutputType& other) const {
    if (other.getType() != "dataframe") {
        return false;
    }

    const auto& otherDf = static_cast<const DataFrameOutput&>(other);

    // Check if same number of columns
    if (data.size() != otherDf.data.size()) {
        return false;
    }

    // Compare each column
    for (const auto& [colName, column] : data) {
        auto otherIt = otherDf.data.find(colName);
        if (otherIt == otherDf.data.end()) {
            return false;
        }

        const Column& otherCol = otherIt->second;
        if (column.size() != otherCol.size()) {
            return false;
        }

        // Compare each value in the column
        for (size_t i = 0; i < column.size(); ++i) {
            if (column[i].index() != otherCol[i].index()) {
                return false;
            }

            if (std::holds_alternative<double>(column[i])) {
                double v1 = std::get<double>(column[i]);
                double v2 = std::get<double>(otherCol[i]);

                // Handle NaN comparison
                if (std::isnan(v1) && std::isnan(v2)) {
                    continue;
                }

                // Floating point comparison with tolerance
                // Use relative tolerance for numerical stability
                double tolerance = 1e-5;  // 0.001% relative tolerance
                double diff = std::abs(v1 - v2);

                // For values close to zero, use absolute tolerance
                if (std::abs(v1) < 1.0 && std::abs(v2) < 1.0) {
                    if (diff > tolerance) {
                        return false;
                    }
                } else {
                    // For larger values, use relative tolerance
                    double maxVal = std::max(std::abs(v1), std::abs(v2));
                    if (diff > maxVal * tolerance) {
                        return false;
                    }
                }
            } else if (column[i] != otherCol[i]) {
                return false;
            }
        }
    }

    return true;
}

std::string DataFrameOutput::toString() const {
    std::stringstream ss;
    ss << "DataFrame {\n";

    for (const auto& [colName, column] : data) {
        ss << "  " << colName << ": [";
        for (size_t i = 0; i < column.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << valueToString(column[i]);
        }
        ss << "]\n";
    }

    ss << "}";
    return ss.str();
}

std::unique_ptr<IOutputType> DataFrameOutput::fromYAML(const YAML::Node& node) {
    auto output = std::make_unique<DataFrameOutput>();

    // Check if the node has explicit type field
    if (node["type"] && node["type"].as<std::string>() == "dataframe") {
        // Parse from data field
        if (node["data"] && node["data"].IsMap()) {
            for (const auto& pair : node["data"]) {
                std::string columnName = pair.first.as<std::string>();
                Column column;

                if (pair.second.IsSequence()) {
                    for (const auto& item : pair.second) {
                        // Parse each value
                        if (!item.IsDefined() || item.IsNull()) {
                            column.push_back(std::monostate{});
                        } else if (item.Type() == YAML::NodeType::Scalar) {
                            const std::string str = item.as<std::string>();

                            if (str == "nan" || str == "NaN" || str == "NAN") {
                                column.push_back(std::numeric_limits<double>::quiet_NaN());
                            } else if (str == "true" || str == "false") {
                                column.push_back(item.as<bool>());
                            } else {
                                try {
                                    size_t pos;
                                    double d = std::stod(str, &pos);
                                    if (pos == str.length()) {
                                        column.push_back(d);
                                    } else {
                                        column.push_back(str);
                                    }
                                } catch (...) {
                                    column.push_back(str);
                                }
                            }
                        }
                    }
                }

                output->data[columnName] = column;
            }
        }
    } else {
        // If no type field, assume the whole node is the dataframe data
        if (node.IsMap()) {
            for (const auto& pair : node) {
                std::string columnName = pair.first.as<std::string>();
                Column column;

                if (pair.second.IsSequence()) {
                    for (const auto& item : pair.second) {
                        // Parse each value
                        if (!item.IsDefined() || item.IsNull()) {
                            column.push_back(std::monostate{});
                        } else if (item.Type() == YAML::NodeType::Scalar) {
                            const std::string str = item.as<std::string>();

                            if (str == "nan" || str == "NaN" || str == "NAN") {
                                column.push_back(std::numeric_limits<double>::quiet_NaN());
                            } else if (str == "true" || str == "false") {
                                column.push_back(item.as<bool>());
                            } else {
                                try {
                                    size_t pos;
                                    double d = std::stod(str, &pos);
                                    if (pos == str.length()) {
                                        column.push_back(d);
                                    } else {
                                        column.push_back(str);
                                    }
                                } catch (...) {
                                    column.push_back(str);
                                }
                            }
                        }
                    }
                }

                output->data[columnName] = column;
            }
        }
    }

    return output;
}

} // namespace test
} // namespace epoch