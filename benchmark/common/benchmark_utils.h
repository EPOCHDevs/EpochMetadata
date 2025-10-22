//
// Benchmark Utilities
// Common utilities for performance benchmarking and regression tracking
//

#pragma once

#include <catch2/catch_all.hpp>
#include <chrono>
#include <fstream>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace epoch_benchmark {

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

/**
 * Structure to hold benchmark results for regression tracking
 */
struct BenchmarkResult {
  std::string name;
  double mean_ms;
  double median_ms;
  double std_dev_ms;
  double min_ms;
  double max_ms;
  size_t samples;
  std::string timestamp;
  std::map<std::string, std::string> metadata; // Additional info like asset count, timeframe, etc.
};

/**
 * Container for multiple benchmark results
 */
struct BenchmarkBaseline {
  std::string version = "1.0";
  int64_t updated = 0;
  std::vector<BenchmarkResult> benchmarks;
};

/**
 * Load baseline results from JSON file
 */
inline std::optional<BenchmarkResult>
load_baseline(const std::string &baseline_file, const std::string &benchmark_name) {
  try {
    std::ifstream file(baseline_file);
    if (!file.is_open()) {
      SPDLOG_WARN("Baseline file not found: {}", baseline_file);
      return std::nullopt;
    }

    std::string json_content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

    BenchmarkBaseline baseline;
    auto parse_error = glz::read_json(baseline, json_content);

    if (parse_error) {
      SPDLOG_ERROR("Error parsing baseline JSON: {}", glz::format_error(parse_error, json_content));
      return std::nullopt;
    }

    for (const auto &bench : baseline.benchmarks) {
      if (bench.name == benchmark_name) {
        return bench;
      }
    }

    SPDLOG_WARN("Benchmark '{}' not found in baseline file", benchmark_name);
    return std::nullopt;
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Error loading baseline: {}", e.what());
    return std::nullopt;
  }
}

/**
 * Save benchmark results to JSON file
 */
inline void save_baseline(const std::string &baseline_file,
                          const std::vector<BenchmarkResult> &results) {
  try {
    BenchmarkBaseline baseline;
    baseline.version = "1.0";
    baseline.updated = std::chrono::system_clock::now().time_since_epoch().count();
    baseline.benchmarks = results;

    std::string buffer;
    auto error = glz::write_file_json(baseline, baseline_file, buffer);
    if (error) {
      SPDLOG_ERROR("Error serializing baseline to JSON: {}", int(error.ec));
      return;
    }

    SPDLOG_INFO("Baseline saved to: {}", baseline_file);
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Error saving baseline: {}", e.what());
  }
}

/**
 * Check for performance regression
 * @param current Current benchmark result
 * @param baseline Baseline benchmark result
 * @param threshold_percent Percentage threshold for regression (default 10%)
 * @return true if regression detected
 */
inline bool check_regression(const BenchmarkResult &current,
                             const BenchmarkResult &baseline,
                             double threshold_percent = 10.0) {
  double regression_percent = ((current.mean_ms - baseline.mean_ms) / baseline.mean_ms) * 100.0;

  if (regression_percent > threshold_percent) {
    SPDLOG_WARN("REGRESSION DETECTED for '{}':", current.name);
    SPDLOG_WARN("  Baseline: {:.3f} ms", baseline.mean_ms);
    SPDLOG_WARN("  Current:  {:.3f} ms", current.mean_ms);
    SPDLOG_WARN("  Regression: {:.1f}% (threshold: {:.1f}%)", regression_percent,
                threshold_percent);
    return true;
  } else if (regression_percent < -threshold_percent) {
    SPDLOG_INFO("IMPROVEMENT DETECTED for '{}':", current.name);
    SPDLOG_INFO("  Baseline: {:.3f} ms", baseline.mean_ms);
    SPDLOG_INFO("  Current:  {:.3f} ms", current.mean_ms);
    SPDLOG_INFO("  Improvement: {:.1f}%", -regression_percent);
  } else {
    SPDLOG_INFO("Performance stable for '{}' (change: {:.1f}%)", current.name,
                regression_percent);
  }

  return false;
}

/**
 * Get current timestamp as ISO 8601 string
 */
inline std::string get_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

/**
 * Format duration in human-readable form
 */
inline std::string format_duration(double milliseconds) {
  if (milliseconds < 1.0) {
    return fmt::format("{:.2f} µs", milliseconds * 1000.0);
  } else if (milliseconds < 1000.0) {
    return fmt::format("{:.2f} ms", milliseconds);
  } else {
    return fmt::format("{:.2f} s", milliseconds / 1000.0);
  }
}

/**
 * Create a benchmark result from manual timing samples
 * Note: This is a helper for manual timing when Catch2 benchmarks aren't suitable
 */
inline BenchmarkResult create_result(const std::string &name,
                                     const std::vector<double> &samples_ms,
                                     const std::map<std::string, std::string> &metadata = {}) {
  if (samples_ms.empty()) {
    throw std::invalid_argument("Cannot create result from empty samples");
  }

  BenchmarkResult result;
  result.name = name;
  result.samples = samples_ms.size();
  result.timestamp = get_timestamp();
  result.metadata = metadata;

  // Calculate statistics
  double sum = 0.0;
  result.min_ms = samples_ms[0];
  result.max_ms = samples_ms[0];

  for (double sample : samples_ms) {
    sum += sample;
    result.min_ms = std::min(result.min_ms, sample);
    result.max_ms = std::max(result.max_ms, sample);
  }

  result.mean_ms = sum / samples_ms.size();

  // Calculate median
  std::vector<double> sorted_samples = samples_ms;
  std::sort(sorted_samples.begin(), sorted_samples.end());
  if (sorted_samples.size() % 2 == 0) {
    result.median_ms = (sorted_samples[sorted_samples.size() / 2 - 1] +
                        sorted_samples[sorted_samples.size() / 2]) /
                       2.0;
  } else {
    result.median_ms = sorted_samples[sorted_samples.size() / 2];
  }

  // Calculate standard deviation
  double variance_sum = 0.0;
  for (double sample : samples_ms) {
    double diff = sample - result.mean_ms;
    variance_sum += diff * diff;
  }
  result.std_dev_ms = std::sqrt(variance_sum / samples_ms.size());

  return result;
}

/**
 * Helper function to load a script file
 */
inline std::string load_script(const std::string &script_path) {
  std::ifstream file(script_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open script file: " + script_path);
  }
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

} // namespace epoch_benchmark
