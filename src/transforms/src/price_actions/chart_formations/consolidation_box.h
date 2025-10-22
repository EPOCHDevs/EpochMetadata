#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/factory/dataframe_factory.h>
#include <iostream>

namespace epoch_metadata::transform {

/**
 * ConsolidationBox - Detects horizontal support/resistance rectangles
 * Based on Bulkowski's Rectangle Pattern criteria:
 * - Two near-horizontal parallel lines
 * - Minimum 5 total touches (one line ≥3, other ≥2)
 * - Distinct peaks and valleys required
 */
class ConsolidationBox : public ITransform {
public:
  explicit ConsolidationBox(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_min_pivot_points(
            config.GetOptionValue("min_pivot_points").GetInteger()),
        m_r_squared_min(config.GetOptionValue("r_squared_min").GetDecimal()),
        m_max_slope(config.GetOptionValue("max_slope").GetDecimal()) {}

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

    std::vector<bool> box_detected(N, false);
    std::vector<double> box_top_result(N, nan);
    std::vector<double> box_bottom_result(N, nan);
    std::vector<double> box_height_result(N, nan);
    std::vector<int64_t> touch_count_result(N, 0);
    std::vector<double> upper_slope_result(N, nan);
    std::vector<double> lower_slope_result(N, nan);
    std::vector<double> target_up_result(N, nan);
    std::vector<double> target_down_result(N, nan);

    // Detect pivots (using left_count=3, right_count=3 like other patterns)
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

    // Pattern detection - scan for horizontal consolidation boxes
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

      // Check if the correct number of pivot points have been found
      size_t total_touches = xxmax.size() + xxmin.size();
      if (total_touches < m_min_pivot_points || xxmax.size() == 0 || xxmin.size() == 0)
        continue;

      // Bulkowski requirement: minimum 5 touches, with at least 2 on each line
      if (total_touches < 5 || xxmax.size() < 2 || xxmin.size() < 2)
        continue;

      // Run the regression to get the slope, intercepts and r-squared
      auto lower_line = PatternValidator::calculate_linear_regression(xxmin, minim);
      auto upper_line = PatternValidator::calculate_linear_regression(xxmax, maxim);

      double slmin = lower_line.slope;
      double rmin = lower_line.r_squared;
      double slmax = upper_line.slope;
      double rmax = upper_line.r_squared;

      // Convert r_squared_min (which is actually r) to r_squared threshold
      const double r_squared_threshold = m_r_squared_min * m_r_squared_min;

      // Key difference from Flag: require HORIZONTAL lines (slope ≈ 0)
      // Both slopes must be near zero (absolute value less than threshold)
      if (std::abs(slmin) > m_max_slope || std::abs(slmax) > m_max_slope)
        continue;

      // Require good fit
      if (rmax < r_squared_threshold || rmin < r_squared_threshold)
        continue;

      // Calculate box boundaries using regression lines
      double candle_idx_d = static_cast<double>(candle_idx);
      double box_top = upper_line.slope * candle_idx_d + upper_line.intercept;
      double box_bottom = lower_line.slope * candle_idx_d + lower_line.intercept;
      double box_height = box_top - box_bottom;

      // Sanity check: box must have positive height
      if (box_height <= 0)
        continue;

      // Pattern detected!
      box_detected[candle_idx] = true;
      box_top_result[candle_idx] = box_top;
      box_bottom_result[candle_idx] = box_bottom;
      box_height_result[candle_idx] = box_height;
      touch_count_result[candle_idx] = static_cast<int64_t>(total_touches);
      upper_slope_result[candle_idx] = slmax;
      lower_slope_result[candle_idx] = slmin;

      // Calculate breakout targets (box height projected from boundaries)
      target_up_result[candle_idx] = box_top + box_height;
      target_down_result[candle_idx] = box_bottom - box_height;
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema(
            {{GetOutputId("box_detected"), arrow::boolean()},
             {GetOutputId("box_top"), arrow::float64()},
             {GetOutputId("box_bottom"), arrow::float64()},
             {GetOutputId("box_height"), arrow::float64()},
             {GetOutputId("touch_count"), arrow::int64()},
             {GetOutputId("upper_slope"), arrow::float64()},
             {GetOutputId("lower_slope"), arrow::float64()},
             {GetOutputId("target_up"), arrow::float64()},
             {GetOutputId("target_down"), arrow::float64()}}),
        {factory::array::make_array(box_detected),
         factory::array::make_array(box_top_result),
         factory::array::make_array(box_bottom_result),
         factory::array::make_array(box_height_result),
         factory::array::make_array(touch_count_result),
         factory::array::make_array(upper_slope_result),
         factory::array::make_array(lower_slope_result),
         factory::array::make_array(target_up_result),
         factory::array::make_array(target_down_result)}));
  }

private:
  size_t m_lookback;
  size_t m_min_pivot_points;
  double m_r_squared_min;
  double m_max_slope;
};

} // namespace epoch_metadata::transform
