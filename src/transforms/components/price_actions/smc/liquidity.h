#pragma once

#include <epochflow/transforms/core/itransform.h>

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace epochflow::transform {
/**
 * Liquidity - identifies clusters of swing highs or swing lows that are close
 * to each other. Liquidity is when there are multiple highs within a small
 * range of each other, or multiple lows within a small range of each other.
 */
class Liquidity : public ITransform {
public:
  explicit Liquidity(const TransformConfiguration &config)
      : ITransform(config),
        m_range_percent(config.GetOptionValue("range_percent").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), BuildTable(df)};
  }

private:
  using index_t = std::size_t;

  template <typename T>
  [[nodiscard]] static auto ToArrow(const std::vector<T> &v) {
    using namespace epoch_frame;
    using Builder = typename arrow::CTypeTraits<T>::BuilderType;
    Builder builder;
    AssertStatusIsOk(builder.Reserve(v.size()));
    for (auto val : v) {
      if constexpr (std::is_floating_point_v<T>) {
        std::isnan(val) ? builder.UnsafeAppendNull()
                        : builder.UnsafeAppend(val);
      } else {
        (val == 0) ? builder.UnsafeAppendNull() : builder.UnsafeAppend(val);
      }
    }
    return AssertResultIsOk(builder.Finish());
  }

  arrow::TablePtr BuildTable(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    const auto &C = epochflow::EpochStratifyXConstants::instance();

    // Column shortcuts
    const auto high = bars[C.HIGH()].contiguous_array().to_view<double>();
    const auto low = bars[C.LOW()].contiguous_array().to_view<double>();
    const auto high_low =
        bars[GetInputId("high_low")].contiguous_array().to_view<int64_t>();
    const auto level =
        bars[GetInputId("level")].contiguous_array().to_view<double>();

    const index_t N = static_cast<index_t>(bars.num_rows());

    // Calculate the pip range based on the overall high-low range
    double max_high = std::numeric_limits<double>::lowest();
    double min_low = std::numeric_limits<double>::max();
    for (index_t i = 0; i < N; ++i) {
      if (!high->IsNull(i))
        max_high = std::max(max_high, high->Value(i));
      if (!low->IsNull(i))
        min_low = std::min(min_low, low->Value(i));
    }
    const double pip_range = (max_high - min_low) * m_range_percent;

    // Initialize output arrays with NaN
    std::vector<double> liquidity(N, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> liquidity_level(
        N, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> liquidity_end(N,
                                      std::numeric_limits<double>::quiet_NaN());
    std::vector<double> liquidity_swept(
        N, std::numeric_limits<double>::quiet_NaN());

    // Copy high_low and level to allow marking used candidates
    std::vector<int64_t> shl_HL(N);
    std::vector<double> shl_Level(N);
    for (index_t i = 0; i < N; ++i) {
      shl_HL[i] = high_low->IsNull(i) ? 0 : high_low->Value(i);
      shl_Level[i] = level->IsNull(i) ? 0.0 : level->Value(i);
    }

    // Process bullish liquidity (HighLow == 1)
    std::vector<index_t> bull_indices;
    for (index_t i = 0; i < N; ++i) {
      if (!high_low->IsNull(i) && high_low->Value(i) == 1) {
        bull_indices.push_back(i);
      }
    }

    for (auto i : bull_indices) {
      // Skip if this candidate has already been used
      if (shl_HL[i] != 1)
        continue;

      double high_level = shl_Level[i];
      double range_low = high_level - pip_range;
      double range_high = high_level + pip_range;
      std::vector<double> group_levels = {high_level};
      index_t group_end = i;

      // Determine the swept index
      index_t swept = 0;
      index_t c_start = i + 1;
      if (c_start < N) {
        for (index_t j = c_start; j < N; ++j) {
          if (!high->IsNull(j) && high->Value(j) >= range_high) {
            swept = j;
            break;
          }
        }
      }

      // Find other swing highs within range
      for (auto j : bull_indices) {
        if (j <= i)
          continue;
        if (swept && j >= swept)
          break;

        if (shl_HL[j] == 1 && range_low <= shl_Level[j] &&
            shl_Level[j] <= range_high) {
          group_levels.push_back(shl_Level[j]);
          group_end = j;
          shl_HL[j] = 0; // mark as used
        }
      }

      // Record if more than one candidate is grouped
      if (group_levels.size() > 1) {
        double avg_level =
            std::accumulate(group_levels.begin(), group_levels.end(), 0.0) /
            group_levels.size();
        liquidity[i] = 1.0;
        liquidity_level[i] = avg_level;
        liquidity_end[i] = static_cast<double>(group_end);
        liquidity_swept[i] = static_cast<double>(swept);
      }
    }

    // Process bearish liquidity (HighLow == -1)
    std::vector<index_t> bear_indices;
    for (index_t i = 0; i < N; ++i) {
      if (!high_low->IsNull(i) && high_low->Value(i) == -1) {
        bear_indices.push_back(i);
      }
    }

    for (auto i : bear_indices) {
      // Skip if this candidate has already been used
      if (shl_HL[i] != -1)
        continue;

      double low_level = shl_Level[i];
      double range_low = low_level - pip_range;
      double range_high = low_level + pip_range;
      std::vector<double> group_levels = {low_level};
      index_t group_end = i;

      // Determine the swept index
      index_t swept = 0;
      index_t c_start = i + 1;
      if (c_start < N) {
        for (index_t j = c_start; j < N; ++j) {
          if (!low->IsNull(j) && low->Value(j) <= range_low) {
            swept = j;
            break;
          }
        }
      }

      // Find other swing lows within range
      for (auto j : bear_indices) {
        if (j <= i)
          continue;
        if (swept && j >= swept)
          break;

        if (shl_HL[j] == -1 && range_low <= shl_Level[j] &&
            shl_Level[j] <= range_high) {
          group_levels.push_back(shl_Level[j]);
          group_end = j;
          shl_HL[j] = 0; // mark as used
        }
      }

      // Record if more than one candidate is grouped
      if (group_levels.size() > 1) {
        double avg_level =
            std::accumulate(group_levels.begin(), group_levels.end(), 0.0) /
            group_levels.size();
        liquidity[i] = -1.0;
        liquidity_level[i] = avg_level;
        liquidity_end[i] = static_cast<double>(group_end);
        liquidity_swept[i] = static_cast<double>(swept);
      }
    }

    // Convert to Arrow arrays
    auto liquidity_arr = ToArrow(liquidity);
    auto level_arr = ToArrow(liquidity_level);
    auto end_arr = ToArrow(liquidity_end);
    auto swept_arr = ToArrow(liquidity_swept);

    auto schema = arrow::schema({
        {GetOutputId("liquidity"), arrow::float64()},
        {GetOutputId("level"), arrow::float64()},
        {GetOutputId("end"), arrow::float64()},
        {GetOutputId("swept"), arrow::float64()},
    });

    return AssertTableResultIsOk(arrow::Table::Make(
        schema, std::vector{liquidity_arr, level_arr, end_arr, swept_arr}));
  }

  double m_range_percent;
};
} // namespace epochflow::transform