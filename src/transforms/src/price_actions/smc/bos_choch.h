#pragma once

#include "epoch_metadata/transforms/itransform.h"

#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace epoch_metadata::transform {
constexpr double epsilon = 1e-8;

// Safe floating-point comparison function
inline bool is_close(double a, double b, double tol = epsilon) {
  return std::abs(a - b) < tol;
}

/**
 * bos_v / choch_v  – Break‑of‑Structure & Change‑of‑Character
 *
 * Preconditions:
 *   ‑ The incoming DataFrame *bars* MUST contain:
 *       1. standard OHLC columns (Open/High/Low/Close)
 *       2. integer column  "high_low"  (1  = swing‑high, −1 = swing‑low, null
 * else)
 *       3. double  column  "level"     (value of the swing, NaN if none)
 *
 * Options:
 *   close_break  (bool) – if true, use close‑price to detect the break;
 *                         otherwise use high / low.
 *
 * Outputs:
 *   bos_v          (int64)   1 = bullish bos_v, −1 = bearish bos_v, null else
 *   choch_v        (int64)   1 = bullish choch_v, −1 = bearish choch_v, null
 * else level        (double)  level associated with that bos_v / choch_v
 *   broken_index (int64)   index of candle that actually broke the level
 */
class BosChoch : public ITransform {
public:
  explicit BosChoch(const TransformConfiguration &cfg)
      : ITransform(cfg),
        m_close_break(cfg.GetOptionValue("close_break").GetBoolean()) {}

  [[nodiscard]]
  epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    return {bars.index(), Call(bars)};
  }

private:
  /* ------------------------------------------------------------------ */
  /*                            Implementation                          */
  /* ------------------------------------------------------------------ */

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;
    using arrow::float64;
    using arrow::int64;

    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    const Scalar null_d{NaN};

    /* --- column shortcuts ----------------------------------------- */
    const auto &C = epoch_metadata::EpochStratifyXConstants::instance();

    const auto open = bars[C.OPEN()].contiguous_array();
    const auto high = bars[C.HIGH()].contiguous_array();
    const auto low = bars[C.LOW()].contiguous_array();
    const auto close = bars[C.CLOSE()].contiguous_array();

    /* change these two strings if your column names differ */
    constexpr auto HL_COL = "high_low";
    constexpr auto LVL_COL = "level";

    const auto high_low = bars[GetInputId(HL_COL)].contiguous_array();
    const auto lvl_arr = bars[GetInputId(LVL_COL)].contiguous_array();

    const std::size_t N = static_cast<std::size_t>(bars.num_rows());

    /* --- working vectors ------------------------------------------ */
    std::vector<int64_t> bos_v(N, 0); //  1 / −1 / 0
    std::vector<int64_t> choch_v(N, 0);
    std::vector<double> level_v(N, 0);
    std::vector<int64_t> broken_v(N, 0);

    std::vector<double> level_order;
    std::vector<int64_t> highs_lows_order;
    std::vector<std::size_t> last_positions;

    level_order.reserve(N);
    highs_lows_order.reserve(N);
    last_positions.reserve(N);

    /* -------------------------------------------------------------- */
    /*        1. Detect bos_v / choch_v on the swing‑structure sequence   */
    /* -------------------------------------------------------------- */
    for (std::size_t i = 0; i < N; ++i) {
      if (high_low[i].is_null())
        continue;

      highs_lows_order.push_back(high_low[i].as_int64());
      level_order.push_back(lvl_arr[i].as_double());

      if (level_order.size() < 4) {
        last_positions.push_back(i);
        continue;
      }
      /* strict monotone helpers (no duplicates allowed) */
      auto strictly_increasing = [](double a, double b, double c, double d) {
        return (a < b) && (b < c) && (c < d);
      };

      auto strictly_decreasing = [](double a, double b, double c, double d) {
        return (a > b) && (b > c) && (c > d);
      };

      const double Lm4 = level_order.end()[-4];
      const double Lm3 = level_order.end()[-3];
      const double Lm2 = level_order.end()[-2];
      const double Lm1 = level_order.end()[-1];

      bool order1_strict_asc =
          strictly_increasing(Lm4, Lm2, Lm3, Lm1); //  –4  <  –2  <  –3  <  –1
      bool order1_strict_desc =
          strictly_decreasing(Lm4, Lm2, Lm3, Lm1); //  –4  >  –2  >  –3  >  –1
      bool order2_strict_asc =
          strictly_increasing(Lm1, Lm3, Lm4, Lm2); //  –1  <  –3  <  –4  <  –2
      bool order2_strict_desc =
          strictly_decreasing(Lm1, Lm3, Lm4, Lm2); //  –1  >  –3  >  –4  >  –2

      const auto idx_ref = last_positions.end()[-2]; // the candle we mark
      const std::size_t s = highs_lows_order.size();

      // look at last four swings
      const bool hl4_eq_seq1 =
          highs_lows_order[s - 4] == -1 && highs_lows_order[s - 3] == 1 &&
          highs_lows_order[s - 2] == -1 && highs_lows_order[s - 1] == 1;

      const bool hl4_eq_seq2 =
          highs_lows_order[s - 4] == 1 && highs_lows_order[s - 3] == -1 &&
          highs_lows_order[s - 2] == 1 && highs_lows_order[s - 1] == -1;

      /* --- bullish bos_v ----------------------------------------- */
      bos_v[idx_ref] = (hl4_eq_seq1 && order1_strict_asc) ? 1 : 0;
      level_v[idx_ref] = (bos_v[idx_ref] != 0) ? level_order.end()[-3] : 0;

      /* --- bearish bos_v ----------------------------------------- */
      bos_v[idx_ref] =
          (hl4_eq_seq2 && order1_strict_desc) ? -1 : bos_v[idx_ref];
      level_v[idx_ref] = (bos_v[idx_ref] != 0) ? level_order.end()[-3] : 0;

      /* --- bullish choch_v -------------------------------------- */
      choch_v[idx_ref] = (hl4_eq_seq1 && order2_strict_desc) ? 1 : 0;
      level_v[idx_ref] =
          (choch_v[idx_ref] != 0) ? level_order.end()[-3] : level_v[idx_ref];

      /* --- bearish choch_v -------------------------------------- */
      choch_v[idx_ref] =
          (hl4_eq_seq2 && order2_strict_asc) ? -1 : choch_v[idx_ref];
      level_v[idx_ref] =
          (choch_v[idx_ref] != 0) ? level_order.end()[-3] : level_v[idx_ref];
      last_positions.push_back(i);
    }

    /* -------------------------------------------------------------- */
    /*        2. Find the candle that actually breaks the level       */
    /* -------------------------------------------------------------- */
    const auto break_key_or_high = m_close_break ? C.CLOSE() : C.HIGH();
    const auto break_key_or_low = m_close_break ? C.CLOSE() : C.LOW();

    for (std::size_t i = 0; i < N; ++i) {
      if (bos_v[i] == 0 && choch_v[i] == 0)
        continue;

      /* build the mask */
      std::optional<int64_t> break_index{};
      if (bos_v[i] == 1 or choch_v[i] == 1) {
        auto y = bars[break_key_or_high]
                     .iloc({i + 2, std::nullopt})
                     .contiguous_array()
                     .to_view<double>();
        auto iter =
            std::find_if(y->begin(), y->end(), [lvl = level_v[i]](auto x) {
              return x && (*x > static_cast<float>(lvl));
            });
        if (iter != y->end()) {
          break_index = std::distance(y->begin(), iter);
        }
      } else if (bos_v[i] == -1 or choch_v[i] == -1) {
        auto y = bars[break_key_or_low]
                     .iloc({i + 2, std::nullopt})
                     .contiguous_array()
                     .to_view<double>();
        auto iter =
            std::find_if(y->begin(), y->end(), [lvl = level_v[i]](auto x) {
              return x && (*x < static_cast<float>(lvl));
            });
        if (iter != y->end()) {
          break_index = std::distance(y->begin(), iter);
        }
      }

      if (!break_index) {
        continue;
      }

      auto j = *break_index + i + 2;
      broken_v[i] = j;

      /* invalidate earlier bos_v/choch_v that survive past this break */
      for (std::size_t k = 0; k < N; ++k) {
        if ((bos_v[k] != 0 || choch_v[k] != 0) &&
            ((k < i) && broken_v[k] >= static_cast<int64_t>(j))) {
          bos_v[k] = 0;
          choch_v[k] = 0;
          level_v[k] = 0;
        }
      }
    }

    /* 3. Drop any structure that never broke ----------------------- */
    for (std::size_t i = 0; i < N; ++i) {
      if ((bos_v[i] != 0 || choch_v[i] != 0) && broken_v[i] == 0) {
        bos_v[i] = 0;
        choch_v[i] = 0;
        level_v[i] = 0;
      }
    }

    /* -------------------------------------------------------------- */
    /*                    4. Build Arrow arrays                       */
    /* -------------------------------------------------------------- */
    auto vec_to_arr = []<typename T>(const std::vector<T> &tmp) {
      typename arrow::CTypeTraits<T>::BuilderType builder;
      AssertStatusIsOk(builder.Reserve(tmp.size()));

      for (T v : tmp) {
        if constexpr (std::is_same_v<T, double>) {
          if (std::isnan(v) || v == 0) {
            builder.UnsafeAppendNull();
          } else {
            builder.UnsafeAppend(v);
          }
        } else {
          if (v == 0) {
            builder.UnsafeAppendNull();
          } else {
            builder.UnsafeAppend(v);
          }
        }
      }
      return AssertResultIsOk(builder.Finish());
    };

    Array bos_arr{vec_to_arr(bos_v)};
    Array choch_arr{vec_to_arr(choch_v)};
    Array level_arr{vec_to_arr(level_v)};
    Array broken_arr{vec_to_arr(broken_v)}; // 0 → null handled

    /* -------------------------------------------------------------- */
    /*                          Return table                          */
    auto schema =
        arrow::schema({{GetOutputId("bos"), arrow::int64()},
                       {GetOutputId("choch"), arrow::int64()},
                       {GetOutputId("level"), arrow::float64()},
                       {GetOutputId("broken_index"), arrow::int64()}});

    return AssertTableResultIsOk(
        arrow::Table::Make(schema, {bos_arr.value(), choch_arr.value(),
                                    level_arr.value(), broken_arr.value()}));
  }

  /* ------------------------------------------------------------------ */
  bool m_close_break;
};

} // namespace epoch_metadata::transform
