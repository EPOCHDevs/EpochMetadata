#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/factory/dataframe_factory.h>

namespace epoch_metadata::transform {

/**
 * Flag - Direct port from Python flag.py
 * Detects parallel channel consolidation patterns
 */
class Flag : public ITransform {
public:
  explicit Flag(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_min_pivot_points(
            config.GetOptionValue("min_pivot_points").GetInteger()),
        m_r_squared_min(config.GetOptionValue("r_squared_min").GetDecimal()),
        m_slope_parallel_tolerance(
            config.GetOptionValue("slope_parallel_tolerance").GetDecimal()) {}

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

    std::vector<bool> bull_flag_detected(N, false);
    std::vector<bool> bear_flag_detected(N, false);
    std::vector<double> slmax_result(N, nan);
    std::vector<double> slmin_result(N, nan);

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

    // Pattern detection - direct port from Python lines 77-118
    for (size_t candle_idx = m_lookback; candle_idx < N; ++candle_idx) {
      std::vector<double> maxim;
      std::vector<double> minim;
      std::vector<double> xxmin;
      std::vector<double> xxmax;

      // Collect pivots in window [candle_idx-lookback, candle_idx+1]
      for (size_t i = candle_idx - m_lookback; i <= candle_idx && i < N; ++i) {
        // Check if i is in pivot_low_indices
        if (std::find(pivot_low_indices.begin(), pivot_low_indices.end(), i) != pivot_low_indices.end()) {
          minim.push_back(low[i].as_double());
          xxmin.push_back(static_cast<double>(i));
        }
        // Check if i is in pivot_high_indices
        if (std::find(pivot_high_indices.begin(), pivot_high_indices.end(), i) != pivot_high_indices.end()) {
          maxim.push_back(high[i].as_double());
          xxmax.push_back(static_cast<double>(i));
        }
      }

      // Python line 93-95: Check if the correct number of pivot points have been found
      if ((xxmax.size() < m_min_pivot_points && xxmin.size() < m_min_pivot_points) ||
          xxmax.size() == 0 || xxmin.size() == 0)
        continue;

      // Python line 97-99: Check the order condition of the pivot points is met
      // not (np.any(np.diff(minim) < 0)) means minim is non-decreasing
      // not (np.any(np.diff(maxim) < 0)) means maxim is non-decreasing
      bool minim_ordered = true;
      bool maxim_ordered = true;

      for (size_t i = 1; i < minim.size(); ++i) {
        if (minim[i] < minim[i-1]) {
          minim_ordered = false;
          break;
        }
      }
      for (size_t i = 1; i < maxim.size(); ++i) {
        if (maxim[i] < maxim[i-1]) {
          maxim_ordered = false;
          break;
        }
      }

      if (!minim_ordered || !maxim_ordered)
        continue;

      // Python lines 101-103: Run the regress to get the slope, intercepts and r-squared
      auto lower_line = PatternValidator::calculate_linear_regression(xxmin, minim);
      auto upper_line = PatternValidator::calculate_linear_regression(xxmax, maxim);

      double slmin = lower_line.slope;
      double rmin = lower_line.r_squared;
      double slmax = upper_line.slope;
      double rmax = upper_line.r_squared;

      // Python uses correlation coefficient r (not r_squared)
      // Python checks abs(r) >= r_max, which equals r_squared >= r_max^2
      const double r_squared_threshold = m_r_squared_min * m_r_squared_min;

      // Python line 106: BUGGY condition due to missing parentheses!
      // Python: if abs(rmax)>=r_max and abs(rmin)>=r_min and (slmin > slope_min and slmax > slope_max) or (slmin < slope_min and slmax < slope_max):
      // Due to operator precedence, this is parsed as:
      // if (abs(rmax)>=r_max and abs(rmin)>=r_min and (slmin > 0 and slmax > 0)) or (slmin < 0 and slmax < 0):
      // So negative slopes (bull flags) don't require r_squared check!
      constexpr double slope_min = 0.0;
      constexpr double slope_max = 0.0;
      constexpr double lower_ratio_slope = 0.9;
      constexpr double upper_ratio_slope = 1.05;

      // Replicate the buggy Python logic to match test expectations
      bool condition_met = (rmax >= r_squared_threshold && rmin >= r_squared_threshold &&
                           slmin > slope_min && slmax > slope_max) ||
                          (slmin < slope_min && slmax < slope_max);

      if (!condition_met)
        continue;

      // Python line 107: Check slope ratio
      // Need to handle division by zero
      if (slmax == 0.0)
        continue;

      const double slope_ratio = slmin / slmax;
      if (slope_ratio > lower_ratio_slope && slope_ratio < upper_ratio_slope) {
        // Pattern detected!
        // Note: Python doesn't distinguish bull/bear, just sets flag_point
        // But C++ implementation provides both outputs for flexibility
        if (slmax < 0 && slmin < 0) {
          // Negative slopes = downward consolidation = bull flag
          bull_flag_detected[candle_idx] = true;
        } else if (slmax > 0 && slmin > 0) {
          // Positive slopes = upward consolidation = bear flag
          bear_flag_detected[candle_idx] = true;
        }
        slmax_result[candle_idx] = slmax;
        slmin_result[candle_idx] = slmin;
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema(
            {{GetOutputId("bull_flag"), arrow::boolean()},
             {GetOutputId("bear_flag"), arrow::boolean()},
             {GetOutputId("slmax"), arrow::float64()},
             {GetOutputId("slmin"), arrow::float64()}}),
        {factory::array::make_array(bull_flag_detected),
         factory::array::make_array(bear_flag_detected),
         factory::array::make_array(slmax_result),
         factory::array::make_array(slmin_result)}));
  }

private:
  size_t m_lookback;
  size_t m_min_pivot_points;
  double m_r_squared_min;
  double m_slope_parallel_tolerance;
};

} // namespace epoch_metadata::transform
