#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/factory/dataframe_factory.h>

namespace epoch_metadata::transform {

/**
 * Triangles - Direct port from Python triangles.py
 * Detects ascending, descending, and symmetrical triangle patterns
 */
class Triangles : public ITransform {
public:
  explicit Triangles(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_triangle_type(config.GetOptionValue("triangle_type").GetString()),
        m_r_squared_min(config.GetOptionValue("r_squared_min").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    using namespace pattern_utils;
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    const size_t N = bars.num_rows();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();

    std::vector<bool> pattern_detected(N, false);
    std::vector<double> upper_slope(N, nan);
    std::vector<double> lower_slope(N, nan);
    std::vector<std::string> triangle_type_result(N, "");

    // Detect pivots (Python uses left_count=3, right_count=3 by default)
    std::vector<size_t> pivot_high_indices;
    std::vector<size_t> pivot_low_indices;

    constexpr size_t pivot_window = 3;
    for (size_t i = pivot_window; i < N - pivot_window; ++i) {
      bool is_pivot_high = true;
      bool is_pivot_low = true;

      for (size_t j = i - pivot_window; j <= i + pivot_window; ++j) {
        if (j == i)
          continue;
        if (high[i].as_double() < high[j].as_double())
          is_pivot_high = false;
        if (low[i].as_double() > low[j].as_double())
          is_pivot_low = false;
      }

      if (is_pivot_high)
        pivot_high_indices.push_back(i);
      if (is_pivot_low)
        pivot_low_indices.push_back(i);
    }

    // Pattern detection - direct port from Python lines 68-128
    for (size_t candle_idx = m_lookback; candle_idx < N; ++candle_idx) {
      std::vector<double> maxim;
      std::vector<double> minim;
      std::vector<double> xxmin;
      std::vector<double> xxmax;

      // Python lines 75-81: Collect pivots in window
      for (size_t i = candle_idx - m_lookback; i <= candle_idx && i < N; ++i) {
        // Check if i is in pivot_low_indices (Python line 76-78)
        if (std::find(pivot_low_indices.begin(), pivot_low_indices.end(), i) != pivot_low_indices.end()) {
          minim.push_back(low[i].as_double());
          xxmin.push_back(static_cast<double>(i));
        }
        // Check if i is in pivot_high_indices (Python line 79-81)
        if (std::find(pivot_high_indices.begin(), pivot_high_indices.end(), i) != pivot_high_indices.end()) {
          maxim.push_back(high[i].as_double());
          xxmax.push_back(static_cast<double>(i));
        }
      }

      // Python line 84-85: Check minimum pivot points (default min_points=3)
      constexpr size_t min_points = 3;
      if ((xxmax.size() < min_points && xxmin.size() < min_points) ||
          xxmax.size() == 0 || xxmin.size() == 0)
        continue;

      // Python lines 87-88: Run linear regression
      auto lower_line = PatternValidator::calculate_linear_regression(xxmin, minim);
      auto upper_line = PatternValidator::calculate_linear_regression(xxmax, maxim);

      double slmin = lower_line.slope;
      double rmin = lower_line.r_squared;
      double slmax = upper_line.slope;
      double rmax = upper_line.r_squared;

      // Python defaults: slmax_limit=0.00001, slmin_limit=0.00001
      constexpr double slmax_limit = 0.00001;
      constexpr double slmin_limit = 0.00001;

      // Python uses correlation coefficient r (not r_squared)
      // Python checks abs(r) >= rlimit, which equals r_squared >= rlimit^2
      const double r_squared_threshold = m_r_squared_min * m_r_squared_min;

      // Python line 90-101: Symmetrical triangle (if/elif/elif structure)
      if (m_triangle_type == "symmetrical") {
        if (rmax >= r_squared_threshold && rmin >= r_squared_threshold &&
            slmin >= slmin_limit && slmax <= -1.0 * slmax_limit) {
          pattern_detected[candle_idx] = true;
          upper_slope[candle_idx] = slmax;
          lower_slope[candle_idx] = slmin;
          triangle_type_result[candle_idx] = "symmetrical";
        }
      }
      // Python line 103-114: Ascending triangle
      else if (m_triangle_type == "ascending") {
        if (rmax >= r_squared_threshold && rmin >= r_squared_threshold &&
            slmin >= slmin_limit && (slmax >= -1.0 * slmax_limit && slmax <= slmax_limit)) {
          pattern_detected[candle_idx] = true;
          upper_slope[candle_idx] = slmax;
          lower_slope[candle_idx] = slmin;
          triangle_type_result[candle_idx] = "ascending";
        }
      }
      // Python line 116-127: Descending triangle
      else if (m_triangle_type == "descending") {
        if (rmax >= r_squared_threshold && rmin >= r_squared_threshold &&
            slmax <= -1.0 * slmax_limit && (slmin >= -1.0 * slmin_limit && slmin <= slmin_limit)) {
          pattern_detected[candle_idx] = true;
          upper_slope[candle_idx] = slmax;
          lower_slope[candle_idx] = slmin;
          triangle_type_result[candle_idx] = "descending";
        }
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("pattern_detected"), arrow::boolean()},
                       {GetOutputId("upper_slope"), arrow::float64()},
                       {GetOutputId("lower_slope"), arrow::float64()},
                       {GetOutputId("triangle_type"), arrow::utf8()}}),
        {factory::array::make_array(pattern_detected),
         factory::array::make_array(upper_slope),
         factory::array::make_array(lower_slope),
         factory::array::make_array(triangle_type_result)}));
  }

private:
  size_t m_lookback;
  std::string m_triangle_type;
  double m_r_squared_min;
};

} // namespace epoch_metadata::transform
