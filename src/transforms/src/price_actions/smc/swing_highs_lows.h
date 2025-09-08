#pragma once

#include "epoch_metadata/transforms/itransform.h"

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

namespace epoch_metadata::transform {
/**
    Swing Highs and Lows
    A swing high is when the current high is the highest high out of the
   swing_length amount of candles before and after. A swing low is when the
   current low is the lowest low out of the swing_length amount of candles
   before and after.
 */
class SwingHighsLows : public ITransform {
public:
  explicit SwingHighsLows(const TransformConfiguration &config)
      : ITransform(config),
        m_swing_length(config.GetOptionValue("swing_length").GetInteger() *
                       2UL) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    const Scalar null{nan};

    size_t N = bars.num_rows();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();

    const auto min_next_low =
        Series{low.shift(-(m_swing_length / 2)).value()}
            .to_frame()
            .rolling_agg({.window_size = static_cast<int64_t>(m_swing_length)})
            .min()
            .to_series()
            .contiguous_array();
    const auto max_next_high =
        Series{high.shift(-(m_swing_length / 2)).value()}
            .to_frame()
            .rolling_agg({.window_size = static_cast<int64_t>(m_swing_length)})
            .max()
            .to_series()
            .contiguous_array();

    auto replacement =
        Array{factory::array::make_contiguous_array(std::vector(N, -1L))}.where(
            low == min_next_low, Scalar{});
    auto swing_highs_lows =
        Array{factory::array::make_contiguous_array(std::vector(N, 1L))}.where(
            high == max_next_high, replacement);

    while (true) {
      auto pos = swing_highs_lows.is_not_null().where().to_vector<uint64_t>();
      const auto P = pos.size();
      if (P < 2)
        break;

      std::vector<bool> remove_flag(P, false);

      for (size_t k = 0; k < P - 1; ++k) {
        const auto cur = swing_highs_lows[pos[k]].as_int64();
        const auto nxt = swing_highs_lows[pos[k + 1]].as_int64();

        if (cur == 1 && nxt == 1) {
          const double h_cur = high[pos[k]].as_double();
          const double h_nxt = high[pos[k + 1]].as_double();
          if (h_cur < h_nxt)
            remove_flag[k] = true;
          else
            remove_flag[k + 1] = true;
        } else if (cur == -1 && nxt == -1) {
          const double l_cur = low[pos[k]].as_double();
          const double l_nxt = low[pos[k + 1]].as_double();
          if (l_cur > l_nxt)
            remove_flag[k] = true;
          else
            remove_flag[k + 1] = true;
        }
      }

      if (std::ranges::none_of(remove_flag, [](bool x) { return x; }))
        break;

      // rebuild array with nans where flagged
      auto shl_vec =
          swing_highs_lows.cast(arrow::float64()).to_vector<double>();
      for (size_t k = 0; k < P; ++k)
        if (remove_flag[k])
          shl_vec[pos[k]] = nan;

      swing_highs_lows =
          Array{factory::array::make_contiguous_array(shl_vec)}.cast(
              arrow::int64());
    }
    // -----------------------------------------------------------------

    // --- “pad” the ends safely --------------------------------------
    {
      auto shl_vec =
          swing_highs_lows.cast(arrow::float64()).to_vector<double>();
      std::vector<size_t> pos;
      for (int64_t i = 0; i < swing_highs_lows.length(); ++i)
        if (!std::isnan(shl_vec[i]))
          pos.push_back(static_cast<size_t>(i));

      if (!pos.empty()) {
        if (shl_vec[pos.front()] == 1)
          shl_vec[0] = -1.0;
        else
          shl_vec[0] = 1.0;

        if (shl_vec[pos.back()] == -1)
          shl_vec.back() = 1.0;
        else
          shl_vec.back() = -1.0;
      }
      swing_highs_lows =
          Array{factory::array::make_contiguous_array(shl_vec)}.cast(
              arrow::int64());
    }

    auto level = high.where(swing_highs_lows == Scalar{1L}, low);
    level = level.where(swing_highs_lows.is_not_null(), null);

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("high_low"), arrow::int64()},
                       {GetOutputId("level"), arrow::float64()}}),
        {swing_highs_lows.value(), level.value()}));
  }

private:
  size_t m_swing_length;
};
} // namespace epoch_metadata::transform