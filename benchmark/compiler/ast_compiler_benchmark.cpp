//
// EpochFlow AST Compiler Benchmark
// Benchmarks for compilation performance across different script complexities
//

#include <catch2/catch_all.hpp>
#include <benchmark_utils.h>
#include "transforms/compiler/ast_compiler.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

using namespace epochflow;
using namespace epoch_benchmark;

// Helper to load script from benchmark/scripts directory
static std::string load_benchmark_script(const std::string &script_name) {
    std::string scripts_dir = BENCHMARK_SCRIPTS_DIR;
    std::filesystem::path script_path = std::filesystem::path(scripts_dir) / script_name;
    return load_script(script_path.string());
}

//=============================================================================
// WORST CASE: Simple Script (3 lines - Literals Only)
// Expected: Fastest compilation time (< 100 µs)
//=============================================================================

TEST_CASE("AST Compiler - Simple Script (Worst Case)", "[compiler][simple][baseline]") {
    std::string script = load_benchmark_script("worst_case_simple.txt");

    SPDLOG_INFO("=== Worst Case Benchmark: Simple Script ===");
    SPDLOG_INFO("Script: {} characters, ~3 lines", script.size());

    BENCHMARK("Compile simple script (3 lines - literals)") {
        AlgorithmAstCompiler compiler;
        auto result = compiler.compile(script);
        REQUIRE(result.size() == 3);  // Expect 3 number/boolean/text nodes
        return result.size();
    };
}

//=============================================================================
// MIDDLE CASE: Basic Strategy (8 lines - Typical Use)
// Expected: Moderate compilation time (< 500 µs)
//=============================================================================

TEST_CASE("AST Compiler - Basic Strategy (Middle Case)", "[compiler][medium][baseline]") {
    std::string script = load_benchmark_script("middle_case_basic.txt");

    SPDLOG_INFO("=== Middle Case Benchmark: Basic Strategy ===");
    SPDLOG_INFO("Script: {} characters, ~8 lines", script.size());

    BENCHMARK("Compile basic strategy (8 lines - EMA crossover)") {
        AlgorithmAstCompiler compiler;
        auto result = compiler.compile(script);
        REQUIRE(result.size() >= 4);  // Expect: src, ema20, ema50, cross, executor, numbers
        return result.size();
    };
}

//=============================================================================
// BEST CASE: Complex Strategy (34 lines - Advanced Strategy)
// Expected: Slowest compilation time (< 2 ms)
//=============================================================================

TEST_CASE("AST Compiler - Complex Strategy (Best Case)", "[compiler][complex][critical]") {
    std::string script = load_benchmark_script("best_case_complex.txt");

    SPDLOG_INFO("=== Best Case Benchmark: Complex Strategy ===");
    SPDLOG_INFO("Script: {} characters, ~34 lines", script.size());

    BENCHMARK("Compile complex strategy (34 lines - consolidation box)") {
        AlgorithmAstCompiler compiler;
        auto result = compiler.compile(script);
        REQUIRE(result.size() >= 10);  // Expect many nodes for complex strategy
        return result.size();
    };
}

//=============================================================================
// ADDITIONAL BENCHMARKS: Stress Testing
//=============================================================================

TEST_CASE("AST Compiler - Repeated Compilation (Cache Effects)", "[compiler][stress]") {
    std::string script = load_benchmark_script("middle_case_basic.txt");

    SPDLOG_INFO("=== Stress Test: Repeated Compilation ===");

    BENCHMARK("Compile same script 10 times (cache effects)") {
        size_t total_nodes = 0;
        for (int i = 0; i < 10; ++i) {
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(script);
            total_nodes += result.size();
        }
        return total_nodes;
    };
}

TEST_CASE("AST Compiler - Empty Script", "[compiler][edge]") {
    SPDLOG_INFO("=== Edge Case: Empty Script ===");

    BENCHMARK("Compile empty script") {
        AlgorithmAstCompiler compiler;
        std::string empty_script = "";
        try {
            auto result = compiler.compile(empty_script);
            return result.size();
        } catch (const std::exception &e) {
            // Expected to throw or return empty
            return size_t(0);
        }
    };
}

//=============================================================================
// SUMMARY TEST: Compare All Scenarios
//=============================================================================

TEST_CASE("AST Compiler - Performance Summary", "[compiler][summary]") {
    SPDLOG_INFO("==========================================");
    SPDLOG_INFO("AST Compiler Performance Summary");
    SPDLOG_INFO("==========================================");

    // Load baseline if available
    std::string baseline_file = std::string(BENCHMARK_BASELINES_DIR) + "/compiler/ast_compiler_baseline.json";

    // Run all three scenarios with manual timing for summary
    struct Scenario {
        std::string name;
        std::string script_file;
        std::string label;
    };

    std::vector<Scenario> scenarios = {
        {"Simple (Worst)", "worst_case_simple.txt", "Simple"},
        {"Basic (Middle)", "middle_case_basic.txt", "Basic"},
        {"Complex (Best)", "best_case_complex.txt", "Complex"}
    };

    for (const auto &scenario : scenarios) {
        std::string script = load_benchmark_script(scenario.script_file);

        // Run multiple samples manually
        std::vector<double> samples;
        const int num_samples = 100;

        for (int i = 0; i < num_samples; ++i) {
            auto start = Clock::now();
            AlgorithmAstCompiler compiler;
            auto result = compiler.compile(script);
            auto end = Clock::now();

            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            samples.push_back(ms);
        }

        // Create result
        std::map<std::string, std::string> metadata;
        metadata["script_file"] = scenario.script_file;
        metadata["script_size"] = std::to_string(script.size());

        auto result = create_result(scenario.name, samples, metadata);

        // Try to load baseline for comparison
        auto baseline_opt = load_baseline(baseline_file, scenario.name);

        // Log results
        if (baseline_opt.has_value()) {
            auto baseline = baseline_opt.value();
            double change_percent = ((result.mean_ms - baseline.mean_ms) / baseline.mean_ms) * 100.0;

            SPDLOG_INFO("{:20} : {:10.3f} ms | Baseline: {:10.3f} ms | Change: {:+6.1f}%",
                       scenario.label, result.mean_ms, baseline.mean_ms, change_percent);

            check_regression(result, baseline, 10.0);
        } else {
            SPDLOG_INFO("{:20} : {:10.3f} ms (no baseline)",
                       scenario.label, result.mean_ms);
        }
    }

    SPDLOG_INFO("==========================================");

    // If UPDATE_BASELINE environment variable is set, save the baseline
    const char* update_baseline_env = std::getenv("UPDATE_BASELINE");
    if (update_baseline_env != nullptr && std::string(update_baseline_env) == "1") {
        SPDLOG_INFO("UPDATE_BASELINE=1 detected, saving new baseline...");

        std::vector<BenchmarkResult> all_results;

        for (const auto &scenario : scenarios) {
            std::string script = load_benchmark_script(scenario.script_file);

            std::vector<double> samples;
            const int num_samples = 100;

            for (int i = 0; i < num_samples; ++i) {
                auto start = Clock::now();
                AlgorithmAstCompiler compiler;
                auto result = compiler.compile(script);
                auto end = Clock::now();

                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                samples.push_back(ms);
            }

            std::map<std::string, std::string> metadata;
            metadata["script_file"] = scenario.script_file;
            metadata["script_size"] = std::to_string(script.size());

            all_results.push_back(create_result(scenario.name, samples, metadata));
        }

        save_baseline(baseline_file, all_results);
    }
}
