#pragma once

#include "date_time/date_offsets.h"
#include "epoch_frame/factory/array_factory.h"
#include <epoch_script/transforms/core/itransform.h>

#include <epoch_script/core/bar_attribute.h>
#include <epoch_script/transforms/core/sessions_utils.h>
#include <algorithm>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/factory/table_factory.h>
#include <epoch_script/core/time_frame.h>
#include <limits>
#include <vector>

using namespace std::chrono_literals;

namespace epoch_script::transform {

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
 *   active          (bool)   true = active session, false = inactive session
 *   session_opened  (bool)   true when session transitions from inactive to
 * active, false otherwise session_closed  (bool)   true when session
 * transitions from active to inactive, false otherwise high           (float64)
 * session high value low            (float64) session low value
 */
class Sessions : public ITransform {
public:
  explicit Sessions(const TransformConfiguration &cfg,
                    epoch_frame::SessionRange time_range)
      : ITransform(cfg), m_time_range(std::move(time_range)) {}

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
    const auto &C = epoch_script::EpochStratifyXConstants::instance();

    const auto high_arr = bars[C.HIGH()].contiguous_array();
    const auto low_arr = bars[C.LOW()].contiguous_array();

    auto barsIndexUTC = bars.index()->tz_localize("UTC");

    auto [active, opened, closed] =
        sessions_utils::BuildActiveMaskUTC(barsIndexUTC, m_time_range);
    std::vector<double> high(bars.size());
    std::vector<double> low(bars.size());

    for (size_t i = 0; i < bars.size(); ++i) {

      if (active[i]) {
        high[i] = std::max(high_arr[i].as_double(), i > 0 ? high[i - 1] : 0);
        low[i] = std::min(low_arr[i].as_double(),
                          (i > 0 && low[i - 1] != 0)
                              ? low[i - 1]
                              : std::numeric_limits<double>::infinity());
      }
    }

    /* -------------------------------------------------------------- */
    /*                          Return table                          */
    auto schema = arrow::schema({{GetOutputId("active"), arrow::boolean()},
                                 {GetOutputId("opened"), arrow::boolean()},
                                 {GetOutputId("closed"), arrow::boolean()},
                                 {GetOutputId("high"), arrow::float64()},
                                 {GetOutputId("low"), arrow::float64()}});

    return AssertTableResultIsOk(arrow::Table::Make(
        schema,
        {factory::array::make_array(active), factory::array::make_array(opened),
         factory::array::make_array(closed), factory::array::make_array(high),
         factory::array::make_array(low)}));
  }

  /* ------------------------------------------------------------------ */
  epoch_frame::SessionRange m_time_range;
};

// inline epoch_frame::Time parse_time(std::string const &time_str) {
//   auto sep = time_str.find(":");
//   int hour, minute;
//   try {
//     hour = std::stoi(time_str.substr(0, sep));
//     minute = std::stoi(time_str.substr(sep + 1));
//   } catch (std::exception const &e) {
//     throw std::invalid_argument("Invalid time format: " + time_str);
//   }
//   return epoch_frame::Time(std::chrono::hours(hour),
//                            std::chrono::minutes(minute));
// }

// class CustomSessions : public ITransform {
// public:
//   explicit CustomSessions(const TransformConfiguration &cfg)
//       : ITransform(cfg, parse_time(cfg.GetOptionValue("start_time")),
//                    parse_time(cfg.GetOptionValue("end_time"))) {}
// };

class DefaultSessions : public Sessions {
public:
  explicit DefaultSessions(const TransformConfiguration &cfg)
      : Sessions(cfg, epoch_script::kSessionRegistry.at(
                          cfg.GetOptionValue("session_type")
                              .GetSelectOption<epoch_core::SessionType>())) {}
};

} // namespace epoch_script::transform
