# EpochMetadata Benchmark Module
# Performance benchmarking and regression tracking

cmake_minimum_required(VERSION 3.20)

message(STATUS "Configuring EpochMetadata Benchmark Module")

# Require Catch2 (should be available via BUILD_TEST=ON)
if(NOT TARGET Catch2::Catch2)
    message(FATAL_ERROR "Catch2 not found. Please enable BUILD_TEST option: cmake .. -DBUILD_TEST=ON")
endif()

# Require spdlog
find_package(spdlog REQUIRED)

# AST Compiler Benchmarks
add_executable(ast_compiler_benchmark
    compiler/ast_compiler_benchmark.cpp
    common/catch_benchmark_main.cpp)

target_link_libraries(ast_compiler_benchmark PRIVATE
    epoch_metadata
    Catch2::Catch2
    spdlog::spdlog
    fmt::fmt)

target_compile_definitions(ast_compiler_benchmark PRIVATE
    -DBENCHMARK_BASELINES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/baselines"
    -DBENCHMARK_SCRIPTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/scripts"
    -DMETADATA_FILES_DIR="${CMAKE_BINARY_DIR}/bin/files")

target_include_directories(ast_compiler_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/src)

# Set output directory for benchmark executables
set_target_properties(ast_compiler_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#=============================================================================
# Custom Targets for Different Run Modes
#=============================================================================

# Full benchmark run (excludes stress tests, 100 samples)
add_custom_target(run_compiler_benchmarks
    COMMAND $<TARGET_FILE:ast_compiler_benchmark> "~[stress]~[edge]" --benchmark-samples 100
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running AST Compiler benchmarks (100 samples)"
    VERBATIM)

# Quick benchmark run (baseline only, 10 samples)
add_custom_target(run_compiler_benchmarks_quick
    COMMAND $<TARGET_FILE:ast_compiler_benchmark> "[baseline]" --benchmark-samples 10
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running AST Compiler benchmarks - Quick mode (10 samples)"
    VERBATIM)

# CI/CD benchmark run (JSON output, 30 samples)
add_custom_target(run_compiler_benchmarks_ci
    COMMAND $<TARGET_FILE:ast_compiler_benchmark> "[baseline]"
            --benchmark-samples 30
            --reporter JSON::out=${CMAKE_BINARY_DIR}/ast_compiler_benchmark_results.json
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running AST Compiler benchmarks for CI/CD (30 samples, JSON output)"
    VERBATIM)

# Update baseline (requires UPDATE_BASELINE=1 environment variable)
add_custom_target(update_compiler_baseline
    COMMAND ${CMAKE_COMMAND} -E env UPDATE_BASELINE=1
            $<TARGET_FILE:ast_compiler_benchmark> "[summary]" --benchmark-samples 100
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Updating AST Compiler baseline (100 samples)"
    VERBATIM)

# Summary report
add_custom_target(compiler_benchmark_summary
    COMMAND $<TARGET_FILE:ast_compiler_benchmark> "[summary]" --benchmark-samples 100
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running AST Compiler benchmark summary"
    VERBATIM)

#=============================================================================
# Installation
#=============================================================================

# Install benchmark scripts
install(DIRECTORY scripts/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/benchmark/scripts
    FILES_MATCHING PATTERN "*.txt")

# Install baseline directory structure
install(DIRECTORY baselines/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/benchmark/baselines
    FILES_MATCHING PATTERN "*.json")

message(STATUS "EpochMetadata Benchmark Module configured successfully")
message(STATUS "  - ast_compiler_benchmark target created")
message(STATUS "  - Custom targets: run_compiler_benchmarks, run_compiler_benchmarks_quick, update_compiler_baseline")
