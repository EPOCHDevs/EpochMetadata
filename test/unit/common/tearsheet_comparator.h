#pragma once

#include <filesystem>
#include <string>

#include "epoch_protos/tearsheet.pb.h"


namespace epoch_script::runtime::test {

/**
 * @brief Utility for comparing TearSheet protobuf objects via JSON conversion
 *
 * Converts TearSheets to JSON strings for human-readable comparison and diff generation.
 */
class TearSheetComparator {
public:
    /**
     * @brief Convert TearSheet protobuf to JSON string
     * @param tearsheet The protobuf TearSheet object
     * @param prettyPrint Whether to format JSON with indentation
     * @return JSON string representation
     */
    static std::string ToJson(const epoch_proto::TearSheet& tearsheet, bool prettyPrint = true);

    /**
     * @brief Load TearSheet JSON from file
     * @param jsonPath Path to JSON file
     * @return JSON string content
     */
    static std::string LoadJson(const std::filesystem::path& jsonPath);

    /**
     * @brief Save TearSheet as JSON to file
     * @param tearsheet The protobuf TearSheet object
     * @param jsonPath Output file path
     */
    static void SaveJson(const epoch_proto::TearSheet& tearsheet, const std::filesystem::path& jsonPath);

    /**
     * @brief Compare two TearSheet JSON strings
     * @param expected Expected JSON string
     * @param actual Actual JSON string
     * @param diff Output parameter for diff string (if not equal)
     * @return True if equal, false otherwise
     */
    static bool Compare(const std::string& expectedJson,
                       const std::string& actualJson,
                       std::string& diff);

    /**
     * @brief Compare TearSheets from protobuf objects
     * @param expected Expected TearSheet
     * @param actual Actual TearSheet
     * @param diff Output parameter for diff string (if not equal)
     * @return True if equal, false otherwise
     */
    static bool Compare(const epoch_proto::TearSheet& expected,
                       const epoch_proto::TearSheet& actual,
                       std::string& diff);

private:
    /**
     * @brief Normalize TearSheet by sorting cards for deterministic comparison
     * @param tearsheet TearSheet to normalize (modified in-place)
     */
    static void NormalizeTearSheet(epoch_proto::TearSheet& tearsheet);

    /**
     * @brief Generate diff between two JSON strings
     * @param expectedJson Expected JSON
     * @param actualJson Actual JSON
     * @return Diff string with line-by-line comparison
     */
    static std::string GenerateDiff(const std::string& expectedJson, const std::string& actualJson);
};

} // namespace epoch_script::runtime::test
