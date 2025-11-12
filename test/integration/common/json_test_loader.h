#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <utility>
#include "json_test_case.h"

namespace epoch_script::test {

/**
 * Loads JSON-based integration test cases from filesystem.
 *
 * Scans directories for *.json files and parses them into JsonTestCase structures.
 * Replaces the old directory-based test case scanning system.
 */
class JsonTestLoader {
public:
    /**
     * Load all JSON test cases from a directory (recursively).
     *
     * @param test_dir Root directory containing test case JSON files
     * @return Vector of (test_name, test_case) pairs, sorted by test name
     *
     * Test naming:
     * - Uses relative path from test_dir as test name
     * - Example: "basic/simple_operator.json" -> name is "basic/simple_operator"
     */
    static std::vector<std::pair<std::string, JsonTestCase>> LoadAllTests(
        const std::filesystem::path& test_dir);

    /**
     * Parse a single JSON test file.
     *
     * @param json_file Path to .json test file
     * @return Parsed JsonTestCase structure
     * @throws std::runtime_error if file cannot be read or parsed
     */
    static JsonTestCase ParseTestFile(const std::filesystem::path& json_file);

    /**
     * Find the test cases root directory.
     *
     * Tries several common locations depending on how tests are launched:
     * 1. ./test_cases (running from build/bin)
     * 2. ./test/integration/test_cases (running from repo root)
     * 3. ./bin/test_cases (running from build root)
     *
     * @return Path to test cases directory, or nullopt if not found
     */
    static std::optional<std::filesystem::path> FindTestCasesRoot();

private:
    /**
     * Recursively scan directory for .json test files.
     *
     * @param dir Directory to scan
     * @param base_dir Base directory for computing relative paths
     * @param cases Output vector to append found test cases
     */
    static void ScanForJsonFiles(
        const std::filesystem::path& dir,
        const std::filesystem::path& base_dir,
        std::vector<std::pair<std::string, JsonTestCase>>& cases);

    /**
     * Check if directory should be skipped during scanning.
     *
     * @param name Directory name
     * @return true if directory should be skipped
     */
    static bool ShouldSkipDirectory(const std::string& name);
};

} // namespace epoch_script::test
