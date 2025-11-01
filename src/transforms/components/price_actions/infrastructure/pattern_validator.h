#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <DataFrame/DataFrame.h>
#include <DataFrame/DataFrameFinancialVisitors.h>

namespace epochflow::transform::pattern_utils {

/**
 * LinearRegressionResult - stores results of linear regression
 */
struct LinearRegressionResult {
  double slope;
  double intercept;
  double r_squared;
  double std_error;
};

/**
 * PatternValidator - Static utility functions for pattern detection
 * Based on chart_patterns library algorithms
 */
class PatternValidator {
public:
  /**
   * Calculate linear regression using Hosseinmoein DataFrame library
   * Returns slope, intercept, R-squared, and standard error
   */
  static LinearRegressionResult
  calculate_linear_regression(const std::vector<double> &x,
                               const std::vector<double> &y) {
    if (x.size() != y.size() || x.size() < 2) {
      return {0.0, 0.0, 0.0, 0.0};
    }

    const size_t n = x.size();
    const double n_double = static_cast<double>(n);

    // Use Hosseinmoein DataFrame linfit_v visitor for linear regression
    hmdf::StdDataFrame<int64_t> df;
    df.load_index(hmdf::StdDataFrame<int64_t>::gen_sequence_index(0, n, 1));
    df.load_column("x", x);
    df.load_column("y", y);

    hmdf::linfit_v<double, int64_t> visitor;
    df.single_act_visit<double, double>("x", "y", visitor);

    const double slope = visitor.get_slope();
    const double intercept = visitor.get_intercept();
    const double ss_res = visitor.get_residual();  // Residual sum of squares

    // Calculate total sum of squares for R-squared
    const double y_mean = std::accumulate(y.begin(), y.end(), 0.0) / n_double;
    double ss_tot = 0.0;
    for (size_t i = 0; i < n; ++i) {
      ss_tot += std::pow(y[i] - y_mean, 2);
    }

    const double r_squared = (ss_tot != 0.0) ? (1.0 - ss_res / ss_tot) : 0.0;

    // Calculate standard error: sqrt(ss_res / (n-2))
    const double std_error =
        (n > 2) ? std::sqrt(ss_res / (n_double - 2.0)) : 0.0;

    return {slope, intercept, r_squared, std_error};
  }

  /**
   * Check if two trendlines are parallel within tolerance
   * Used for flag and pennant patterns
   */
  static bool are_trendlines_parallel(double slope1, double slope2,
                                       double lower_ratio = 0.9,
                                       double upper_ratio = 1.1) {
    if (slope2 == 0.0)
      return false;
    const double ratio = slope1 / slope2;
    return (ratio >= lower_ratio && ratio <= upper_ratio);
  }

  /**
   * Check if values are within a ratio tolerance
   * Used for double tops/bottoms and head & shoulders
   */
  static bool are_values_similar(double val1, double val2,
                                  double tolerance = 0.02) {
    if (val2 == 0.0)
      return std::abs(val1) < tolerance;
    const double ratio = std::abs((val1 - val2) / val2);
    return ratio <= tolerance;
  }

  /**
   * Find indices where pivot type matches target
   */
  static std::vector<size_t>
  find_pivot_indices(const std::vector<int64_t> &pivot_types,
                     int64_t target_type) {
    std::vector<size_t> indices;
    for (size_t i = 0; i < pivot_types.size(); ++i) {
      if (pivot_types[i] == target_type) {
        indices.push_back(i);
      }
    }
    return indices;
  }

  /**
   * Extract values at specific indices
   */
  template <typename T>
  static std::vector<T> extract_at_indices(const std::vector<T> &values,
                                            const std::vector<size_t> &indices) {
    std::vector<T> result;
    result.reserve(indices.size());
    for (size_t idx : indices) {
      if (idx < values.size()) {
        result.push_back(values[idx]);
      }
    }
    return result;
  }

  /**
   * Check if series is monotonically increasing
   */
  static bool is_increasing(const std::vector<double> &values) {
    for (size_t i = 1; i < values.size(); ++i) {
      if (values[i] <= values[i - 1]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Check if series is monotonically decreasing
   */
  static bool is_decreasing(const std::vector<double> &values) {
    for (size_t i = 1; i < values.size(); ++i) {
      if (values[i] >= values[i - 1]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Find maximum value and its index
   */
  static std::pair<double, size_t>
  find_max_with_index(const std::vector<double> &values) {
    if (values.empty()) {
      return {0.0, 0};
    }
    auto it = std::max_element(values.begin(), values.end());
    return {*it, static_cast<size_t>(std::distance(values.begin(), it))};
  }

  /**
   * Find minimum value and its index
   */
  static std::pair<double, size_t>
  find_min_with_index(const std::vector<double> &values) {
    if (values.empty()) {
      return {0.0, 0};
    }
    auto it = std::min_element(values.begin(), values.end());
    return {*it, static_cast<size_t>(std::distance(values.begin(), it))};
  }

  /**
   * Check if point sequence matches expected pattern order
   * For Head & Shoulders: shoulder1 < head > shoulder2
   */
  static bool check_head_and_shoulders_order(
      const std::vector<size_t> &indices, const std::vector<double> &highs,
      [[maybe_unused]] const std::vector<double> &lows, double head_ratio_before = 1.0002,
      double head_ratio_after = 1.0002) {

    if (indices.size() < 5 || highs.size() < 3) {
      return false;
    }

    // Expecting: shoulder1_high, neck1_low, head_high, neck2_low, shoulder2_high
    // In highs array: [shoulder1, head, shoulder2]
    const double shoulder1 = highs[0];
    const double head = highs[1];
    const double shoulder2 = highs[2];

    // Head must be higher than both shoulders by ratio
    const bool head_higher_than_left =
        (head - shoulder1 > 0) && (head / shoulder1 > head_ratio_before);
    const bool head_higher_than_right =
        (head - shoulder2 > 0) && (head / shoulder2 > head_ratio_after);

    return head_higher_than_left && head_higher_than_right;
  }

  /**
   * Validate neckline slope is within acceptable range
   * For Head & Shoulders neckline
   */
  static bool validate_neckline_slope(double slope, double max_slope = 1e-4) {
    return std::abs(slope) <= max_slope;
  }

  /**
   * Check if highs/lows form converging pattern (pennant/triangle)
   */
  static bool is_converging(const LinearRegressionResult &upper_line,
                             const LinearRegressionResult &lower_line) {
    // Slopes must have opposite signs or be converging
    if (upper_line.slope > 0 && lower_line.slope < 0) {
      return true; // Ascending upper, descending lower
    }
    if (upper_line.slope < 0 && lower_line.slope > 0) {
      return true; // Descending upper, ascending lower
    }
    // Both positive but lower slope is greater (converging)
    if (upper_line.slope > 0 && lower_line.slope > 0 &&
        lower_line.slope > upper_line.slope) {
      return true;
    }
    // Both negative but upper slope is less negative (converging)
    if (upper_line.slope < 0 && lower_line.slope < 0 &&
        upper_line.slope > lower_line.slope) {
      return true;
    }
    return false;
  }
};

} // namespace epochflow::transform::pattern_utils
