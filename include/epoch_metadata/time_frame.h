//
// Created by adesola on 1/4/25.
//

#pragma once
#include "date_time/date_offsets.h"
#include "epoch_frame/day_of_week.h"
#include "epoch_frame/time_delta.h"
#include <epoch_core/enum_wrapper.h>
#include <epoch_frame/factory/date_offset_factory.h>
#include <glaze/json/json_t.hpp>
#include <string_view>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

// String constants for JSON/YAML keys and common values.
namespace epoch_metadata::tf_str {
inline constexpr std::string_view kType = "type";
inline constexpr std::string_view kInterval = "interval";
inline constexpr std::string_view kAnchor = "anchor";
inline constexpr std::string_view kWeekOfMonth = "week_of_month";
inline constexpr std::string_view kWeekday = "weekday";
inline constexpr std::string_view kMonth = "month";
inline constexpr std::string_view kTimeOffset = "time_offset";
inline constexpr std::string_view kSession = "session";
inline constexpr std::string_view kSessionAnchor = "session_anchor";

// Time offset component fields
inline constexpr std::string_view kDays = "days";
inline constexpr std::string_view kHours = "hours";
inline constexpr std::string_view kMinutes = "minutes";
inline constexpr std::string_view kSeconds = "seconds";
inline constexpr std::string_view kMilliseconds = "milliseconds";
inline constexpr std::string_view kMicroseconds = "microseconds";
inline constexpr std::string_view kWeeks = "weeks";

// Common values
inline constexpr std::string_view kAnchorStart = "Start";
inline constexpr std::string_view kAnchorEnd = "End";
inline constexpr std::string_view kNull = "Null";

// Common timeframe mapping keys
inline constexpr std::string_view k1Min = "1Min";
inline constexpr std::string_view k2Min = "2Min";
inline constexpr std::string_view k3Min = "3Min";
inline constexpr std::string_view k5Min = "5Min";
inline constexpr std::string_view k10Min = "10Min";
inline constexpr std::string_view k15Min = "15Min";
inline constexpr std::string_view k30Min = "30Min";
inline constexpr std::string_view k45Min = "45Min";
inline constexpr std::string_view k1H = "1H";
inline constexpr std::string_view k2H = "2H";
inline constexpr std::string_view k3H = "3H";
inline constexpr std::string_view k4H = "4H";
inline constexpr std::string_view k1W_SUN = "1W-SUN";
inline constexpr std::string_view k1W_MON = "1W-MON";
inline constexpr std::string_view k1W_FRI = "1W-FRI";
inline constexpr std::string_view k1W_MON_1st = "1W-MON-1st";
inline constexpr std::string_view k1W_MON_2nd = "1W-MON-2nd";
inline constexpr std::string_view k1W_MON_3rd = "1W-MON-3rd";
inline constexpr std::string_view k1W_FRI_Last = "1W-FRI-Last";
inline constexpr std::string_view k1D = "1D";
inline constexpr std::string_view k1ME = "1ME";
inline constexpr std::string_view k1MS = "1MS";
inline constexpr std::string_view k1QE = "1QE";
inline constexpr std::string_view k1QS = "1QS";
inline constexpr std::string_view k1YE = "1YE";
inline constexpr std::string_view k1YS = "1YS";
} // namespace epoch_metadata::tf_str

CREATE_ENUM(StratifyxMonth, jan, feb, mar, apr, may, jun, jul, aug, sep, oct,
            nov, dec);
CREATE_ENUM(StratifyxTimeFrameType, minute, hour, day, week, month, bmonth,
            quarter, year, bday, session, week_of_month);
CREATE_ENUM(WeekOfMonth, First, Second, Third, Fourth, Last);
CREATE_ENUM(StratifyxBarType, TickBar, VolumeBar, DollarBar, TickImbalanceBar,
            VolumeImbalanceBar, DollarImbalanceBar, TimeBar);

CREATE_ENUM(AnchoredTimeFrameType, Start, End);

// Sessions supported by session-anchored offsets. Maps to concrete
// SessionRange.
CREATE_ENUM(SessionType, Sydney, Tokyo, London, NewYork, AsianKillZone,
            LondonOpenKillZone, NewYorkKillZone, LondonCloseKillZone);

CREATE_ENUM(SessionAnchorType, AfterOpen, BeforeClose);

struct DateOffsetOption {
  epoch_core::StratifyxTimeFrameType type{
      epoch_core::StratifyxTimeFrameType::Null};
  uint32_t interval{0};
  epoch_core::AnchoredTimeFrameType anchor{
      epoch_core::AnchoredTimeFrameType::Null};
  epoch_core::WeekOfMonth week_of_month{epoch_core::WeekOfMonth::Null};
  epoch_core::EpochDayOfWeek weekday{epoch_core::EpochDayOfWeek::Null};
  epoch_core::StratifyxMonth month{epoch_core::StratifyxMonth::Null};
  std::optional<epoch_frame::TimeDelta> time_offset{std::nullopt};
  // Session-anchored `
  epoch_core::SessionType session{epoch_core::SessionType::Null};
  epoch_core::SessionAnchorType session_anchor{
      epoch_core::SessionAnchorType::Null};
};

namespace epoch_metadata {

bool IsIntraday(epoch_core::EpochOffsetType);

class TimeFrame {
public:
  explicit TimeFrame(epoch_frame::DateOffsetHandlerPtr offset);
  explicit TimeFrame(std::string mapping_key);

  bool IsIntraDay() const;

  std::string ToString() const;

  [[nodiscard]] epoch_frame::DateOffsetHandlerPtr GetOffset() const {
    return m_offset;
  }

  bool operator==(TimeFrame const &other) const;

  bool operator!=(TimeFrame const &other) const;

  bool operator<(TimeFrame const &other) const;

  std::string Serialize() const;

  // Public interface to check original construction
  [[nodiscard]] bool WasCreatedFromString() const {
    return m_created_from_string;
  }
  [[nodiscard]] const std::string &SourceString() const {
    return m_mapping_key;
  }

private:
  epoch_frame::DateOffsetHandlerPtr m_offset;
  bool m_created_from_string{false};
  std::string m_mapping_key{};
};

TimeFrame CreateTimeFrameFromYAML(YAML::Node const &);

epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandlerFromJSON(glz::json_t const &);
glz::json_t
CreateDateOffsetHandlerJSON(epoch_frame::DateOffsetHandlerPtr const &);

epoch_frame::RelativeDelta CreateRelativeDeltaFromJSON(glz::json_t const &);
glz::json_t CreateRelativeDeltaJSON(epoch_frame::RelativeDelta const &);

struct TimeFrameHash {
  size_t operator()(TimeFrame const &timeframe) const {
    return std::hash<std::string>()(timeframe.ToString());
  }
};

using TimeFrameSet = std::unordered_set<TimeFrame, TimeFrameHash>;

template <class T>
using TimeFrameHashMap = std::unordered_map<TimeFrame, T, TimeFrameHash>;
} // namespace epoch_metadata

namespace glz {
template <> struct meta<epoch_frame::DateOffsetHandlerPtr> {
  static constexpr auto read = [](epoch_frame::DateOffsetHandlerPtr &x,
                                  const json_t &input) {
    x = epoch_metadata::CreateDateOffsetHandlerFromJSON(input);
  };

  static constexpr auto write =
      [](const epoch_frame::DateOffsetHandlerPtr &x) -> auto {
    return epoch_metadata::CreateDateOffsetHandlerJSON(x);
  };

  static constexpr auto value = glz::custom<read, write>;
};

template <> struct meta<epoch_metadata::TimeFrame> {
  static constexpr auto read = [](epoch_metadata::TimeFrame &x,
                                  const glz::json_t &input) {
    if (input.is_string()) {
      x = epoch_metadata::TimeFrame(input.as<std::string>());
      return;
    }
    auto offset = epoch_metadata::CreateDateOffsetHandlerFromJSON(input);
    if (offset) {
      x = epoch_metadata::TimeFrame(offset);
    }
  };

  static constexpr auto write = [](const epoch_metadata::TimeFrame &x) -> auto {
    if (x.WasCreatedFromString()) {
      glz::json_t out;
      out = x.SourceString();
      return out;
    }
    return epoch_metadata::CreateDateOffsetHandlerJSON(x.GetOffset());
  };

  static constexpr auto value = glz::custom<read, write>;
};

template <> struct meta<std::optional<epoch_metadata::TimeFrame>> {
  static constexpr auto read = [](std::optional<epoch_metadata::TimeFrame> &x,
                                  const glz::json_t &input) {
    if (input.is_null()) {
      x = std::nullopt;
    } else {
      auto offset = epoch_metadata::CreateDateOffsetHandlerFromJSON(input);
      if (offset) {
        x = epoch_metadata::TimeFrame(offset);
      } else {
        x = std::nullopt;
      }
    }
  };

  static constexpr auto write =
      [](const std::optional<epoch_metadata::TimeFrame> &x) -> auto {
    if (x) {
      return epoch_metadata::CreateDateOffsetHandlerJSON(x->GetOffset());
    }
    return glz::json_t{};
  };

  static constexpr auto value = glz::custom<read, write>;
};
} // namespace glz

namespace YAML {

template <> struct convert<DateOffsetOption> {
  static bool decode(const Node &node, DateOffsetOption &rhs);
};

template <> struct convert<epoch_frame::DateOffsetHandlerPtr> {
  static bool decode(const Node &node, epoch_frame::DateOffsetHandlerPtr &rhs);
};

template <> struct convert<epoch_metadata::TimeFrame> {
  static bool decode(const Node &node, epoch_metadata::TimeFrame &rhs) {
    if (node.IsScalar()) {
      rhs = epoch_metadata::TimeFrame(node.as<std::string>());
      return true;
    }
    rhs =
        epoch_metadata::TimeFrame(node.as<epoch_frame::DateOffsetHandlerPtr>());
    return true;
  }
};
} // namespace YAML