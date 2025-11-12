#include "json_test_loader.h"
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <glaze/glaze.hpp>

namespace fs = std::filesystem;

namespace epoch_script::test {

std::vector<std::pair<std::string, JsonTestCase>> JsonTestLoader::LoadAllTests(
    const fs::path& test_dir)
{
    std::vector<std::pair<std::string, JsonTestCase>> cases;

    if (!fs::exists(test_dir) || !fs::is_directory(test_dir)) {
        return cases;
    }

    // Recursively scan for JSON files
    ScanForJsonFiles(test_dir, test_dir, cases);

    // Sort by test name for consistent ordering
    std::sort(cases.begin(), cases.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    return cases;
}

JsonTestCase JsonTestLoader::ParseTestFile(const fs::path& json_file)
{
    // Read file contents
    std::ifstream file(json_file);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open test file: " + json_file.string());
    }

    std::string json_content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};

    // Parse JSON using glaze
    auto result = glz::read_json<JsonTestCase>(json_content);
    if (!result.has_value()) {
        throw std::runtime_error(
            "Failed to parse test file: " + json_file.string() + "\n" +
            "Error: " + glz::format_error(result.error(), json_content));
    }

    return result.value();
}

std::optional<fs::path> JsonTestLoader::FindTestCasesRoot()
{
    // 1) Running from build/bin: ./test_cases_json
    fs::path p1 = fs::current_path() / "test_cases_json";
    if (fs::exists(p1) && fs::is_directory(p1)) {
        return p1;
    }

    // 2) Running from repo root: ./test/integration/test_cases_json
    fs::path p2 = fs::current_path() / "test" / "integration" / "test_cases_json";
    if (fs::exists(p2) && fs::is_directory(p2)) {
        return p2;
    }

    // 3) Running from build root: ./bin/test_cases_json
    fs::path p3 = fs::current_path() / "bin" / "test_cases_json";
    if (fs::exists(p3) && fs::is_directory(p3)) {
        return p3;
    }

    return std::nullopt;
}

void JsonTestLoader::ScanForJsonFiles(
    const fs::path& dir,
    const fs::path& base_dir,
    std::vector<std::pair<std::string, JsonTestCase>>& cases)
{
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(dir))
    {
        // Skip special directories
        if (entry.is_directory())
        {
            if (ShouldSkipDirectory(entry.path().filename().string())) {
                continue;
            }

            // Recursively scan subdirectory
            ScanForJsonFiles(entry.path(), base_dir, cases);
            continue;
        }

        // Check if this is a JSON file
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }

        // Parse JSON test file
        try {
            auto test_case = ParseTestFile(entry.path());

            // Build test name from relative path (without .json extension)
            fs::path relative_path = fs::relative(entry.path(), base_dir);
            std::string test_name = relative_path.replace_extension("").string();

            cases.emplace_back(test_name, std::move(test_case));
        }
        catch (const std::exception& e) {
            // Log error but continue scanning
            // The test runner will report this as a failed test load
            throw std::runtime_error(
                "Failed to load test case: " + entry.path().string() + "\n" +
                "Error: " + e.what());
        }
    }
}

bool JsonTestLoader::ShouldSkipDirectory(const std::string& name)
{
    // Skip special directories
    return name == "archived" ||
           name == "shared_data" ||
           name == "actual" ||  // Skip actual/ directories from old test runs
           name == "expected";  // Skip old expected/ directories
}

} // namespace epoch_script::test
