//
// Migration Tool: Directory-Based Tests → JSON-Based Tests
//
// Converts old test case directory structure to new JSON format.
//
// Old structure:
//   test_cases/category/test_name/
//   ├── input.txt
//   ├── input_data/ (optional)
//   └── expected/
//       └── graph.json
//
// New structure:
//   test_cases/category/test_name.json
//   {
//     "input": "...",
//     "graph": [...],
//     "runtime": {...} or null,
//     "error": null or "..."
//   }
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <regex>
#include <glaze/glaze.hpp>
#include "transforms/compiler/ast_compiler.h"
#include "../common/json_test_case.h"

namespace fs = std::filesystem;
using namespace epoch_script;
using namespace epoch_script::test;

// Read file contents
std::string ReadFile(const fs::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

// Write file contents
void WriteFile(const fs::path& path, const std::string& content)
{
    // Create parent directories if needed
    fs::create_directories(path.parent_path());

    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + path.string());
    }
    file << content;
}

// Check if directory should be skipped
bool ShouldSkipDirectory(const std::string& name)
{
    return name == "archived" ||
           name == "shared_data" ||
           name == "actual" ||
           name == "expected";
}

// Migrate a single test case
void MigrateTestCase(
    const fs::path& test_dir,
    const fs::path& output_dir,
    const fs::path& base_dir)
{
    // Required files
    fs::path input_file = test_dir / "input.txt";
    fs::path expected_graph_file = test_dir / "expected" / "graph.json";

    // Check if this is a valid test case
    if (!fs::exists(input_file) || !fs::exists(expected_graph_file)) {
        return;  // Not a test case
    }

    std::cout << "Migrating: " << test_dir.string() << std::endl;

    try {
        // Read input script
        std::string input = ReadFile(input_file);

        // Read expected graph
        std::string expected_graph_json = ReadFile(expected_graph_file);

        // Create JsonTestCase
        JsonTestCase test_case;
        test_case.input = input;

        // Check if this is an error case
        if (expected_graph_json.find("\"error\"") != std::string::npos) {
            // Error case - extract error message
            std::regex re(R"(\"error\"\s*:\s*\"([^\"]*)\")");
            std::smatch m;
            if (std::regex_search(expected_graph_json, m, re) && m.size() > 1) {
                test_case.error = m[1].str();
            } else {
                test_case.error = "Unknown error";
            }
            test_case.graph = std::nullopt;
        } else {
            // Success case - parse graph
            auto graph_result = glz::read_json<CompilationResult>(expected_graph_json);
            if (!graph_result.has_value()) {
                std::cerr << "  Warning: Failed to parse graph.json: "
                         << glz::format_error(graph_result.error(), expected_graph_json)
                         << std::endl;
                return;
            }
            test_case.graph = graph_result.value();
            test_case.error = std::nullopt;
        }

        // Check if runtime test data exists
        fs::path input_data_dir = test_dir / "input_data";
        bool has_runtime = fs::exists(input_data_dir) && !fs::is_empty(input_data_dir);

        if (has_runtime) {
            // Has runtime test - create stub runtime validation
            // User will need to manually fill in column validation rules
            test_case.runtime = RuntimeValidation{};
            std::cout << "  Note: Runtime test detected. Please manually add column validations." << std::endl;
        } else {
            test_case.runtime = std::nullopt;
        }

        // Serialize to JSON
        auto json_output = glz::write_json(test_case);
        if (!json_output.has_value()) {
            std::cerr << "  Error: Failed to serialize test case" << std::endl;
            return;
        }

        // Pretty print JSON
        auto pretty_json = glz::prettify_json(json_output.value());

        // Build output path
        fs::path relative_path = fs::relative(test_dir, base_dir);
        fs::path output_file = output_dir / relative_path.replace_extension(".json");

        // Write JSON file
        WriteFile(output_file, pretty_json);

        std::cout << "  ✓ Created: " << output_file.string() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "  ✗ Error: " << e.what() << std::endl;
    }
}

// Recursively scan and migrate test cases
void ScanAndMigrate(
    const fs::path& dir,
    const fs::path& output_dir,
    const fs::path& base_dir)
{
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }

    // Check if this is a test case directory
    fs::path input_file = dir / "input.txt";
    fs::path expected_dir = dir / "expected";
    fs::path expected_graph = expected_dir / "graph.json";

    if (fs::exists(input_file) && fs::exists(expected_graph)) {
        // This is a test case - migrate it
        MigrateTestCase(dir, output_dir, base_dir);
        return;  // Don't recurse into test case directories
    }

    // Not a test case - recurse into subdirectories
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) {
            continue;
        }

        // Skip special directories
        if (ShouldSkipDirectory(entry.path().filename().string())) {
            continue;
        }

        // Recursively scan subdirectory
        ScanAndMigrate(entry.path(), output_dir, base_dir);
    }
}

int main(int argc, char* argv[])
{
    std::cout << "EpochScript Test Migration Tool" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    if (argc != 3) {
        std::cerr << "Usage: migrate_to_json <input_dir> <output_dir>" << std::endl;
        std::cerr << "  input_dir  - Directory containing old test cases" << std::endl;
        std::cerr << "  output_dir - Directory to write JSON test files" << std::endl;
        return 1;
    }

    fs::path input_dir = argv[1];
    fs::path output_dir = argv[2];

    // Validate input directory
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir << std::endl;
        return 1;
    }

    // Create output directory
    fs::create_directories(output_dir);

    std::cout << "Input:  " << input_dir << std::endl;
    std::cout << "Output: " << output_dir << std::endl;
    std::cout << std::endl;

    // Scan and migrate
    ScanAndMigrate(input_dir, output_dir, input_dir);

    std::cout << std::endl;
    std::cout << "Migration complete!" << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "1. Review generated JSON files in: " << output_dir << std::endl;
    std::cout << "2. For runtime tests, manually add column validation rules" << std::endl;
    std::cout << "3. Run tests with: cmake-build-debug/bin/epoch_script_test" << std::endl;

    return 0;
}
