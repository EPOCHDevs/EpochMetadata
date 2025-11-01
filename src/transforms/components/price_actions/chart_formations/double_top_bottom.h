#pragma once

#include "../infrastructure/flexible_pivot_detector.h"
#include "../infrastructure/pattern_validator.h"
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>

namespace epoch_script::transform {

/**
 * DoubleTopBottom - Direct port from Python doubles.py
 * Detects double top and double bottom reversal patterns
 *
 * Requires exactly 5 pivots in the lookback window
 */
class DoubleTopBottom : public ITransform {
public:
  explicit DoubleTopBottom(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_pattern_type(config.GetOptionValue("pattern_type").GetString()),
        m_similarity_tolerance(config.GetOptionValue("similarity_tolerance").GetDecimal()) {}

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

    std::vector<bool> pattern_detected(N, false);
    std::vector<double> breakout_level(N, nan);
    std::vector<double> target_price(N, nan);

    // Detect pivots (Python uses left_count=3, right_count=3 by default)
    std::vector<size_t> pivot_high_indices;
    std::vector<size_t> pivot_low_indices;
    std::vector<double> pivot_high_values;
    std::vector<double> pivot_low_values;

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

      if (is_pivot_high) {
        pivot_high_indices.push_back(i);
        pivot_high_values.push_back(high[i].as_double());
      }
      if (is_pivot_low) {
        pivot_low_indices.push_back(i);
        pivot_low_values.push_back(low[i].as_double());
      }
    }

    // Convert similarity_tolerance to Python ratio format
    // For tops: tolerance 0.01 → tops_max_ratio = 1.01
    // For bottoms: tolerance 0.02 → bottoms_min_ratio = 0.98
    const double tops_max_ratio = 1.0 + m_similarity_tolerance;
    const double bottoms_min_ratio = 1.0 - m_similarity_tolerance;

    // Pattern detection - direct port from Python lines 57-87
    for (size_t candle_idx = m_lookback; candle_idx < N; ++candle_idx) {
      // Python line 58: Get sub_ohlc window
      // Python line 60: Collect pivot indices where pivot != 0
      std::vector<size_t> pivot_indx;
      std::vector<double> pivot_pos;

      // Collect all pivots (both high and low) in the window
      // Python mixes highs and lows in order of appearance
      for (size_t i = candle_idx - m_lookback; i <= candle_idx && i < N; ++i) {
        // Check if i is a pivot high
        auto high_it = std::find(pivot_high_indices.begin(), pivot_high_indices.end(), i);
        if (high_it != pivot_high_indices.end()) {
          pivot_indx.push_back(i);
          size_t idx = std::distance(pivot_high_indices.begin(), high_it);
          pivot_pos.push_back(pivot_high_values[idx]);
        }
        // Check if i is a pivot low
        auto low_it = std::find(pivot_low_indices.begin(), pivot_low_indices.end(), i);
        if (low_it != pivot_low_indices.end()) {
          pivot_indx.push_back(i);
          size_t idx = std::distance(pivot_low_indices.begin(), low_it);
          pivot_pos.push_back(pivot_low_values[idx]);
        }
      }

      // Python line 61-62: Must have exactly 5 pivots
      if (pivot_indx.size() != 5)
        continue;

      // Python line 68-75: Double Tops detection (if/elif structure)
      if (m_pattern_type == "tops" || m_pattern_type == "both") {
        // Python line 69-71: Check pattern conditions
        if ((pivot_pos[0] < pivot_pos[1]) && (pivot_pos[0] < pivot_pos[3]) &&
            (pivot_pos[2] < pivot_pos[1]) && (pivot_pos[2] < pivot_pos[3]) &&
            (pivot_pos[4] < pivot_pos[1]) && (pivot_pos[4] < pivot_pos[3]) &&
            (pivot_pos[1] > pivot_pos[3]) &&
            (pivot_pos[1] / pivot_pos[3] <= tops_max_ratio)) {

          pattern_detected[candle_idx] = true;
          breakout_level[candle_idx] = pivot_pos[2]; // Middle trough
          // Target = trough - (peak - trough)
          target_price[candle_idx] = pivot_pos[2] - (pivot_pos[1] - pivot_pos[2]);
        }
      }
      // Python line 79-86: Double Bottoms detection (elif in Python)
      else if (m_pattern_type == "bottoms" || m_pattern_type == "both") {
        // Python line 80-82: Check pattern conditions
        if ((pivot_pos[0] > pivot_pos[1]) && (pivot_pos[0] > pivot_pos[3]) &&
            (pivot_pos[2] > pivot_pos[1]) && (pivot_pos[2] > pivot_pos[3]) &&
            (pivot_pos[4] > pivot_pos[1]) && (pivot_pos[4] > pivot_pos[3]) &&
            (pivot_pos[1] < pivot_pos[3]) &&
            (pivot_pos[1] / pivot_pos[3] >= bottoms_min_ratio)) {

          pattern_detected[candle_idx] = true;
          breakout_level[candle_idx] = pivot_pos[2]; // Middle peak
          // Target = peak + (peak - trough)
          target_price[candle_idx] = pivot_pos[2] + (pivot_pos[2] - pivot_pos[1]);
        }
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("pattern_detected"), arrow::boolean()},
                       {GetOutputId("breakout_level"), arrow::float64()},
                       {GetOutputId("target"), arrow::float64()}}),
        {factory::array::make_array(pattern_detected),
         factory::array::make_array(breakout_level),
         factory::array::make_array(target_price)}));
  }

private:
  size_t m_lookback;
  std::string m_pattern_type;
  double m_similarity_tolerance;
};

} // namespace epoch_script::transform
