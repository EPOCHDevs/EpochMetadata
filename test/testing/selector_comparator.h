#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "../../include/epochflow/transforms/core/itransform.h"


namespace epoch_flow::runtime::test {

/**
 * @brief Utility for comparing Selector data via JSON conversion
 *
 * Converts Selector data to JSON for human-readable comparison.
 */
class SelectorComparator {
public:
    using SelectorData = epochflow::transform::SelectorData;

    /**
     * @brief Convert selector data to JSON string
     * @param selector The selector data object
     * @param prettyPrint Whether to format JSON with indentation
     * @return JSON string representation
     */
    static std::string ToJson(const SelectorData& selector, bool prettyPrint = true);

    /**
     * @brief Convert vector of selectors to JSON array
     * @param selectors Vector of selector data
     * @param prettyPrint Whether to format JSON with indentation
     * @return JSON string representation
     */
    static std::string ToJson(const std::vector<SelectorData>& selectors, bool prettyPrint = true);

    /**
     * @brief Load selector JSON from file
     * @param jsonPath Path to JSON file
     * @return JSON string content
     */
    static std::string LoadJson(const std::filesystem::path& jsonPath);

    /**
     * @brief Save selectors as JSON to file
     * @param selectors Vector of selector data
     * @param jsonPath Output file path
     */
    static void SaveJson(const std::vector<SelectorData>& selectors,
                        const std::filesystem::path& jsonPath);

    /**
     * @brief Compare two selector JSON strings
     * @param expected Expected JSON string
     * @param actual Actual JSON string
     * @param diff Output parameter for diff string (if not equal)
     * @return True if equal, false otherwise
     */
    static bool Compare(const std::string& expectedJson,
                       const std::string& actualJson,
                       std::string& diff);

    /**
     * @brief Compare selector vectors
     * @param expected Expected selectors
     * @param actual Actual selectors
     * @param diff Output parameter for diff string (if not equal)
     * @return True if equal, false otherwise
     */
    static bool Compare(const std::vector<SelectorData>& expected,
                       const std::vector<SelectorData>& actual,
                       std::string& diff);

private:
    /**
     * @brief Generate diff between two JSON strings
     * @param expectedJson Expected JSON
     * @param actualJson Actual JSON
     * @return Diff string
     */
    static std::string GenerateDiff(const std::string& expectedJson, const std::string& actualJson);
};

} // namespace epoch_flow::runtime::test
