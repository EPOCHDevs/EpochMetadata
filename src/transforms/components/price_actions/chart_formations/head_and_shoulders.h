#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

namespace epoch_script::transform {

/**
 * HeadAndShoulders - Detects classic bearish reversal pattern
 *
 * Pattern structure: Left Shoulder - Head - Right Shoulder
 * - Three successive peaks (highs)
 * - Middle peak (head) is highest
 * - Two shoulders at approximately same height
 * - Neckline connects two troughs between peaks
 * - Bearish signal when price breaks below neckline
 *
 * Direct port from Python chart_patterns library
 */
class HeadAndShoulders : public ITransform {
public:
  explicit HeadAndShoulders(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_pivot_interval(10), // Python default
        m_short_pivot_interval(5), // Python default
        m_head_ratio_before(
            config.GetOptionValue("head_ratio_before").GetDecimal()),
        m_head_ratio_after(
            config.GetOptionValue("head_ratio_after").GetDecimal()),
        m_neckline_slope_max(
            config.GetOptionValue("neckline_slope_max").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    using namespace pattern_utils;
    const auto &C = epoch_script::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    const size_t N = bars.num_rows();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();

    // Python line 65-66: Find pivot points with TWO levels
    // Main pivots: pivot_interval=10
    std::vector<int64_t> pivot(N, 0);
    for (size_t i = m_pivot_interval; i < N - m_pivot_interval; ++i) {
      bool is_pivot_high = true;
      bool is_pivot_low = true;

      for (size_t j = i - m_pivot_interval; j <= i + m_pivot_interval; ++j) {
        if (j == i) continue;
        if (high[i].as_double() < high[j].as_double())
          is_pivot_high = false;
        if (low[i].as_double() > low[j].as_double())
          is_pivot_low = false;
      }

      if (is_pivot_high && is_pivot_low) pivot[i] = 3;
      else if (is_pivot_low) pivot[i] = 1;
      else if (is_pivot_high) pivot[i] = 2;
    }

    // Short pivots: short_pivot_interval=5
    std::vector<int64_t> short_pivot(N, 0);
    for (size_t i = m_short_pivot_interval; i < N - m_short_pivot_interval; ++i) {
      bool is_pivot_high = true;
      bool is_pivot_low = true;

      for (size_t j = i - m_short_pivot_interval; j <= i + m_short_pivot_interval; ++j) {
        if (j == i) continue;
        if (high[i].as_double() < high[j].as_double())
          is_pivot_high = false;
        if (low[i].as_double() > low[j].as_double())
          is_pivot_low = false;
      }

      if (is_pivot_high && is_pivot_low) short_pivot[i] = 3;
      else if (is_pivot_low) short_pivot[i] = 1;
      else if (is_pivot_high) short_pivot[i] = 2;
    }

    // Pattern detection results
    std::vector<bool> pattern_detected(N, false);
    std::vector<double> neckline_level(N, nan);
    std::vector<double> target_price(N, nan);

    // Python line 73: Scan for Head & Shoulders pattern
    for (size_t candle_idx = m_lookback; candle_idx < N; ++candle_idx) {
      // Python line 75-76: Must be pivot high on BOTH levels
      if (pivot[candle_idx] != 2 || short_pivot[candle_idx] != 2) {
        continue;
      }

      // Python line 79: find_points - collect pivots in centered window
      // Window: candle_idx - half_lookback to candle_idx + half_lookback
      const size_t half_lookback = m_lookback / 2;
      const size_t idx = candle_idx - half_lookback;

      if (idx < half_lookback || idx + half_lookback >= N) {
        continue;
      }

      std::vector<double> maxim;
      std::vector<double> minim;
      std::vector<size_t> xxmax;
      std::vector<size_t> xxmin;
      size_t maxbcount = 0, minbcount = 0, maxacount = 0, minacount = 0;

      // Python lines 42-56: Collect short pivots in window
      for (size_t i = idx - half_lookback; i <= idx + half_lookback && i < N; ++i) {
        if (short_pivot[i] == 1) {
          minim.push_back(low[i].as_double());
          xxmin.push_back(i);
          if (i < idx) minbcount++;
          else if (i > idx) minacount++;
        }
        if (short_pivot[i] == 2) {
          maxim.push_back(high[i].as_double());
          xxmax.push_back(i);
          if (i < idx) maxbcount++;
          else if (i > idx) maxacount++;
        }
      }

      // Python line 81-82: Check minimum counts
      if (minbcount < 1 || minacount < 1 || maxbcount < 1 || maxacount < 1) {
        continue;
      }

      // Python line 84-85: Calculate neckline slope
      auto neckline_reg = PatternValidator::calculate_linear_regression(
          std::vector<double>(xxmin.begin(), xxmin.end()),
          minim);
      double slmin = neckline_reg.slope;

      // Python line 85: Find head (argmax)
      auto [max_val, headidx] = PatternValidator::find_max_with_index(maxim);

      // Python line 88-89: Head can't be last
      if (headidx == maxim.size() - 1) {
        continue;
      }

      // Python line 92-94: Validate H&S structure
      if (maxim[headidx] - maxim[headidx - 1] > 0 &&
          maxim[headidx] / maxim[headidx - 1] > m_head_ratio_before &&
          maxim[headidx] - maxim[headidx + 1] > 0 &&
          maxim[headidx] / maxim[headidx + 1] > m_head_ratio_after &&
          std::abs(slmin) <= m_neckline_slope_max &&
          xxmin[0] > xxmax[headidx - 1] &&
          xxmin[1] < xxmax[headidx + 1]) {

        // Pattern detected!
        pattern_detected[candle_idx] = true;
        neckline_level[candle_idx] = minim[1]; // Right neckline point

        // Calculate target: Head height projected below neckline
        double head_height = maxim[headidx] - neckline_level[candle_idx];
        target_price[candle_idx] = neckline_level[candle_idx] - head_height;
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({
            {GetOutputId("pattern_detected"), arrow::boolean()},
            {GetOutputId("neckline_level"), arrow::float64()},
            {GetOutputId("target"), arrow::float64()}
        }),
        {
            factory::array::make_array(pattern_detected),
            factory::array::make_array(neckline_level),
            factory::array::make_array(target_price)
        }));
  }

private:
  size_t m_lookback;
  size_t m_pivot_interval;
  size_t m_short_pivot_interval;
  double m_head_ratio_before;
  double m_head_ratio_after;
  double m_neckline_slope_max;
};

} // namespace epoch_script::transform

