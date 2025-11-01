#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include <epochflow/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>

namespace epochflow::transform {

/**
 * Pennant - Direct port from Python pennant.py
 * Detects converging trendlines (rising support, falling resistance)
 */
class Pennant : public ITransform {
public:
  explicit Pennant(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_min_pivot_points(
            config.GetOptionValue("min_pivot_points").GetInteger()),
        m_r_squared_min(config.GetOptionValue("r_squared_min").GetDecimal()),
        m_max_duration(config.GetOptionValue("max_duration").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    using namespace pattern_utils;
    const auto &C = epochflow::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    const size_t N = bars.num_rows();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();

    std::vector<bool> bull_pennant(N, false);
    std::vector<bool> bear_pennant(N, false);
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

    // Pattern detection - direct port from Python lines 78-115
    for (size_t candle_idx = m_lookback; candle_idx < N; ++candle_idx) {
      std::vector<double> maxim;
      std::vector<double> minim;
      std::vector<double> xxmin;
      std::vector<double> xxmax;

      // Collect pivots in window [candle_idx-lookback, candle_idx+1]
      for (size_t idx : pivot_low_indices) {
        if (idx >= candle_idx - m_lookback && idx <= candle_idx) {
          minim.push_back(low[idx].as_double());
          xxmin.push_back(static_cast<double>(idx));
        }
      }
      for (size_t idx : pivot_high_indices) {
        if (idx >= candle_idx - m_lookback && idx <= candle_idx) {
          maxim.push_back(high[idx].as_double());
          xxmax.push_back(static_cast<double>(idx));
        }
      }

      // Python line 94-95: Check the correct number of pivot points have been found
      if ((xxmax.size() < m_min_pivot_points && xxmin.size() < m_min_pivot_points) ||
          xxmax.size() == 0 || xxmin.size() == 0)
        continue;

      // Python lines 98-100: Run the regress to get the slope, intercepts and r-squared
      auto lower_line = PatternValidator::calculate_linear_regression(xxmin, minim);
      auto upper_line = PatternValidator::calculate_linear_regression(xxmax, maxim);

      double slmin = lower_line.slope;
      double rmin = lower_line.r_squared;
      double slmax = upper_line.slope;
      double rmax = upper_line.r_squared;

      // Python line 104: Check pattern conditions
      // Python defaults: slope_min=0.0001, slope_max=-0.0001, lower_ratio_slope=0.95, upper_ratio_slope=1.0
      constexpr double slope_min = 0.0001;
      constexpr double slope_max = -0.0001;
      constexpr double lower_ratio_slope = 0.95;
      constexpr double upper_ratio_slope = 1.0;

      if (std::abs(rmax) >= m_r_squared_min && std::abs(rmin) >= m_r_squared_min &&
          slmin >= slope_min && slmax <= slope_max &&
          std::abs(slmax / slmin) > lower_ratio_slope &&
          std::abs(slmax / slmin) < upper_ratio_slope) {

        // Pattern detected!
        bull_pennant[candle_idx] = true;
        slmax_result[candle_idx] = slmax;
        slmin_result[candle_idx] = slmin;
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("bull_pennant"), arrow::boolean()},
                       {GetOutputId("bear_pennant"), arrow::boolean()},
                       {GetOutputId("slmax"), arrow::float64()},
                       {GetOutputId("slmin"), arrow::float64()}}),
        {factory::array::make_array(bull_pennant),
         factory::array::make_array(bear_pennant),
         factory::array::make_array(slmax_result),
         factory::array::make_array(slmin_result)}));
  }

private:
  size_t m_lookback;
  size_t m_min_pivot_points;
  double m_r_squared_min;
  size_t m_max_duration;
};

} // namespace epochflow::transform
