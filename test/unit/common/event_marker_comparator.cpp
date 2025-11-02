#include "event_marker_comparator.h"

#include <fstream>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

#include "epoch_frame/serialization.h"

namespace epoch_script::runtime::test {

std::string SelectorComparator::ToJson(const EventMarkerData& selector, bool prettyPrint) {
    glz::generic json;

    // Build JSON object
    json["title"] = selector.title;
    json["icon"] = selector.icon;  // icon is already a string

    // Serialize schemas
    auto schemasResult = glz::write_json(selector.schemas);
    if (schemasResult) {
        auto schemasJson = glz::read_json<glz::generic>(schemasResult.value());
        if (schemasJson) {
            json["schemas"] = schemasJson.value();
        }
    }

    // Add pivot_index
    if (selector.pivot_index.has_value()) {
        json["pivot_index"] = selector.pivot_index.value();
    } else {
        json["pivot_index"] = nullptr;
    }

    // Convert DataFrame to JSON
    if (selector.data.num_rows() > 0) {
        glz::generic dataJson;

        // Get column names
        auto columns = selector.data.column_names();
        dataJson["columns"] = columns;

        // Get rows (convert to row-major format)
        glz::generic::array_t rows;
        for (size_t i = 0; i < selector.data.num_rows(); ++i) {
            glz::generic::array_t row;
            for (const auto& col : columns) {
                auto value = selector.data[col].iloc(i);
                // Convert epoch_frame::Scalar to JSON-compatible type using repr()
                if (value.is_null()) {
                    row.push_back(nullptr);
                } else {
                    // Use repr() which returns a string representation
                    row.push_back(value.repr());
                }
            }
            rows.push_back(row);
        }
        dataJson["rows"] = rows;

        json["data"] = dataJson;
    } else {
        json["data"] = glz::generic::object_t{};
    }

    // Serialize to JSON string
    auto result = glz::write_json(json);
    if (!result) {
        throw std::runtime_error("Failed to serialize selector to JSON");
    }

    return result.value();
}

std::string SelectorComparator::ToJson(const std::vector<EventMarkerData>& selectors,
                                      bool prettyPrint) {
    glz::generic::array_t jsonArray;

    for (const auto& selector : selectors) {
        std::string selectorJson = ToJson(selector, false);
        auto parsed = glz::read_json<glz::generic>(selectorJson);
        if (parsed) {
            jsonArray.push_back(parsed.value());
        }
    }

    auto result = glz::write_json(jsonArray);
    if (!result) {
        throw std::runtime_error("Failed to serialize selectors to JSON array");
    }

    return result.value();
}

std::string SelectorComparator::LoadJson(const std::filesystem::path& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + jsonPath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void SelectorComparator::SaveJson(const std::vector<EventMarkerData>& selectors,
                                  const std::filesystem::path& jsonPath) {
    // Ensure parent directory exists
    std::filesystem::create_directories(jsonPath.parent_path());

    std::string jsonStr = ToJson(selectors, true);

    std::ofstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + jsonPath.string());
    }

    file << jsonStr;
    file.close();

    SPDLOG_DEBUG("Saved {} selectors to {}", selectors.size(), jsonPath.string());
}

bool SelectorComparator::Compare(const std::string& expectedJson,
                                 const std::string& actualJson,
                                 std::string& diff) {
    if (expectedJson == actualJson) {
        return true;
    }

    diff = GenerateDiff(expectedJson, actualJson);
    return false;
}

bool SelectorComparator::Compare(const std::vector<EventMarkerData>& expected,
                                 const std::vector<EventMarkerData>& actual,
                                 std::string& diff) {
    std::string expectedJson = ToJson(expected, true);
    std::string actualJson = ToJson(actual, true);
    return Compare(expectedJson, actualJson, diff);
}

std::string SelectorComparator::GenerateDiff(const std::string& expectedJson,
                                             const std::string& actualJson) {
    std::ostringstream diffStream;

    // Split into lines
    auto splitLines = [](const std::string& str) -> std::vector<std::string> {
        std::vector<std::string> lines;
        std::istringstream stream(str);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        return lines;
    };

    auto expectedLines = splitLines(expectedJson);
    auto actualLines = splitLines(actualJson);

    diffStream << "Selector JSON Diff:\n";
    diffStream << "------------------\n\n";

    size_t maxLines = std::max(expectedLines.size(), actualLines.size());

    for (size_t i = 0; i < maxLines; ++i) {
        std::string expectedLine = (i < expectedLines.size()) ? expectedLines[i] : "<missing>";
        std::string actualLine = (i < actualLines.size()) ? actualLines[i] : "<missing>";

        if (expectedLine != actualLine) {
            diffStream << "Line " << (i + 1) << ":\n";
            diffStream << "  Expected: " << expectedLine << "\n";
            diffStream << "  Actual:   " << actualLine << "\n";
            diffStream << "\n";
        }
    }

    diffStream << "\n=== Full Expected ===\n" << expectedJson << "\n\n";
    diffStream << "=== Full Actual ===\n" << actualJson << "\n";

    return diffStream.str();
}

} // namespace epoch_script::runtime::test
