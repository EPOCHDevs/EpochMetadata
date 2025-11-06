#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/sessions_utils.h>
#include <epoch_script/core/bar_attribute.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include "date_time/date_offsets.h"
#include <regex>
#include <stdexcept>
#include <limits>
#include <algorithm>

namespace epoch_script::transform {

/**
 * TimeOfDay - Detects when bars fall within a custom UTC time range
 *
 * Returns true when a bar's timestamp falls within the specified start/end time range.
 * All times are in UTC.
 *
 * Example usage:
 *   morning = time_of_day(start="09:30", end="12:00")
 *   is_morning = morning.active
 */
class TimeOfDay : public ITransform {
public:
  explicit TimeOfDay(const TransformConfiguration &config)
      : ITransform(config),
        m_start_str(config.GetOptionValue("start").GetString()),
        m_end_str(config.GetOptionValue("end").GetString()),
        m_range(ParseTimeRange(m_start_str, m_end_str)) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;

    // Get column shortcuts
    const auto &C = epoch_script::EpochStratifyXConstants::instance();
    const auto high_arr = bars[C.HIGH()].contiguous_array();
    const auto low_arr = bars[C.LOW()].contiguous_array();

    // Get UTC index
    auto index = bars.index();
    auto utcIndex = index->tz_localize("UTC");

    // Build active/opened/closed masks using existing utility
    auto state = sessions_utils::BuildActiveMaskUTC(utcIndex, m_range);

    // Calculate period high/low (same logic as sessions)
    std::vector<double> high(bars.size());
    std::vector<double> low(bars.size());

    for (size_t i = 0; i < bars.size(); ++i) {
      if (state.active[i]) {
        high[i] = std::max(high_arr[i].as_double(), i > 0 ? high[i - 1] : 0);
        low[i] = std::min(low_arr[i].as_double(),
                          (i > 0 && low[i - 1] != 0)
                              ? low[i - 1]
                              : std::numeric_limits<double>::infinity());
      }
    }

    // Return all 5 outputs (matching sessions transform)
    auto schema = arrow::schema({
        {GetOutputId("active"), arrow::boolean()},
        {GetOutputId("opened"), arrow::boolean()},
        {GetOutputId("closed"), arrow::boolean()},
        {GetOutputId("high"), arrow::float64()},
        {GetOutputId("low"), arrow::float64()}
    });

    return AssertTableResultIsOk(arrow::Table::Make(
        schema,
        {factory::array::make_array(state.active),
         factory::array::make_array(state.opened),
         factory::array::make_array(state.closed),
         factory::array::make_array(high),
         factory::array::make_array(low)}));
  }

private:
  std::string m_start_str;
  std::string m_end_str;
  epoch_frame::SessionRange m_range;

  /**
   * Parse time string in format "HH:MM" or "HH:MM:SS" into Time object
   */
  static epoch_frame::Time ParseTime(const std::string &time_str) {
    // Regex for HH:MM or HH:MM:SS format
    std::regex time_regex(R"(^(\d{1,2}):(\d{2})(?::(\d{2}))?$)");
    std::smatch match;

    if (!std::regex_match(time_str, match, time_regex)) {
      throw std::runtime_error(
          "Invalid time format: '" + time_str +
          "'. Expected format: 'HH:MM' or 'HH:MM:SS'");
    }

    int hour = std::stoi(match[1]);
    int minute = std::stoi(match[2]);
    int second = match[3].matched ? std::stoi(match[3]) : 0;

    // Validate ranges
    if (hour < 0 || hour > 23) {
      throw std::runtime_error(
          "Hour must be 0-23, got: " + std::to_string(hour));
    }
    if (minute < 0 || minute > 59) {
      throw std::runtime_error(
          "Minute must be 0-59, got: " + std::to_string(minute));
    }
    if (second < 0 || second > 59) {
      throw std::runtime_error(
          "Second must be 0-59, got: " + std::to_string(second));
    }

    return epoch_frame::Time{
        std::chrono::hours(hour),
        std::chrono::minutes(minute),
        std::chrono::seconds(second),
        std::chrono::microseconds(0),
        "UTC"  // Always UTC as requested
    };
  }

  /**
   * Parse start and end time strings into a SessionRange
   */
  static epoch_frame::SessionRange ParseTimeRange(
      const std::string &start_str,
      const std::string &end_str) {

    auto start_time = ParseTime(start_str);
    auto end_time = ParseTime(end_str);

    return epoch_frame::SessionRange{start_time, end_time};
  }
};

} // namespace epoch_script::transform
