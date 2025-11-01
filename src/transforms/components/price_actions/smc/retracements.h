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
 * Retracements - calculates percentage retracements from swing highs and lows.
 * This gives an indication of how much a price has retraced from a prior swing
 * point.
 */
class Retracements : public ITransform {
public:
  explicit Retracements(const TransformConfiguration &config)
      : ITransform(config) {}

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

  // Special version for direction array that preserves zeros
  [[nodiscard]] static auto ToArrowDirection(const std::vector<int64_t> &v) {
    using namespace epoch_frame;
    using Builder = arrow::Int64Builder;
    Builder builder;
    AssertStatusIsOk(builder.Reserve(v.size()));
    for (auto val : v) {
      builder.UnsafeAppend(val);
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

    // Initialize output arrays with zeros
    std::vector<int64_t> direction(N, 0);
    std::vector<double> current_retracement(N, 0.0);
    std::vector<double> deepest_retracement(N, 0.0);

    // Calculate retracements
    double top = 0.0;
    double bottom = 0.0;

    for (index_t i = 0; i < N; ++i) {
      if (!high_low->IsNull(i)) {
        if (high_low->Value(i) == 1) {
          // Swing high
          direction[i] = 1;
          top = level->Value(i);
        } else if (high_low->Value(i) == -1) {
          // Swing low
          direction[i] = -1;
          bottom = level->Value(i);
        } else {
          direction[i] = (i > 0) ? direction[i - 1] : 0;
        }
      } else {
        direction[i] = (i > 0) ? direction[i - 1] : 0;
      }

      // Calculate retracements based on direction
      if (i > 0 && direction[i - 1] == 1 && std::abs(top - bottom) > 1e-10) {
        // Bullish direction - retracing from a swing high
        double low_val = !low->IsNull(i) ? low->Value(i) : 0.0;
        current_retracement[i] =
            std::round(
                (100.0 - (((low_val - bottom) / (top - bottom)) * 100.0)) *
                10.0) /
            10.0;
        deepest_retracement[i] = std::max(
            (i > 0 && direction[i - 1] == 1) ? deepest_retracement[i - 1] : 0.0,
            current_retracement[i]);
      }

      if (direction[i] == -1 && std::abs(bottom - top) > 1e-10) {
        // Bearish direction - retracing from a swing low
        double high_val = !high->IsNull(i) ? high->Value(i) : 0.0;
        current_retracement[i] =
            std::round((100.0 - ((high_val - top) / (bottom - top)) * 100.0) *
                       10.0) /
            10.0;
        deepest_retracement[i] = std::max((i > 0 && direction[i - 1] == -1)
                                              ? deepest_retracement[i - 1]
                                              : 0.0,
                                          current_retracement[i]);
      }
    }

    // Roll arrays by 1 (shift forward)
    for (index_t i = N - 1; i > 0; --i) {
      direction[i] = direction[i - 1];
      current_retracement[i] = current_retracement[i - 1];
      deepest_retracement[i] = deepest_retracement[i - 1];
    }
    direction[0] = 0;
    current_retracement[0] = 0.0;
    deepest_retracement[0] = 0.0;

    // Remove first few retracements
    int remove_first_count = 0;
    for (index_t i = 0; i < N; ++i) {
      if (i + 1 == N)
        break;

      if (direction[i] != direction[i + 1]) {
        remove_first_count++;
      }

      direction[i] = 0;
      current_retracement[i] = 0.0;
      deepest_retracement[i] = 0.0;

      if (remove_first_count == 3) {
        direction[i + 1] = 0;
        current_retracement[i + 1] = 0.0;
        deepest_retracement[i + 1] = 0.0;
        break;
      }
    }

    // Convert to Arrow arrays
    auto direction_arr = ToArrowDirection(direction);
    auto current_arr = ToArrow(current_retracement);
    auto deepest_arr = ToArrow(deepest_retracement);

    auto schema = arrow::schema({
        {GetOutputId("direction"), arrow::int64()},
        {GetOutputId("current_retracement"), arrow::float64()},
        {GetOutputId("deepest_retracement"), arrow::float64()},
    });

    return AssertTableResultIsOk(arrow::Table::Make(
        schema, std::vector{direction_arr, current_arr, deepest_arr}));
  }
};
} // namespace epochflow::transform