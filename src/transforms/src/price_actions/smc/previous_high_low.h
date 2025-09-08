#pragma once

#include "../../bar_resampler.h"
#include "epoch_frame/factory/array_factory.h"
#include "epoch_frame/scalar.h"
#include "epoch_metadata/transforms/itransform.h"

#include <cstdint>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace epoch_metadata::transform {
/**
 * PreviousHighLow - identifies the previous high or low within a given
 * interval.
 */
class PreviousHighLow : public ITransform {
public:
  explicit PreviousHighLow(const TransformConfiguration &config)
      : ITransform(config), m_barResampler(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), BuildTable(df)};
  }

private:
  using index_t = std::size_t;
  BarResampler m_barResampler;

  arrow::TablePtr BuildTable(epoch_frame::DataFrame const &input) const {
    using namespace epoch_frame;
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();

    // Column shortcuts
    const auto N = input.num_rows();

    const auto high = input[C.HIGH()].contiguous_array().to_view<double>();
    const auto low = input[C.LOW()].contiguous_array().to_view<double>();

    auto resampled = m_barResampler.TransformData(input).drop_null();
    const auto resampled_index = resampled.index()->array();
    const auto resampled_high =
        resampled[C.HIGH()].contiguous_array().to_view<double>();
    const auto resampled_low =
        resampled[C.LOW()].contiguous_array().to_view<double>();

    std::vector<double> previous_high(N);
    std::vector<double> previous_low(N);
    std::vector<bool> broken_high(N);
    std::vector<bool> broken_low(N);

    bool currently_broken_high = false;
    bool currently_broken_low = false;
    uint64_t last_broken_time{static_cast<size_t>(-1)};

    for (index_t i = 0; i < N; ++i) {
      const auto resampled_previous_index =
          (resampled_index < input.index()->at(i)).where();
      if (resampled_previous_index.length() <= 1) {
        previous_high[i] = std::numeric_limits<double>::quiet_NaN();
        previous_low[i] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }

      auto resampled_previous_index_scalar =
          resampled_previous_index[-2].value<size_t>().value();

      if (last_broken_time != resampled_previous_index_scalar) {
        currently_broken_high = false;
        currently_broken_low = false;
        last_broken_time = resampled_previous_index_scalar;
      }

      previous_high[i] = resampled_high->Value(
          static_cast<int64_t>(resampled_previous_index_scalar));
      previous_low[i] = resampled_low->Value(
          static_cast<int64_t>(resampled_previous_index_scalar));

      currently_broken_high =
          (high->Value(i) > static_cast<float>(previous_high[i])) ||
          currently_broken_high;
      currently_broken_low =
          (low->Value(i) < static_cast<float>(previous_low[i])) ||
          currently_broken_low;

      broken_high[i] = currently_broken_high;
      broken_low[i] = currently_broken_low;
    }

    return arrow::Table::Make(
        arrow::schema({
            arrow::field(GetOutputId("previous_high"), arrow::float64()),
            arrow::field(GetOutputId("previous_low"), arrow::float64()),
            arrow::field(GetOutputId("broken_high"), arrow::boolean()),
            arrow::field(GetOutputId("broken_low"), arrow::boolean()),
        }),
        {factory::array::make_array(previous_high),
         factory::array::make_array(previous_low),
         factory::array::make_array(broken_high),
         factory::array::make_array(broken_low)});
  }
};
} // namespace epoch_metadata::transform