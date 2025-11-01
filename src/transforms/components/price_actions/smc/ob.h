#pragma once

#include <epochflow/transforms/core/itransform.h>

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <ranges>
#include <vector>

namespace epochflow::transform {

/**
 * OrderBlocks ‑ detects bullish and bearish order‑block zones.
 * The implementation is a C++ port of the reference Python algorithm, but the
 * logic is decomposed into small helpers so that each part can be unit‑tested
 * and reasoned about in isolation.
 */
class OrderBlocks : public ITransform {
  /* --------------------------------------------------------------------- */
  /*                            Public interface                           */
  /* --------------------------------------------------------------------- */
public:
  explicit OrderBlocks(const TransformConfiguration &cfg)
      : ITransform(cfg),
        m_close_mitigation(
            cfg.GetOptionValue("close_mitigation").GetBoolean()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &bars) const override {
    return {bars.index(), BuildTable(bars)};
  }

private:
  /* --------------------------------------------------------------------- */
  /*                               Aliases                                 */
  /* --------------------------------------------------------------------- */
  using index_t = std::size_t;
  using i64_vec = std::vector<std::int64_t>;
  using dbl_vec = std::vector<double>;

  /* --------------------------------------------------------------------- */
  /*                                State                                  */
  /* --------------------------------------------------------------------- */
  struct OBState {
    i64_vec ob{};                ///<  1 = bullish, ‑1 = bearish, 0 = none
    dbl_vec top{};               ///<  top of block
    dbl_vec bottom{};            ///<  bottom of block
    dbl_vec ob_volume{};         ///<  vol. of candle + 2 previous
    dbl_vec low_volume{};        ///<  helper
    dbl_vec high_volume{};       ///<  helper
    dbl_vec percentage{};        ///<  strength metric
    i64_vec mitigated_idx{};     ///< index at which block was mitigated
    std::vector<bool> breaker{}; ///<  breaker flag per candle

    explicit OBState(index_t n)
        : ob(n, 0), top(n, 0.0), bottom(n, 0.0), ob_volume(n, 0.0),
          low_volume(n, 0.0), high_volume(n, 0.0), percentage(n, 0.0),
          mitigated_idx(n, 0), breaker(n, false) {}
  };

  /* --------------------------------------------------------------------- */
  /*                         Helper: utils / math                          */
  /* --------------------------------------------------------------------- */
  [[nodiscard]] static inline double nan() {
    return std::numeric_limits<double>::quiet_NaN();
  }

  template <typename T>
  [[nodiscard]] static auto ToArrow(const std::vector<T> &v,
                                    const i64_vec &ob) {
    using namespace epoch_frame;
    // build arrow array with nulls where ob is 0
    using Builder = typename arrow::CTypeTraits<T>::BuilderType;
    Builder builder;
    AssertStatusIsOk(builder.Reserve(v.size()));
    for (auto const &[val, obs_ref] : std::views::zip(v, ob)) {
      (obs_ref == 0) ? builder.UnsafeAppendNull() : builder.UnsafeAppend(val);
    }
    return AssertResultIsOk(builder.Finish());
  }

  template <typename T>
  [[nodiscard]] static auto ToArrow(const std::vector<T> &v) {
    // For the ob array itself
    using namespace epoch_frame;
    using Builder = typename arrow::CTypeTraits<T>::BuilderType;
    Builder builder;
    AssertStatusIsOk(builder.Reserve(v.size()));
    for (auto val : v) {
      (val == 0) ? builder.UnsafeAppendNull() : builder.UnsafeAppend(val);
    }
    return AssertResultIsOk(builder.Finish());
  }

  /* --------------------------------------------------------------------- */
  /*                       Helper: swing‑index extraction                  */
  /* --------------------------------------------------------------------- */
  void ExtractSwingIndices(const arrow::Int64Array &high_low,
                           i64_vec &swing_high, i64_vec &swing_low) const {
    const auto N = high_low.length();
    swing_high.reserve(N / 2);
    swing_low.reserve(N / 2);
    for (index_t i = 0; i < static_cast<index_t>(N); ++i) {
      if (high_low.IsNull(i))
        continue;
      const auto v = static_cast<std::int64_t>(high_low.Value(i));
      if (v == 1) {
        swing_high.push_back(static_cast<std::int64_t>(i));
      } else if (v == -1) {
        swing_low.push_back(static_cast<std::int64_t>(i));
      }
    }
  }

  /* --------------------------------------------------------------------- */
  /*                 Helper: reset order‑block at given idx                */
  /* --------------------------------------------------------------------- */
  static void ResetOB(OBState &S, index_t idx) {
    S.ob[idx] = 0;
    S.top[idx] = S.bottom[idx] = S.ob_volume[idx] = S.percentage[idx] = 0.0;
    S.low_volume[idx] = S.high_volume[idx] = 0.0;
    S.mitigated_idx[idx] = 0;
    S.breaker[idx] = false;
  }

  /* --------------------------------------------------------------------- */
  /*         Helper: bullish / bearish order‑block processors              */
  /* --------------------------------------------------------------------- */
  template <bool is_bullish>
  void ProcessActiveOB(const index_t close_idx, const arrow::DoubleArray &open,
                       const arrow::DoubleArray &high,
                       const arrow::DoubleArray &low,
                       const arrow::DoubleArray &close, OBState &S,
                       i64_vec &active) const {
    /* Generic routine shared by bullish & bearish blocks */
    auto cmp_price = [&](double price, double level) {
      if constexpr (is_bullish) {
        return price < level;
      } else {
        return price > level;
      }
    };

    for (auto it = active.begin(); it != active.end();) {
      const auto idx = static_cast<index_t>(*it);
      if (S.breaker[idx]) {
        // block already mitigated – look for full invalidation
        bool invalidate = is_bullish ? (high.Value(close_idx) > S.top[idx])
                                     : (low.Value(close_idx) < S.bottom[idx]);
        if (invalidate) {
          ResetOB(S, idx);
          it = active.erase(it);
          continue;
        }
      } else {
        // check for mitigation
        if constexpr (is_bullish) {
          double price = m_close_mitigation ? std::min(open.Value(close_idx),
                                                       close.Value(close_idx))
                                            : low.Value(close_idx);

          if (cmp_price(price, S.bottom[idx])) {
            S.breaker[idx] = true;
            S.mitigated_idx[idx] = static_cast<std::int64_t>(close_idx - 1);
          }
        } else {
          double price = m_close_mitigation ? std::max(open.Value(close_idx),
                                                       close.Value(close_idx))
                                            : high.Value(close_idx);

          if (cmp_price(price, S.top[idx])) {
            S.breaker[idx] = true;
            S.mitigated_idx[idx] = static_cast<std::int64_t>(close_idx);
          }
        }
      }
      ++it;
    }
  }

  template <typename ArrowArrayT,
            typename CType = typename ArrowArrayT::value_type>
  static std::optional<int64_t>
  last_index_equal_to(const ArrowArrayT &arr, CType target,
                      int64_t offset_in_parent = 0) {
    for (int64_t i = arr.length() - 1; i >= 0; --i) {
      if (!arr.IsNull(i) && arr.Value(i) == target)
        return offset_in_parent + i;
    }
    return std::nullopt;
  }

  template <bool is_bullish>
  void
  TryCreateOB(const index_t close_idx, const arrow::DoubleArray &high,
              const arrow::DoubleArray &low, const arrow::DoubleArray &close,
              const arrow::DoubleArray &volume, const i64_vec &swings,
              std::vector<bool> &crossed, OBState &S, i64_vec &active) const {
    auto cmp_close_cross = [&](double c, double lvl) {
      return is_bullish ? (c > lvl) : (c < lvl);
    };
    auto cmp_seg_extreme = [&](const arrow::DoubleArray &seg) {
      std::span<const double> view{seg.raw_values(),
                                   static_cast<size_t>(seg.length())};
      return is_bullish ? std::ranges::min(view) : std::ranges::max(view);
    };

    /* Locate last swing point behind current candle */
    auto it =
        std::ranges::lower_bound(swings, static_cast<std::int64_t>(close_idx));
    if (it == swings.begin() || it == swings.end())
      return; // no prior swing
    const auto swing_idx = static_cast<index_t>(*(--it));

    const bool cond = cmp_close_cross(close.Value(close_idx),
                                      is_bullish ? high.Value(swing_idx)
                                                 : low.Value(swing_idx)) &&
                      !crossed[swing_idx];
    if (!cond)
      return; // no cross
    crossed[swing_idx] = true;

    /* Find default block (candle before current) */
    const auto default_idx = static_cast<index_t>(close_idx - 1);
    double ob_top, ob_bottom;
    index_t ob_idx = default_idx;

    if constexpr (is_bullish) {
      // For bullish blocks - note inconsistency in Python implementation
      ob_top = low.Value(default_idx);
      ob_bottom = high.Value(default_idx);
    } else {
      // For bearish blocks
      ob_top = high.Value(default_idx);
      ob_bottom = low.Value(default_idx);
    }

    /* Scan between swing and close for better extreme */
    if ((close_idx - swing_idx) > 1) {
      const auto start = swing_idx + 1;
      const auto end = close_idx;
      if (end > start) {
        const auto len = end - start;
        auto seg = is_bullish ? low.Slice(start, len) : high.Slice(start, len);
        auto seg_view = epoch_frame::PtrCast<arrow::DoubleArray>(seg);
        auto extreme = cmp_seg_extreme(*seg_view);
        auto last = last_index_equal_to(*seg_view, extreme);
        if (last) {
          ob_idx = start + *last;
          if constexpr (is_bullish) {
            // For bullish blocks - note inconsistency with the default
            // assignment
            ob_top = high.Value(ob_idx);
            ob_bottom = low.Value(ob_idx);
          } else {
            // For bearish blocks
            ob_top = high.Value(ob_idx);
            ob_bottom = low.Value(ob_idx);
          }
        }
      }
    }

    /* Fill state vectors */
    S.ob[ob_idx] = is_bullish ? 1 : -1;
    S.top[ob_idx] = ob_top;
    S.bottom[ob_idx] = ob_bottom;

    auto close_idx_2 = close_idx >= 2 ? close_idx - 2 : 0;
    auto close_idx_1 = close_idx >= 1 ? close_idx - 1 : 0;

    S.ob_volume[ob_idx] = volume.Value(close_idx) + volume.Value(close_idx_1) +
                          volume.Value(close_idx_2);

    if constexpr (is_bullish) {
      S.low_volume[ob_idx] = volume.Value(close_idx_2);
      S.high_volume[ob_idx] =
          volume.Value(close_idx) + volume.Value(close_idx_1);
    } else {
      S.low_volume[ob_idx] =
          volume.Value(close_idx) + volume.Value(close_idx_1);
      S.high_volume[ob_idx] = volume.Value(close_idx_2);
    }
    const double max_v = std::max(S.high_volume[ob_idx], S.low_volume[ob_idx]);
    S.percentage[ob_idx] =
        max_v == 0.0 ? 100.0
                     : (std::min(S.high_volume[ob_idx], S.low_volume[ob_idx]) /
                        max_v * 100.0);
    active.push_back(static_cast<std::int64_t>(ob_idx));
  }

  /* --------------------------------------------------------------------- */
  /*                          Main calculation                             */
  /* --------------------------------------------------------------------- */
  arrow::TablePtr BuildTable(const epoch_frame::DataFrame &bars) const {
    using namespace epoch_frame;
    const auto &C = epochflow::EpochStratifyXConstants::instance();

    /* Column shortcuts */
    const auto open = bars[C.OPEN()].contiguous_array().to_view<double>();
    const auto high = bars[C.HIGH()].contiguous_array().to_view<double>();
    const auto low = bars[C.LOW()].contiguous_array().to_view<double>();
    const auto close = bars[C.CLOSE()].contiguous_array().to_view<double>();
    const auto volume = bars[C.VOLUME()].contiguous_array().to_view<double>();

    /* Input columns coming from previous transform */
    const auto high_low =
        bars[GetInputId("high_low")].contiguous_array().to_view<int64_t>();

    const index_t N = static_cast<index_t>(bars.num_rows());

    /* Pre‑computed swing indices */
    i64_vec swing_high, swing_low;
    ExtractSwingIndices(*high_low, swing_high, swing_low);

    /* Working state */
    OBState S(N);
    std::vector<bool> crossed(N, false);
    i64_vec active_bullish, active_bearish;

    /* Main loop */
    for (index_t i = 0; i < N; ++i) {
      ProcessActiveOB<true>(i, *open, *high, *low, *close, S, active_bullish);
      ProcessActiveOB<false>(i, *open, *high, *low, *close, S, active_bearish);

      TryCreateOB<true>(i, *high, *low, *close, *volume, swing_high, crossed, S,
                        active_bullish);
      TryCreateOB<false>(i, *high, *low, *close, *volume, swing_low, crossed, S,
                         active_bearish);
    }

    /* Convert to Arrow arrays */
    auto ob_arr = ToArrow(S.ob);
    auto top_arr = ToArrow(S.top, S.ob);
    auto bottom_arr = ToArrow(S.bottom, S.ob);
    auto ob_volume_arr = ToArrow(S.ob_volume, S.ob);
    auto mitigated_arr = ToArrow(S.mitigated_idx, S.ob);
    auto percentage_arr = ToArrow(S.percentage, S.ob);

    auto schema = arrow::schema({
        {GetOutputId("ob"), arrow::int64()},
        {GetOutputId("top"), arrow::float64()},
        {GetOutputId("bottom"), arrow::float64()},
        {GetOutputId("ob_volume"), arrow::float64()},
        {GetOutputId("mitigated_index"), arrow::int64()},
        {GetOutputId("percentage"), arrow::float64()},
    });

    return AssertTableResultIsOk(arrow::Table::Make(
        schema, std::vector{ob_arr, top_arr, bottom_arr, ob_volume_arr,
                            mitigated_arr, percentage_arr}));
  }

  /* --------------------------------------------------------------------- */
  /*                               Members                                 */
  /* --------------------------------------------------------------------- */
  bool m_close_mitigation;
};

} // namespace epochflow::transform