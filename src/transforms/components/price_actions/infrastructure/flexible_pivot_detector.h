#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>
#include <limits>

namespace epoch_script::transform {

/**
 * FlexiblePivotDetector - Detects pivot points (local highs/lows) with
 * asymmetric lookback support.
 *
 * Algorithm based on chart_patterns library:
 * - A pivot high is when the current high is the highest in left_count bars
 * before AND right_count bars after
 * - A pivot low is when the current low is the lowest in left_count bars before
 * AND right_count bars after
 *
 * Returns:
 * - pivot_type: 0=none, 1=pivot_low, 2=pivot_high, 3=both
 * - pivot_level: price level at pivot (high or low)
 */
class FlexiblePivotDetector : public ITransform {
public:
  explicit FlexiblePivotDetector(const TransformConfiguration &config)
      : ITransform(config),
        m_left_count(config.GetOptionValue("left_count").GetInteger()),
        m_right_count(config.GetOptionValue("right_count").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    const auto &C = epoch_script::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    const size_t N = bars.num_rows();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();

    std::vector<int64_t> pivot_type(N, 0);
    std::vector<double> pivot_level(N, nan);
    std::vector<int64_t> pivot_index(N, -1);

    // Process each candle
    for (size_t i = 0; i < N; ++i) {
      // Check boundary conditions
      if (i < m_left_count || i + m_right_count >= N) {
        continue;
      }

      bool is_pivot_high = true;
      bool is_pivot_low = true;

      const double current_high = high[i].as_double();
      const double current_low = low[i].as_double();

      // Check left and right bars
      for (size_t j = i - m_left_count; j <= i + m_right_count; ++j) {
        if (j == i) continue;

        const double check_high = high[j].as_double();
        const double check_low = low[j].as_double();

        if (current_high < check_high) {
          is_pivot_high = false;
        }
        if (current_low > check_low) {
          is_pivot_low = false;
        }
      }

      // Set pivot type
      if (is_pivot_high && is_pivot_low) {
        pivot_type[i] = 3; // Both
        pivot_level[i] = current_high; // Use high for plotting
      } else if (is_pivot_high) {
        pivot_type[i] = 2;
        pivot_level[i] = current_high;
      } else if (is_pivot_low) {
        pivot_type[i] = 1;
        pivot_level[i] = current_low;
      }

      if (pivot_type[i] != 0) {
        pivot_index[i] = static_cast<int64_t>(i);
      }
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({
            {GetOutputId("pivot_type"), arrow::int64()},
            {GetOutputId("pivot_level"), arrow::float64()},
            {GetOutputId("pivot_index"), arrow::int64()}
        }),
        {
            factory::array::make_array(pivot_type),
            factory::array::make_array(pivot_level),
            factory::array::make_array(pivot_index)
        }));
  }

private:
  size_t m_left_count;
  size_t m_right_count;
};

} // namespace epoch_script::transform
