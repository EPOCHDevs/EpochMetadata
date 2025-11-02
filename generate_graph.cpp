//
// Standalone tool to generate graph.json from EpochScript input
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <glaze/glaze.hpp>
#include "transforms/compiler/ast_compiler.h"
#include <epoch_script/transforms/core/registration.h>
#include <epoch_script/strategy/registration.h>
#include <arrow/compute/initialize.h>
#include <absl/log/initialize.h>
#include <google/protobuf/stubs/common.h>

// Simple YAML loader (no files directory needed for compilation-only mode)
static const auto YAML_LOADER = [](const std::string& path) -> YAML::Node {
    return YAML::Node(); // Empty node for standalone compilation
};

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to write file: " + path);
    }
    file << content;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <input.txt> [output.json]" << std::endl;
        std::cerr << "If output.json is not specified, prints to stdout" << std::endl;
        return 1;
    }

    // Initialize (similar to catch_main.cpp)
    absl::InitializeLog();
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto arrowStatus = arrow::compute::Initialize();
    if (!arrowStatus.ok()) {
        std::cerr << "Arrow compute initialization failed" << std::endl;
        return 1;
    }

    // Register transform metadata and initialize registry
    epoch_script::transforms::RegisterTransformMetadata(YAML_LOADER);
    epoch_script::transform::InitializeTransforms(YAML_LOADER, {}, {});

    std::string input_path = argv[1];
    std::string output_path = argc == 3 ? argv[2] : "";

    try {
        // Read source code
        std::string source = readFile(input_path);

        // Compile
        epoch_script::AlgorithmAstCompiler compiler;
        auto result = compiler.compile(source);

        // Sort by id for consistency
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.id < b.id; });

        // Serialize to JSON (pretty print)
        auto json = glz::write<glz::opts{.prettify = true}>(result);
        if (!json.has_value()) {
            std::cerr << "Failed to serialize compilation result" << std::endl;
            return 1;
        }

        // Write to file or stdout
        if (output_path.empty()) {
            std::cout << json.value();
        } else {
            writeFile(output_path, json.value());
            std::cerr << "✓ Generated " << result.size() << " nodes -> " << output_path << std::endl;
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
