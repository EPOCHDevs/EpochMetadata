//
// Tool to generate graph.json from EpochScript input.txt files
// Usage: generate_graph_json <input_txt_path> <output_json_path>
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <glaze/glaze.hpp>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;

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
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_txt_path> <output_json_path>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];

    try {
        // Read source code
        std::string source = readFile(input_path);

        // Compile
        AlgorithmAstCompiler compiler;
        CompilationResult result = compiler.compile(source);

        // Sort by id for consistency
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.id < b.id; });

        // Serialize to JSON
        auto json = glz::write_json(result);
        if (!json.has_value()) {
            std::cerr << "Failed to serialize compilation result" << std::endl;
            return 1;
        }

        // Write to file
        writeFile(output_path, json.value());

        std::cout << "Successfully generated " << output_path << std::endl;
        std::cout << "Generated " << result.size() << " algorithm nodes" << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
