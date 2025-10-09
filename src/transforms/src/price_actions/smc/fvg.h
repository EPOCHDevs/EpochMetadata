#pragma once

#include "epoch_metadata/transforms/itransform.h"

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

namespace epoch_metadata::transform {
/**
 *  A fair value gap is when the previous high is lower than the next low if the
 current candle is bullish. Or when the previous low is higher than the next
 high if the current candle is bearish.
 */
class FairValueGap : public ITransform {
public:
  explicit FairValueGap(const TransformConfiguration &config)
      : ITransform(config),
        m_join_consecutive(
            config.GetOptionValue("join_consecutive").GetBoolean()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

private:
  bool m_join_consecutive;

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    const Scalar null{nan};

    const size_t N = bars.num_rows();
    const auto index = bars.index();
    const auto open = bars[C.OPEN()].contiguous_array();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();
    const auto close = bars[C.CLOSE()].contiguous_array();

    const auto is_close_gt_open = (close > open);
    const auto is_close_lt_open = (close < open);
    const auto prev_low = low.shift(1);
    const auto next_low = low.shift(-1);
    const auto prev_high = high.shift(1);
    const auto next_high = high.shift(-1);

    auto fvg =
        Array{factory::array::make_contiguous_array(std::vector(N, 1L))}.where(
            is_close_gt_open, Scalar{-1L});

    auto cond0 = (prev_high < next_low) && is_close_gt_open;
    auto cond1 = (prev_low > next_high) && is_close_lt_open;
    fvg = fvg.where(cond0 || cond1, Scalar{});

    auto top = next_low.where(is_close_gt_open, prev_low);
    top = top.where(fvg.is_not_null(), null);

    auto bottom = prev_high.where(is_close_gt_open, next_high);
    bottom = bottom.where(fvg.is_not_null(), null);

    if (m_join_consecutive) {
      std::vector<double> top_v = top.to_vector<double>();
      std::vector<double> bottom_v = bottom.to_vector<double>();
      std::vector<double> fvg_v =
          fvg.cast(arrow::float64()).to_vector<double>();

      for (int64_t i : std::views::iota(0L, fvg.length() - 1)) {
        if (fvg_v[i] == fvg_v[i + 1]) {
          top_v[i + 1] = std::max(top_v[i], top_v[i + 1]);
          bottom_v[i + 1] = std::min(bottom_v[i], bottom_v[i + 1]);
          fvg_v[i] = nan;
          top_v[i] = nan;
          bottom_v[i] = nan;
        }
      }
      fvg = Array{factory::array::make_contiguous_array(fvg_v)}.cast(
          arrow::int64());
      top = Array{factory::array::make_contiguous_array(top_v)};
      bottom = Array{factory::array::make_contiguous_array(bottom_v)};
    }

    std::vector mitigated_index(N, 0L);
    auto fvg_is_not_null = fvg.is_not_null();
    for (auto const &[i, is_valid] :
         std::views::enumerate(*fvg_is_not_null.to_view<bool>())) {
      const bool v = is_valid.value();
      if (!v) {
        continue;
      }

      auto mask = Array::FromVector(std::vector<bool>(N, false));
      if (fvg[i].as_int64() == 1) {
        mask = Array{low.value()->Slice(i + 2)} <= top[i];
      } else if (fvg[i].as_int64() == -1) {
        mask = Array{high.value()->Slice(i + 2)} >= bottom[i];
      }
      if (mask.any()) {
        auto j = mask.argmax() + i + 2;
        mitigated_index[i] = static_cast<int64_t>(j);
      }
    }

    auto mitigated_index_array =
        Array{factory::array::make_array(mitigated_index)}.where(
            fvg_is_not_null, Scalar{});

    // Build ArrayVector safely - .value() extracts underlying shared_ptr
    arrow::ArrayVector column_arrays;
    column_arrays.reserve(4);
    column_arrays.push_back(fvg.value());
    column_arrays.push_back(top.value());
    column_arrays.push_back(bottom.value());
    column_arrays.push_back(mitigated_index_array.value());

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("fvg"), arrow::int64()},
                       {GetOutputId("top"), arrow::float64()},
                       {GetOutputId("bottom"), arrow::float64()},
                       {GetOutputId("mitigated_index"), arrow::int64()}}),
        column_arrays));
  }
};
} // namespace epoch_metadata::transform