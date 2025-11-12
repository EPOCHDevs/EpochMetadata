#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <epoch_script/transforms/runtime/types.h>
#include <epoch_frame/dataframe.h>

namespace epoch_script::runtime::test {

namespace fs = std::filesystem;

/**
 * @brief Tracks transform test coverage and data quality metrics
 *
 * Singleton class that tracks which transforms have been tested, how many times,
 * and various quality metrics about their outputs (null rates, execution time, etc.)
 */
class CoverageTracker {
public:
    /**
     * @brief Statistics about null values in outputs
     */
    struct NullStatistics {
        size_t all_null_count = 0;     ///< Number of tests where output was all null
        size_t some_null_count = 0;    ///< Number of tests where output had some nulls
        size_t no_null_count = 0;      ///< Number of tests where output had no nulls

        size_t TotalTests() const {
            return all_null_count + some_null_count + no_null_count;
        }

        double AllNullPercent() const {
            auto total = TotalTests();
            return total > 0 ? (100.0 * all_null_count / total) : 0.0;
        }
    };

    /**
     * @brief Statistics about output values
     */
    struct ValueStatistics {
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        double sum = 0.0;
        double sum_of_squares = 0.0;
        size_t count = 0;

        double Mean() const {
            return count > 0 ? sum / count : 0.0;
        }

        double StdDev() const {
            if (count <= 1) return 0.0;
            double mean = Mean();
            double variance = (sum_of_squares / count) - (mean * mean);
            return variance > 0 ? std::sqrt(variance) : 0.0;
        }

        void Update(double value) {
            min = std::min(min, value);
            max = std::max(max, value);
            sum += value;
            sum_of_squares += value * value;
            count++;
        }
    };

    /**
     * @brief Statistics about output sizes
     */
    struct OutputSizeStatistics {
        size_t total_rows = 0;
        size_t total_columns = 0;
        size_t count = 0;

        double AvgRows() const {
            return count > 0 ? static_cast<double>(total_rows) / count : 0.0;
        }

        double AvgColumns() const {
            return count > 0 ? static_cast<double>(total_columns) / count : 0.0;
        }

        void Update(size_t rows, size_t columns) {
            total_rows += rows;
            total_columns += columns;
            count++;
        }
    };

    /**
     * @brief All metrics for a single transform
     */
    struct TransformMetrics {
        std::string transform_name;
        size_t test_count = 0;
        size_t pass_count = 0;
        size_t fail_count = 0;

        // Execution time tracking
        int64_t total_execution_time_ms = 0;

        // Data quality metrics
        NullStatistics null_stats;
        ValueStatistics value_stats;
        OutputSizeStatistics output_size_stats;

        // Scenario tracking
        std::set<size_t> asset_counts_tested;      ///< e.g., {1, 30, 500}
        std::set<std::string> timeframes_tested;   ///< e.g., {"1D", "1H"}

        double AvgExecutionTimeMs() const {
            return test_count > 0 ? static_cast<double>(total_execution_time_ms) / test_count : 0.0;
        }
    };

    /**
     * @brief Coverage report with summary statistics
     */
    struct CoverageReport {
        size_t total_transforms = 0;
        size_t tested_transforms = 0;
        std::vector<std::string> untested_transforms;
        std::map<std::string, TransformMetrics> metrics;

        double CoveragePercent() const {
            return total_transforms > 0 ? (100.0 * tested_transforms / total_transforms) : 0.0;
        }

        std::vector<std::pair<std::string, size_t>> GetMostTestedTransforms(size_t limit = 5) const;
        std::vector<std::pair<std::string, double>> GetSlowestTransforms(size_t limit = 5) const;
        std::vector<std::pair<std::string, double>> GetHighNullRateTransforms(double threshold = 50.0) const;

        void WriteToFile(const fs::path& output_path) const;
        void PrintSummary(std::ostream& os = std::cout) const;
    };

    // Singleton access
    static CoverageTracker& GetInstance();

    // Deleted copy/move constructors
    CoverageTracker(const CoverageTracker&) = delete;
    CoverageTracker& operator=(const CoverageTracker&) = delete;
    CoverageTracker(CoverageTracker&&) = delete;
    CoverageTracker& operator=(CoverageTracker&&) = delete;

    /**
     * @brief Record a test execution for a transform
     * @param transform_name Name of the transform
     * @param outputs Output dataframes from the transform (for quality analysis)
     * @param execution_time_ms Time taken to execute
     * @param passed Whether the test passed
     * @param asset_count Number of assets tested (1, 30, 500, etc.)
     * @param timeframe Timeframe tested ("1D", "1H", etc.)
     */
    void RecordExecution(
        const std::string& transform_name,
        const TimeFrameAssetDataFrameMap& outputs,
        int64_t execution_time_ms,
        bool passed = true,
        size_t asset_count = 1,
        const std::string& timeframe = "1D"
    );

    /**
     * @brief Record a test execution without output validation (for executors/reporters)
     * @param transform_name Name of the transform
     * @param execution_time_ms Time taken to execute
     * @param passed Whether the test passed
     * @param asset_count Number of assets tested
     * @param timeframe Timeframe tested
     */
    void RecordExecutionNoOutput(
        const std::string& transform_name,
        int64_t execution_time_ms,
        bool passed = true,
        size_t asset_count = 1,
        const std::string& timeframe = "1D"
    );

    /**
     * @brief Generate comprehensive coverage report
     * @return Coverage report with all metrics
     */
    CoverageReport GenerateReport() const;

    /**
     * @brief Reset all tracked data (useful for testing)
     */
    void Reset();

    /**
     * @brief Set the total number of registered transforms (from registry)
     * @param total Total transform count
     */
    void SetTotalTransforms(size_t total) {
        total_transforms_ = total;
    }

    /**
     * @brief Get metrics for a specific transform
     * @param transform_name Name of the transform
     * @return Pointer to metrics, or nullptr if not found
     */
    const TransformMetrics* GetMetrics(const std::string& transform_name) const;

private:
    CoverageTracker() = default;
    ~CoverageTracker() = default;

    /**
     * @brief Analyze a DataFrame for null statistics and value statistics
     */
    void AnalyzeDataFrame(
        const epoch_frame::DataFrame& df,
        NullStatistics& null_stats,
        ValueStatistics& value_stats,
        OutputSizeStatistics& size_stats
    );

    /**
     * @brief Check if all values in DataFrame are null
     */
    bool IsAllNull(const epoch_frame::DataFrame& df) const;

    /**
     * @brief Check if some values in DataFrame are null
     */
    bool HasSomeNull(const epoch_frame::DataFrame& df) const;

    size_t total_transforms_ = 0;
    std::map<std::string, TransformMetrics> metrics_;
};

} // namespace epoch_script::runtime::test
