//
// Created by adesola on 1/4/25.
//

#include "epoch_metadata/time_frame.h"
#include <epoch_core/error_type.h>

#include "date_time/date_offsets.h"
#include "epoch_core/macros.h"
#include "epoch_frame/factory/calendar_factory.h"
#include "epoch_frame/factory/date_offset_factory.h"
#include <cctype>
#include <glaze/json/json_t.hpp>
#include <utility>

namespace epoch_metadata {
bool IsIntraday(epoch_core::EpochOffsetType type) {
  return (type == epoch_core::EpochOffsetType::Hour ||
          type == epoch_core::EpochOffsetType::Minute ||
          type == epoch_core::EpochOffsetType::Second ||
          type == epoch_core::EpochOffsetType::Milli ||
          type == epoch_core::EpochOffsetType::Micro ||
          type == epoch_core::EpochOffsetType::Nano);
}

TimeFrame::TimeFrame(epoch_frame::DateOffsetHandlerPtr offset)
    : m_offset(std::move(offset)) {
  AssertFalseFromStream(m_offset == nullptr, "TimeFrame offset cannot be "
                                             "nullptr");
}

bool TimeFrame::IsIntraDay() const { return IsIntraday(m_offset->type()); }

std::string TimeFrame::ToString() const {
  return m_offset ? m_offset->name() : "";
}

bool TimeFrame::operator==(TimeFrame const &other) const {
  return m_offset->name() == other.m_offset->name();
}

bool TimeFrame::operator!=(TimeFrame const &other) const {
  return !(*this == other);
}

std::string TimeFrame::Serialize() const {
  std::string result;
  const auto error = glz::write_json(m_offset, result);
  if (error) {
    const auto desc = format_error(error);
    throw std::runtime_error("Failed to serialize timeframe: " + desc);
  }
  return result;
}

std::optional<epoch_core::EpochOffsetType>
toEpochOffsetType(epoch_core::StratifyxTimeFrameType type) {
  switch (type) {
  case epoch_core::StratifyxTimeFrameType::minute:
    return epoch_core::EpochOffsetType::Minute;
  case epoch_core::StratifyxTimeFrameType::hour:
    return epoch_core::EpochOffsetType::Hour;
  case epoch_core::StratifyxTimeFrameType::day:
    return epoch_core::EpochOffsetType::Day;
  case epoch_core::StratifyxTimeFrameType::week:
    return epoch_core::EpochOffsetType::Week;
  case epoch_core::StratifyxTimeFrameType::week_of_month:
    return epoch_core::EpochOffsetType::WeekOfMonth;
  case epoch_core::StratifyxTimeFrameType::month:
    return epoch_core::EpochOffsetType::Month;
  case epoch_core::StratifyxTimeFrameType::bmonth:
    return epoch_core::EpochOffsetType::BusinessMonth;
  case epoch_core::StratifyxTimeFrameType::quarter:
    return epoch_core::EpochOffsetType::Quarter;
  case epoch_core::StratifyxTimeFrameType::year:
    return epoch_core::EpochOffsetType::Year;
  case epoch_core::StratifyxTimeFrameType::bday:
    return epoch_core::EpochOffsetType::BusinessDay;
  case epoch_core::StratifyxTimeFrameType::session:
    return epoch_core::EpochOffsetType::SessionAnchor;
  default:
    break;
  }
  return std::nullopt;
}

epoch_core::StratifyxTimeFrameType
fromOffset(epoch_core::EpochOffsetType type) {
  switch (type) {
  case epoch_core::EpochOffsetType::Minute:
    return epoch_core::StratifyxTimeFrameType::minute;
  case epoch_core::EpochOffsetType::Hour:
    return epoch_core::StratifyxTimeFrameType::hour;
  case epoch_core::EpochOffsetType::Day:
    return epoch_core::StratifyxTimeFrameType::day;
  case epoch_core::EpochOffsetType::Week:
    return epoch_core::StratifyxTimeFrameType::week;
  case epoch_core::EpochOffsetType::WeekOfMonth:
  case epoch_core::EpochOffsetType::LastWeekOfMonth:
    return epoch_core::StratifyxTimeFrameType::week_of_month;
  case epoch_core::EpochOffsetType::Month:
  case epoch_core::EpochOffsetType::MonthStart:
  case epoch_core::EpochOffsetType::MonthEnd:
    return epoch_core::StratifyxTimeFrameType::month;
  case epoch_core::EpochOffsetType::BusinessMonth:
  case epoch_core::EpochOffsetType::BusinessMonthStart:
  case epoch_core::EpochOffsetType::BusinessMonthEnd:
    return epoch_core::StratifyxTimeFrameType::bmonth;
  case epoch_core::EpochOffsetType::Quarter:
  case epoch_core::EpochOffsetType::QuarterStart:
  case epoch_core::EpochOffsetType::QuarterEnd:
    return epoch_core::StratifyxTimeFrameType::quarter;
  case epoch_core::EpochOffsetType::Year:
  case epoch_core::EpochOffsetType::YearStart:
  case epoch_core::EpochOffsetType::YearEnd:
    return epoch_core::StratifyxTimeFrameType::year;
  case epoch_core::EpochOffsetType::BusinessDay:
    return epoch_core::StratifyxTimeFrameType::bday;
  case epoch_core::EpochOffsetType::SessionAnchor:
    return epoch_core::StratifyxTimeFrameType::session;
  case epoch_core::EpochOffsetType::RelativeDelta:
    return epoch_core::StratifyxTimeFrameType::week;
  default:
    break;
  }
  throw std::runtime_error("Invalid Timeframe Type: " +
                           epoch_core::EpochOffsetTypeWrapper::ToString(type));
}

std::chrono::month toChronoMonth(epoch_core::StratifyxMonth type) {
  return std::chrono::month(epoch_core::StratifyxMonthWrapper::toNumber(type) +
                            1);
}

epoch_core::StratifyxMonth fromChronoMonth(std::chrono::month type) {
  return static_cast<epoch_core::StratifyxMonth>(static_cast<unsigned>(type) -
                                                 1);
}

// Helper: build handler from parsed options
static epoch_frame::DateOffsetHandlerPtr
MakeHandlerFromOption(DateOffsetOption const &option) {
  switch (option.type) {
  case epoch_core::StratifyxTimeFrameType::day:
    return epoch_frame::factory::offset::days(option.interval);
  case epoch_core::StratifyxTimeFrameType::hour:
    return epoch_frame::factory::offset::hours(option.interval);
  case epoch_core::StratifyxTimeFrameType::minute:
    return epoch_frame::factory::offset::minutes(option.interval);
  case epoch_core::StratifyxTimeFrameType::week: {
    return epoch_frame::factory::offset::weeks(option.interval, option.weekday);
  }
  case epoch_core::StratifyxTimeFrameType::week_of_month: {
    if (option.week_of_month == epoch_core::WeekOfMonth::Last) {
      return epoch_frame::factory::offset::last_week_of_month(option.interval,
                                                              option.weekday);
    }

    int week_index = 0; // 0-based index
    switch (option.week_of_month) {
    case epoch_core::WeekOfMonth::First:
      week_index = 0;
      break;
    case epoch_core::WeekOfMonth::Second:
      week_index = 1;
      break;
    case epoch_core::WeekOfMonth::Third:
      week_index = 2;
      break;
    case epoch_core::WeekOfMonth::Fourth:
      week_index = 3;
      break;
    default:
      week_index = 0;
      break;
    }
    return epoch_frame::factory::offset::week_of_month(
        option.interval, week_index, option.weekday);
  }
  case epoch_core::StratifyxTimeFrameType::month: {
    if (option.anchor == epoch_core::AnchoredTimeFrameType::Start) {
      return epoch_frame::factory::offset::month_start(option.interval);
    }
    return epoch_frame::factory::offset::month_end(option.interval);
  }
  case epoch_core::StratifyxTimeFrameType::bmonth: {
    // Business month begin/end
    if (option.anchor == epoch_core::AnchoredTimeFrameType::Start) {
      return epoch_frame::factory::offset::bmonth_begin(option.interval);
    }
    return epoch_frame::factory::offset::bmonth_end(option.interval);
  }
  case epoch_core::StratifyxTimeFrameType::quarter: {
    std::optional<std::chrono::month> m = std::nullopt;
    if (option.month != epoch_core::StratifyxMonth::Null) {
      m = toChronoMonth(option.month);
    }
    if (option.anchor == epoch_core::AnchoredTimeFrameType::Start) {
      return epoch_frame::factory::offset::quarter_start(option.interval, m);
    }
    return epoch_frame::factory::offset::quarter_end(option.interval, m);
  }
  case epoch_core::StratifyxTimeFrameType::year: {
    std::optional<std::chrono::month> m = std::nullopt;
    if (option.month != epoch_core::StratifyxMonth::Null) {
      m = toChronoMonth(option.month);
    }
    if (option.anchor == epoch_core::AnchoredTimeFrameType::Start) {
      return epoch_frame::factory::offset::year_start(option.interval, m);
    }
    return epoch_frame::factory::offset::year_end(option.interval, m);
  }
  case epoch_core::StratifyxTimeFrameType::bday: {
    return epoch_frame::factory::offset::bday(option.interval,
                                              option.time_offset);
  }
  case epoch_core::StratifyxTimeFrameType::session: {
    using epoch_frame::calendar::CalendarFactory;
    const auto cal_name =
        epoch_core::MarketCalendarNameWrapper::ToString(option.market_calendar);
    auto cal = CalendarFactory::instance().get_calendar(cal_name);
    auto which =
        option.session_anchor == epoch_core::SessionAnchorType::AfterOpen
            ? epoch_frame::SessionAnchorWhich::AfterOpen
            : epoch_frame::SessionAnchorWhich::BeforeClose;
    const auto delta = option.time_offset.value_or(
        epoch_frame::TimeDelta{epoch_frame::TimeDelta::Components{}});
    return epoch_frame::factory::offset::session_anchor(cal, which, delta,
                                                        option.interval);
  }
  default:
    break;
  }
  std::unreachable();
}

template <typename Serializer>
epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandler(Serializer const &buffer) {
  DateOffsetOption option;
  if constexpr (std::is_same_v<Serializer, glz::json_t>) {
    option = buffer.template as<DateOffsetOption>();
  } else if constexpr (std::is_same_v<Serializer, YAML::Node>) {
    option = buffer.template as<DateOffsetOption>();
  } else {
    static_assert(false, "Invalid serializer type");
  }

  return MakeHandlerFromOption(option);
}

bool TimeFrame::operator<(TimeFrame const &other) const {
  const auto m_type = fromOffset(m_offset->type());
  const auto other_type = fromOffset(other.m_offset->type());
  if (m_type == other_type)
    return m_offset->n() < other.m_offset->n();

  return epoch_core::StratifyxTimeFrameTypeWrapper::toNumber(m_type) <
         epoch_core::StratifyxTimeFrameTypeWrapper::toNumber(other_type);
}

epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandlerFromJSON(glz::json_t const &buffer) {
  if (buffer.is_null()) {
    return nullptr;
  }
  // Manually parse JSON into DateOffsetOption to avoid requiring glaze meta
  DateOffsetOption option;
  AssertFromFormat(
      buffer.contains("type") && buffer.contains("interval"),
      "DateOffsetHandler JSON must contain type and interval fields");

  const auto &type_node = buffer["type"];
  const auto &interval_node = buffer["interval"];
  AssertFalseFromStream(type_node.is_null(), "Missing required field: type");
  AssertFalseFromStream(interval_node.is_null(),
                        "Missing required field: interval");
  option.type = epoch_core::StratifyxTimeFrameTypeWrapper::FromString(
      type_node.as<std::string>());
  option.interval = static_cast<uint32_t>(interval_node.as<int64_t>());

  if (buffer.contains("anchor")) {
    const auto &anchor_node = buffer["anchor"];
    if (!anchor_node.is_null()) {
      option.anchor = epoch_core::AnchoredTimeFrameTypeWrapper::FromString(
          anchor_node.as<std::string>());
    }
  }

  if (buffer.contains("week_of_month")) {
    const auto &wom_node = buffer["week_of_month"];
    if (!wom_node.is_null()) {
      option.week_of_month = epoch_core::WeekOfMonthWrapper::FromString(
          wom_node.as<std::string>());
    }
  }

  if (buffer.contains("weekday")) {
    const auto &weekday_node = buffer["weekday"];
    if (!weekday_node.is_null()) {
      option.weekday = epoch_core::EpochDayOfWeekWrapper::FromString(
          weekday_node.as<std::string>());
    }
  }

  if (buffer.contains("month")) {
    const auto &month_node = buffer["month"];
    if (!month_node.is_null()) {
      option.month = epoch_core::StratifyxMonthWrapper::FromString(
          month_node.as<std::string>());
    }
  }

  // Optional time_offset for business day offsets
  if (buffer.contains("time_offset")) {
    const auto &to_node = buffer["time_offset"];
    if (!to_node.is_null() && to_node.is_object()) {
      epoch_frame::TimeDelta::Components c{};
      if (to_node.contains("days"))
        c.days = to_node["days"].as<double>();
      if (to_node.contains("hours"))
        c.hours = to_node["hours"].as<double>();
      if (to_node.contains("minutes"))
        c.minutes = to_node["minutes"].as<double>();
      if (to_node.contains("seconds"))
        c.seconds = to_node["seconds"].as<double>();
      if (to_node.contains("milliseconds"))
        c.milliseconds = to_node["milliseconds"].as<double>();
      if (to_node.contains("microseconds"))
        c.microseconds = to_node["microseconds"].as<double>();
      if (to_node.contains("weeks"))
        c.weeks = to_node["weeks"].as<double>();
      option.time_offset = epoch_frame::TimeDelta{c};
    }
  }

  // Session-anchored fields
  if (buffer.contains("market_calendar")) {
    const auto &mc_node = buffer["market_calendar"];
    if (!mc_node.is_null()) {
      option.market_calendar =
          epoch_core::MarketCalendarNameWrapper::FromString(
              mc_node.as<std::string>());
    }
  }
  if (buffer.contains("session_anchor")) {
    const auto &sa_node = buffer["session_anchor"];
    if (!sa_node.is_null()) {
      option.session_anchor = epoch_core::SessionAnchorTypeWrapper::FromString(
          sa_node.as<std::string>());
    }
  }

  return MakeHandlerFromOption(option);
}

glz::json_t
CreateDateOffsetHandlerJSON(epoch_frame::DateOffsetHandlerPtr const &x) {
  glz::json_t result;
  if (!x) {
    return result; // null json
  }

  const auto tf_type = fromOffset(x->type());
  result["type"] = epoch_core::StratifyxTimeFrameTypeWrapper::ToString(tf_type);
  result["interval"] = static_cast<int64_t>(x->n());

  // Add anchor information for anchored types
  if (tf_type == epoch_core::StratifyxTimeFrameType::month ||
      tf_type == epoch_core::StratifyxTimeFrameType::quarter ||
      tf_type == epoch_core::StratifyxTimeFrameType::year) {
    const auto anchor = x->is_end() ? epoch_core::AnchoredTimeFrameType::End
                                    : epoch_core::AnchoredTimeFrameType::Start;
    result["anchor"] =
        epoch_core::AnchoredTimeFrameTypeWrapper::ToString(anchor);
    if (tf_type == epoch_core::StratifyxTimeFrameType::quarter) {
      auto handler = dynamic_cast<epoch_frame::QuarterOffsetHandler *>(x.get());
      if (handler) {
        result["month"] = epoch_core::StratifyxMonthWrapper::ToString(
            fromChronoMonth(handler->get_starting_month()));
      }
    }
    if (tf_type == epoch_core::StratifyxTimeFrameType::year) {
      auto handler = dynamic_cast<epoch_frame::YearOffsetHandler *>(x.get());
      if (handler) {
        result["month"] = epoch_core::StratifyxMonthWrapper::ToString(
            fromChronoMonth(handler->get_month()));
      }
    }
  }

  // Best-effort: include weekday for weekly offsets if present in the code
  // string
  if (tf_type == epoch_core::StratifyxTimeFrameType::week) {
    if (auto week_handler = dynamic_cast<epoch_frame::WeekHandler *>(x.get())) {
      auto weekday = week_handler->get_weekday();
      if (weekday) {
        result["weekday"] =
            epoch_core::EpochDayOfWeekWrapper::ToString(weekday.value());
      }
    }
    if (auto rd_handler =
            dynamic_cast<epoch_frame::RelativeDeltaOffsetHandler *>(x.get())) {
      auto weekday = rd_handler->get_relative_delta().weekday();
      if (weekday) {
        result["weekday"] =
            epoch_core::EpochDayOfWeekWrapper::ToString(weekday->weekday());
        auto n = weekday->n().value_or(1);
        auto wom = epoch_core::WeekOfMonth::Null;
        if (n == 1) {
          wom = epoch_core::WeekOfMonth::First;
        } else if (n == 2) {
          wom = epoch_core::WeekOfMonth::Second;
        } else if (n == 3) {
          wom = epoch_core::WeekOfMonth::Third;
        } else if (n == 4) {
          wom = epoch_core::WeekOfMonth::Fourth;
        }
        result["week_of_month"] = epoch_core::WeekOfMonthWrapper::ToString(wom);
      }
    }
  }

  return result;
}

} // namespace epoch_metadata

// YAML helper surface for a concise timeframe mapping
namespace epoch_metadata {
TimeFrame CreateTimeFrameFromYAML(YAML::Node const &node) {
  auto offset = CreateDateOffsetHandler(node);
  return TimeFrame(offset);
}
} // namespace epoch_metadata

namespace YAML {
bool convert<epoch_frame::DateOffsetHandlerPtr>::decode(
    const Node &node, epoch_frame::DateOffsetHandlerPtr &rhs) {
  rhs = epoch_metadata::CreateDateOffsetHandler(node);
  return true;
}

bool convert<DateOffsetOption>::decode(const Node &node,
                                       DateOffsetOption &rhs) {
  rhs.type = epoch_core::StratifyxTimeFrameTypeWrapper::FromString(
      node["type"].as<std::string>());
  rhs.interval = node["interval"].as<uint32_t>(1);
  rhs.anchor = epoch_core::AnchoredTimeFrameTypeWrapper::FromString(
      node["anchor"].as<std::string>("Start"));
  rhs.week_of_month = epoch_core::WeekOfMonthWrapper::FromString(
      node["week_of_month"].as<std::string>("Null"));
  rhs.weekday = epoch_core::EpochDayOfWeekWrapper::FromString(
      node["weekday"].as<std::string>("Null"));
  rhs.month = epoch_core::StratifyxMonthWrapper::FromString(
      node["month"].as<std::string>("Null"));
  if (node["time_offset"]) {
    const auto &to_node = node["time_offset"];
    epoch_frame::TimeDelta::Components c{};
    c.days = to_node["days"].as<double>(0);
    c.hours = to_node["hours"].as<double>(0);
    c.minutes = to_node["minutes"].as<double>(0);
    c.seconds = to_node["seconds"].as<double>(0);
    c.milliseconds = to_node["milliseconds"].as<double>(0);
    c.microseconds = to_node["microseconds"].as<double>(0);
    c.weeks = to_node["weeks"].as<double>(0);
    rhs.time_offset = epoch_frame::TimeDelta{c};
  }
  rhs.market_calendar = epoch_core::MarketCalendarNameWrapper::FromString(
      node["market_calendar"].as<std::string>("Null"));
  rhs.session_anchor = epoch_core::SessionAnchorTypeWrapper::FromString(
      node["session_anchor"].as<std::string>("Null"));
  return true;
}
} // namespace YAML