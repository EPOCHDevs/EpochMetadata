#!/usr/bin/env python3
"""
Helper script to compile EpochScript to graph.json
Uses the C++ compiler through Python bindings or subprocess
"""

import sys
import os
import json
import subprocess

def compile_to_json_via_test(input_path, output_path):
    """
    Compile by running a minimal test that just compiles and outputs JSON
    """
    # Read the source file
    with open(input_path, 'r') as f:
        source = f.read()

    # Create a temporary C++ file that compiles and outputs JSON
    temp_cpp = """
#include <iostream>
#include <fstream>
#include <algorithm>
#include <glaze/glaze.hpp>
#include "transforms/compiler/ast_compiler.h"

int main() {
    std::string source = R"SOURCE({source})SOURCE";

    try {{
        epoch_script::AlgorithmAstCompiler compiler;
        auto result = compiler.compile(source);

        // Sort by id for consistency
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) {{ return a.id < b.id; }});

        // Serialize to JSON
        auto json = glz::write<glz::opts{{.prettify = true}}>(result);
        if (!json.has_value()) {{
            std::cerr << "Failed to serialize" << std::endl;
            return 1;
        }}

        std::cout << json.value();
        return 0;
    }}
    catch (const std::exception& e) {{
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }}
}}
"""
    temp_cpp = temp_cpp.format(source=source.replace('\\', '\\\\').replace('"', '\\"'))

    # Write temp file
    temp_file = "/tmp/compile_temp.cpp"
    with open(temp_file, 'w') as f:
        f.write(temp_cpp)

    # Get project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    build_dir = os.path.join(project_root, "cmake-build-debug")

    # Compile and run
    temp_exe = "/tmp/compile_temp"

    # Compile
    compile_cmd = [
        "g++", "-std=c++23", temp_file,
        "-I" + os.path.join(project_root, "include"),
        "-I" + os.path.join(project_root, "src"),
        "-I" + os.path.join(build_dir, "vcpkg_installed/x64-linux/include"),
        "-L" + os.path.join(build_dir, "lib"),
        "-lepoch_script",
        "-o", temp_exe
    ]

    print(f"Compiling {input_path}...", file=sys.stderr)
    result = subprocess.run(compile_cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Compilation failed: {result.stderr}", file=sys.stderr)
        return False

    # Run
    print(f"Generating JSON...", file=sys.stderr)
    result = subprocess.run([temp_exe], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Execution failed: {result.stderr}", file=sys.stderr)
        return False

    # Write output
    with open(output_path, 'w') as f:
        f.write(result.stdout)

    # Clean up
    os.remove(temp_file)
    if os.path.exists(temp_exe):
        os.remove(temp_exe)

    return True

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.txt> <output.json>", file=sys.stderr)
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    if not os.path.exists(input_path):
        print(f"Error: Input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    success = compile_to_json_via_test(input_path, output_path)

    if not success:
        sys.exit(1)

    print(f"Successfully generated {output_path}", file=sys.stderr)

if __name__ == "__main__":
    main()
