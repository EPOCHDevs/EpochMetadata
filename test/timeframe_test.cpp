#include "epoch_metadata/time_frame.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <epoch_frame/factory/date_offset_factory.h>
#include <epoch_frame/relative_delta_options.h>
#include <glaze/json/generic.hpp>
#include <yaml-cpp/yaml.h>

using namespace epoch_metadata;

// Helper function to create TimeFrame objects for testing
TimeFrame CreateTimeFrame(epoch_core::EpochOffsetType type, int interval)
{
  switch (type)
  {
  case epoch_core::EpochOffsetType::Day:
    return TimeFrame(epoch_frame::factory::offset::days(interval));
  case epoch_core::EpochOffsetType::Hour:
    return TimeFrame(epoch_frame::factory::offset::hours(interval));
  case epoch_core::EpochOffsetType::Minute:
    return TimeFrame(epoch_frame::factory::offset::minutes(interval));
  case epoch_core::EpochOffsetType::Week:
    return TimeFrame(epoch_frame::factory::offset::weeks(interval));
  case epoch_core::EpochOffsetType::Month:
    return TimeFrame(epoch_frame::factory::offset::month_end(interval));
  case epoch_core::EpochOffsetType::Quarter:
    return TimeFrame(epoch_frame::factory::offset::quarter_end(interval));
  case epoch_core::EpochOffsetType::Year:
    return TimeFrame(epoch_frame::factory::offset::year_end(interval));
  default:
    throw std::runtime_error("Unsupported offset type for testing");
  }
}

TEST_CASE("TimeFrame operator< - same type different intervals",
          "[TimeFrame]")
{
  // Test that smaller intervals are less than larger intervals for the same
  // type
  auto tf1Day = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto tf5Days = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 5);

  REQUIRE(tf1Day < tf5Days);
  REQUIRE_FALSE(tf5Days < tf1Day);
  REQUIRE_FALSE(tf1Day <
                tf1Day); // Same timeframes should not be less than each other
}

TEST_CASE("TimeFrame operator< - different types same interval",
          "[TimeFrame]")
{
  // Test type ordering: minute < hour < day < week < month < quarter < year
  auto minute1 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 1);
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto week1 = CreateTimeFrame(epoch_core::EpochOffsetType::Week, 1);
  auto month1 = CreateTimeFrame(epoch_core::EpochOffsetType::Month, 1);
  auto quarter1 = CreateTimeFrame(epoch_core::EpochOffsetType::Quarter, 1);
  auto year1 = CreateTimeFrame(epoch_core::EpochOffsetType::Year, 1);

  // Minute should be less than all others
  REQUIRE(minute1 < hour1);
  REQUIRE(minute1 < day1);
  REQUIRE(minute1 < week1);
  REQUIRE(minute1 < month1);
  REQUIRE(minute1 < quarter1);
  REQUIRE(minute1 < year1);

  // Hour should be less than day, week, etc.
  REQUIRE(hour1 < day1);
  REQUIRE(hour1 < week1);
  REQUIRE(hour1 < month1);
  REQUIRE(hour1 < quarter1);
  REQUIRE(hour1 < year1);

  // Day should be less than week, month, etc.
  REQUIRE(day1 < week1);
  REQUIRE(day1 < month1);
  REQUIRE(day1 < quarter1);
  REQUIRE(day1 < year1);

  // Week should be less than month, quarter, year
  REQUIRE(week1 < month1);
  REQUIRE(week1 < quarter1);
  REQUIRE(week1 < year1);

  // Month should be less than quarter, year
  REQUIRE(month1 < quarter1);
  REQUIRE(month1 < year1);

  // Quarter should be less than year
  REQUIRE(quarter1 < year1);
}

TEST_CASE("TimeFrame operator< - mixed type and interval comparisons",
          "[TimeFrame]")
{
  // Test scenarios where type trumps interval size
  auto minute60 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 60);
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto day365 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 365);
  auto year1 = CreateTimeFrame(epoch_core::EpochOffsetType::Year, 1);

  // Even though 60 minutes == 1 hour in real time, minute type is less than
  // hour type
  REQUIRE(minute60 < hour1);

  // Even though 365 days == 1 year in real time, day type is less than year
  // type
  REQUIRE(day365 < year1);

  // Large interval of smaller type should still be less than small interval of
  // larger type
  auto minute1000 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 1000);
  auto hour1_small = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  REQUIRE(minute1000 < hour1_small);
}

TEST_CASE("TimeFrame operator< - same type different intervals comprehensive",
          "[TimeFrame]")
{
  // Test multiple intervals for different types

  // Minutes
  auto minute5 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 5);
  auto minute15 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 15);
  auto minute30 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 30);
  REQUIRE(minute5 < minute15);
  REQUIRE(minute15 < minute30);
  REQUIRE(minute5 < minute30);

  // Hours
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto hour4 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 4);
  auto hour24 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 24);
  REQUIRE(hour1 < hour4);
  REQUIRE(hour4 < hour24);
  REQUIRE(hour1 < hour24);

  // Days
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto day7 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 7);
  auto day30 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 30);
  REQUIRE(day1 < day7);
  REQUIRE(day7 < day30);
  REQUIRE(day1 < day30);
}

TEST_CASE("TimeFrame operator< - edge cases with equal timeframes",
          "[TimeFrame]")
{
  // Test that identical timeframes are not less than each other
  auto tf1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto tf2 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);

  REQUIRE_FALSE(tf1 < tf2);
  REQUIRE_FALSE(tf2 < tf1);

  // Test with different types but want to make sure equal doesn't trigger less
  // than
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto hour1_copy = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);

  REQUIRE_FALSE(hour1 < hour1_copy);
  REQUIRE_FALSE(hour1_copy < hour1);
}

TEST_CASE("TimeFrame operator< - comprehensive type ordering validation",
          "[TimeFrame]")
{
  // Create one of each type with the same interval to test type ordering
  std::vector<TimeFrame> timeframes = {
      CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Week, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Month, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Quarter, 1),
      CreateTimeFrame(epoch_core::EpochOffsetType::Year, 1)};

  // Verify that each timeframe is less than all timeframes that come after it
  for (size_t i = 0; i < timeframes.size(); ++i)
  {
    for (size_t j = i + 1; j < timeframes.size(); ++j)
    {
      REQUIRE(timeframes[i] < timeframes[j]);
      REQUIRE_FALSE(timeframes[j] < timeframes[i]);
    }
  }
}

TEST_CASE("TimeFrame operator< - practical trading timeframes", "[TimeFrame]")
{
  // Test common trading timeframes
  auto minute1 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 1);
  auto minute5 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 5);
  auto minute15 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 15);
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto hour4 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 4);
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto week1 = CreateTimeFrame(epoch_core::EpochOffsetType::Week, 1);
  auto month1 = CreateTimeFrame(epoch_core::EpochOffsetType::Month, 1);

  // Common intraday progression
  REQUIRE(minute1 < minute5);
  REQUIRE(minute5 < minute15);
  REQUIRE(minute15 < hour1);
  REQUIRE(hour1 < hour4);
  REQUIRE(hour4 < day1);
  REQUIRE(day1 < week1);
  REQUIRE(week1 < month1);
}

TEST_CASE("IsIntraday function - various offset types", "[IsIntraday]")
{
  // Intraday types
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Hour));
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Minute));
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Second));
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Milli));
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Micro));
  REQUIRE(IsIntraday(epoch_core::EpochOffsetType::Nano));

  // Non-intraday types
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::Day));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::Week));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::Month));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::MonthEnd));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::Quarter));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::QuarterEnd));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::Year));
  REQUIRE_FALSE(IsIntraday(epoch_core::EpochOffsetType::YearEnd));
}

TEST_CASE("TimeFrame::IsIntraDay method", "[TimeFrame][IsIntraDay]")
{
  // Intraday timeframes
  auto hourly = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  auto minutely = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 5);

  REQUIRE(hourly.IsIntraDay());
  REQUIRE(minutely.IsIntraDay());

  // Non-intraday timeframes
  auto daily = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto weekly = CreateTimeFrame(epoch_core::EpochOffsetType::Week, 1);
  auto monthly = CreateTimeFrame(epoch_core::EpochOffsetType::Month, 1);

  REQUIRE_FALSE(daily.IsIntraDay());
  REQUIRE_FALSE(weekly.IsIntraDay());
  REQUIRE_FALSE(monthly.IsIntraDay());
}

TEST_CASE("TimeFrame operator== - equality comparisons",
          "[TimeFrame][operator==]")
{
  // Same type and interval
  auto day1_a = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto day1_b = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  REQUIRE(day1_a == day1_b);

  // Same type, different intervals
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto day5 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 5);
  REQUIRE_FALSE(day1 == day5);

  // Different types, same interval
  auto day1_type = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  REQUIRE_FALSE(day1_type == hour1);

  // Different types, different intervals
  auto minute5 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 5);
  auto week2 = CreateTimeFrame(epoch_core::EpochOffsetType::Week, 2);
  REQUIRE_FALSE(minute5 == week2);
}

TEST_CASE("TimeFrame operator!= - inequality comparisons",
          "[TimeFrame][operator!=]")
{
  // Same type and interval should be equal (not not-equal)
  auto day1_a = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto day1_b = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  REQUIRE_FALSE(day1_a != day1_b);

  // Same type, different intervals
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto day5 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 5);
  REQUIRE(day1 != day5);

  // Different types, same interval
  auto day1_type = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto hour1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);
  REQUIRE(day1_type != hour1);

  // Different types, different intervals
  auto minute5 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 5);
  auto week2 = CreateTimeFrame(epoch_core::EpochOffsetType::Week, 2);
  REQUIRE(minute5 != week2);
}

TEST_CASE("TimeFrame::Serialize method", "[TimeFrame][Serialize]")
{
  // Test serialization of various timeframes
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto hour4 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 4);
  auto minute15 = CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 15);

  // Serialize and check that result is valid JSON
  std::string day_json = day1.Serialize();
  std::string hour_json = hour4.Serialize();
  std::string minute_json = minute15.Serialize();

  REQUIRE_FALSE(day_json.empty());
  REQUIRE_FALSE(hour_json.empty());
  REQUIRE_FALSE(minute_json.empty());

  // Check that they contain expected fields
  REQUIRE(day_json.find("type") != std::string::npos);
  REQUIRE(day_json.find("interval") != std::string::npos);
  REQUIRE(hour_json.find("type") != std::string::npos);
  REQUIRE(hour_json.find("interval") != std::string::npos);
  REQUIRE(minute_json.find("type") != std::string::npos);
  REQUIRE(minute_json.find("interval") != std::string::npos);
}

TEST_CASE("CreateDateOffsetHandlerFromJSON - various paths",
          "[CreateDateOffsetHandlerFromJSON]")
{
  // Test null input
  glz::generic null_json;
  auto null_result = CreateDateOffsetHandlerFromJSON(null_json);
  REQUIRE(null_result == nullptr);

  // Test valid day offset
  glz::generic day_json;
  day_json["type"] = "day";
  day_json["interval"] = 1;
  auto day_result = CreateDateOffsetHandlerFromJSON(day_json);
  REQUIRE(day_result != nullptr);
  REQUIRE(day_result->type() == epoch_core::EpochOffsetType::Day);
  REQUIRE(day_result->n() == 1);

  // Test valid hour offset
  glz::generic hour_json;
  hour_json["type"] = "hour";
  hour_json["interval"] = 4;
  auto hour_result = CreateDateOffsetHandlerFromJSON(hour_json);
  REQUIRE(hour_result != nullptr);
  REQUIRE(hour_result->type() == epoch_core::EpochOffsetType::Hour);
  REQUIRE(hour_result->n() == 4);

  // Test valid minute offset
  glz::generic minute_json;
  minute_json["type"] = "minute";
  minute_json["interval"] = 15;
  auto minute_result = CreateDateOffsetHandlerFromJSON(minute_json);
  REQUIRE(minute_result != nullptr);
  REQUIRE(minute_result->type() == epoch_core::EpochOffsetType::Minute);
  REQUIRE(minute_result->n() == 15);

  // Test valid week offset
  glz::generic week_json;
  week_json["type"] = "week";
  week_json["interval"] = 2;
  auto week_result = CreateDateOffsetHandlerFromJSON(week_json);
  REQUIRE(week_result != nullptr);
  REQUIRE(week_result->type() == epoch_core::EpochOffsetType::Week);
  REQUIRE(week_result->n() == 2);

  // Test week-of-month direct configuration (Second Tuesday)
  glz::generic wom_json;
  wom_json["type"] = "week";
  wom_json["interval"] = 1;
  wom_json["week_of_month"] = "Second";
  wom_json["weekday"] = "Tuesday";
  auto wom_result = CreateDateOffsetHandlerFromJSON(wom_json);
  REQUIRE(wom_result != nullptr);

  // Test valid month offset
  glz::generic month_json;
  month_json["type"] = "month";
  month_json["interval"] = 3;
  auto month_result = CreateDateOffsetHandlerFromJSON(month_json);
  REQUIRE(month_result != nullptr);
  REQUIRE(month_result->type() == epoch_core::EpochOffsetType::MonthEnd);
  REQUIRE(month_result->n() == 3);

  // Test valid quarter offset
  glz::generic quarter_json;
  quarter_json["type"] = "quarter";
  quarter_json["interval"] = 1;
  auto quarter_result = CreateDateOffsetHandlerFromJSON(quarter_json);
  REQUIRE(quarter_result != nullptr);
  REQUIRE(quarter_result->type() == epoch_core::EpochOffsetType::QuarterEnd);
  REQUIRE(quarter_result->n() == 1);

  // Test valid year offset
  glz::generic year_json;
  year_json["type"] = "year";
  year_json["interval"] = 5;
  auto year_result = CreateDateOffsetHandlerFromJSON(year_json);
  REQUIRE(year_result != nullptr);
  REQUIRE(year_result->type() == epoch_core::EpochOffsetType::YearEnd);
  REQUIRE(year_result->n() == 5);
}

TEST_CASE("CreateDateOffsetHandlerFromJSON - exception paths",
          "[CreateDateOffsetHandlerFromJSON][exception]")
{
  // Test invalid type
  glz::generic invalid_type_json;
  invalid_type_json["type"] = "invalid_type";
  invalid_type_json["interval"] = 1;
  REQUIRE_THROWS(CreateDateOffsetHandlerFromJSON(invalid_type_json));

  // Test missing type field
  glz::generic missing_type_json;
  missing_type_json["interval"] = 1;
  REQUIRE_THROWS(CreateDateOffsetHandlerFromJSON(missing_type_json));

  // Test missing interval field
  glz::generic missing_interval_json;
  missing_interval_json["type"] = "day";
  REQUIRE_THROWS(CreateDateOffsetHandlerFromJSON(missing_interval_json));
}

TEST_CASE("CreateDateOffsetHandlerJSON function",
          "[CreateDateOffsetHandlerJSON]")
{
  // Test null pointer
  epoch_frame::DateOffsetHandlerPtr null_ptr = nullptr;
  auto null_json = CreateDateOffsetHandlerJSON(null_ptr);
  REQUIRE(null_json.is_null());

  // Test valid day offset
  auto day_offset = epoch_frame::factory::offset::days(1);
  auto day_json = CreateDateOffsetHandlerJSON(day_offset);
  REQUIRE(day_json.is_object());
  REQUIRE(day_json["type"].as<std::string>() == "day");
  REQUIRE(day_json["interval"].as<int>() == 1);

  // Test valid hour offset
  auto hour_offset = epoch_frame::factory::offset::hours(4);
  auto hour_json = CreateDateOffsetHandlerJSON(hour_offset);
  REQUIRE(hour_json.is_object());
  REQUIRE(hour_json["type"].as<std::string>() == "hour");
  REQUIRE(hour_json["interval"].as<int>() == 4);

  // Test valid minute offset
  auto minute_offset = epoch_frame::factory::offset::minutes(15);
  auto minute_json = CreateDateOffsetHandlerJSON(minute_offset);
  REQUIRE(minute_json.is_object());
  REQUIRE(minute_json["type"].as<std::string>() == "minute");
  REQUIRE(minute_json["interval"].as<int>() == 15);
}

TEST_CASE("JSON serialization round-trip", "[JSON][serialization]")
{
  // Test round-trip serialization: TimeFrame -> JSON -> TimeFrame
  auto original_day = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto original_hour = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 4);
  auto original_minute =
      CreateTimeFrame(epoch_core::EpochOffsetType::Minute, 15);

  // Serialize to JSON
  std::string day_json = original_day.Serialize();
  std::string hour_json = original_hour.Serialize();
  std::string minute_json = original_minute.Serialize();

  // Parse JSON back
  glz::generic day_parsed;
  glz::generic hour_parsed;
  glz::generic minute_parsed;

  REQUIRE_FALSE(glz::read_json(day_parsed, day_json));
  REQUIRE_FALSE(glz::read_json(hour_parsed, hour_json));
  REQUIRE_FALSE(glz::read_json(minute_parsed, minute_json));

  // Create new TimeFrame objects from parsed JSON
  auto day_offset = CreateDateOffsetHandlerFromJSON(day_parsed);
  auto hour_offset = CreateDateOffsetHandlerFromJSON(hour_parsed);
  auto minute_offset = CreateDateOffsetHandlerFromJSON(minute_parsed);

  TimeFrame reconstructed_day(day_offset);
  TimeFrame reconstructed_hour(hour_offset);
  TimeFrame reconstructed_minute(minute_offset);

  // Verify they match the originals
  REQUIRE(original_day == reconstructed_day);
  REQUIRE(original_hour == reconstructed_hour);
  REQUIRE(original_minute == reconstructed_minute);
}

TEST_CASE("YAML serialization", "[YAML][serialization]")
{
  // Test YAML serialization and deserialization
  YAML::Node day_node;
  day_node["type"] = "day";
  day_node["interval"] = 1;

  YAML::Node hour_node;
  hour_node["type"] = "hour";
  hour_node["interval"] = 4;

  YAML::Node minute_node;
  minute_node["type"] = "minute";
  minute_node["interval"] = 15;

  // Convert from YAML
  auto day_offset = day_node.as<epoch_frame::DateOffsetHandlerPtr>();
  auto hour_offset = hour_node.as<epoch_frame::DateOffsetHandlerPtr>();
  auto minute_offset = minute_node.as<epoch_frame::DateOffsetHandlerPtr>();

  REQUIRE(day_offset != nullptr);
  REQUIRE(hour_offset != nullptr);
  REQUIRE(minute_offset != nullptr);

  REQUIRE(day_offset->type() == epoch_core::EpochOffsetType::Day);
  REQUIRE(day_offset->n() == 1);

  REQUIRE(hour_offset->type() == epoch_core::EpochOffsetType::Hour);
  REQUIRE(hour_offset->n() == 4);

  REQUIRE(minute_offset->type() == epoch_core::EpochOffsetType::Minute);
  REQUIRE(minute_offset->n() == 15);

  // Test TimeFrame YAML conversion
  TimeFrame day_tf(day_offset);
  TimeFrame hour_tf(hour_offset);
  TimeFrame minute_tf(minute_offset);

  REQUIRE(day_tf.ToString() == day_offset->name());
  REQUIRE(hour_tf.ToString() == hour_offset->name());
  REQUIRE(minute_tf.ToString() == minute_offset->name());
}

TEST_CASE("CreateDateOffsetHandlerJSON - anchored types and extras",
          "[CreateDateOffsetHandlerJSON][anchored]")
{
  // Month start/end anchors
  auto m_start = epoch_frame::factory::offset::month_start(2);
  auto m_start_json = CreateDateOffsetHandlerJSON(m_start);
  REQUIRE(m_start_json.is_object());
  REQUIRE(m_start_json["type"].as<std::string>() == "month");
  REQUIRE(m_start_json["interval"].as<int>() == 2);
  REQUIRE(m_start_json["anchor"].as<std::string>() == "Start");

  auto m_end = epoch_frame::factory::offset::month_end(3);
  auto m_end_json = CreateDateOffsetHandlerJSON(m_end);
  REQUIRE(m_end_json.is_object());
  REQUIRE(m_end_json["type"].as<std::string>() == "month");
  REQUIRE(m_end_json["interval"].as<int>() == 3);
  REQUIRE(m_end_json["anchor"].as<std::string>() == "End");

  // Quarter with starting month
  auto q_start = epoch_frame::factory::offset::quarter_start(
      1, std::optional<std::chrono::month>{std::chrono::month{3}});
  auto q_start_json = CreateDateOffsetHandlerJSON(q_start);
  REQUIRE(q_start_json.is_object());
  REQUIRE(q_start_json["type"].as<std::string>() == "quarter");
  REQUIRE(q_start_json["interval"].as<int>() == 1);
  REQUIRE(q_start_json["anchor"].as<std::string>() == "Start");
  REQUIRE_FALSE(q_start_json["month"].is_null());

  // Year with month
  auto y_end = epoch_frame::factory::offset::year_end(
      5, std::optional<std::chrono::month>{std::chrono::month{2}});
  auto y_end_json = CreateDateOffsetHandlerJSON(y_end);
  REQUIRE(y_end_json.is_object());
  REQUIRE(y_end_json["type"].as<std::string>() == "year");
  REQUIRE(y_end_json["interval"].as<int>() == 5);
  REQUIRE(y_end_json["anchor"].as<std::string>() == "End");
  REQUIRE_FALSE(y_end_json["month"].is_null());

  // Weekly: best-effort extras for weekday/week_of_month if present
  auto rd_week = epoch_frame::factory::offset::date_offset(
      1, epoch_frame::RelativeDeltaOption{
             .weekday = epoch_frame::Weekday{
                 epoch_core::EpochDayOfWeekWrapper::FromString("Monday"), 2}});
  auto rd_week_json = CreateDateOffsetHandlerJSON(rd_week);
  REQUIRE(rd_week_json.is_object());
  REQUIRE(rd_week_json["type"].as<std::string>() == "week");
  REQUIRE_FALSE(rd_week_json["weekday"].is_null());
  REQUIRE_FALSE(rd_week_json["week_of_month"].is_null());
}

TEST_CASE("CreateDateOffsetHandlerFromJSON - business days",
          "[CreateDateOffsetHandlerFromJSON][bday]")
{
  // Business day
  glz::generic bday_json;
  bday_json["type"] = "bday";
  bday_json["interval"] = 4;
  bday_json["time_offset"]["minutes"] = 30;
  auto bday = CreateDateOffsetHandlerFromJSON(bday_json);
  REQUIRE(bday != nullptr);
  REQUIRE(bday->type() == epoch_core::EpochOffsetType::BusinessDay);
  REQUIRE(bday->n() == 4);

  // Session anchored via JSON (NewYork before close -30m)
  glz::generic session_json;
  session_json["type"] = "session";
  session_json["interval"] = 1;
  session_json["session"] = "NewYork";
  session_json["session_anchor"] = "BeforeClose";
  session_json["time_offset"]["minutes"] = 30;
  auto session = CreateDateOffsetHandlerFromJSON(session_json);
  REQUIRE(session != nullptr);
}

TEST_CASE("CreateDateOffsetHandlerFromJSON - anchored read",
          "[CreateDateOffsetHandlerFromJSON][anchored]")
{
  // Month start
  glz::generic month_start_json;
  month_start_json["type"] = "month";
  month_start_json["interval"] = 1;
  month_start_json["anchor"] = "Start";
  auto m_start = CreateDateOffsetHandlerFromJSON(month_start_json);
  REQUIRE(m_start != nullptr);
  auto m_start_roundtrip = CreateDateOffsetHandlerJSON(m_start);
  REQUIRE(m_start_roundtrip["anchor"].as<std::string>() == "Start");

  // Year end with month
  glz::generic year_end_json;
  year_end_json["type"] = "year";
  year_end_json["interval"] = 3;
  year_end_json["anchor"] = "End";
  year_end_json["month"] = "feb";
  auto y_end = CreateDateOffsetHandlerFromJSON(year_end_json);
  REQUIRE(y_end != nullptr);
  auto y_end_roundtrip = CreateDateOffsetHandlerJSON(y_end);
  REQUIRE(y_end_roundtrip["anchor"].as<std::string>() == "End");
  REQUIRE_FALSE(y_end_roundtrip["month"].is_null());
}

TEST_CASE("CreateTimeFrameFromYAML - basic and anchored",
          "[CreateTimeFrameFromYAML]")
{
  // Basic day
  YAML::Node day_node;
  day_node["type"] = "day";
  day_node["interval"] = 1;
  auto day_tf = CreateTimeFrameFromYAML(day_node);
  REQUIRE(day_tf.ToString() == day_tf.GetOffset()->name());
  REQUIRE(day_tf.GetOffset()->type() == epoch_core::EpochOffsetType::Day);

  // Anchored month end
  YAML::Node month_node;
  month_node["type"] = "month";
  month_node["interval"] = 2;
  month_node["anchor"] = "End";
  auto month_tf = CreateTimeFrameFromYAML(month_node);
  REQUIRE(month_tf.GetOffset()->type() ==
          epoch_core::EpochOffsetType::MonthEnd);

  // Week-of-month via YAML (Last Friday)
  YAML::Node wom_node;
  wom_node["type"] = "week";
  wom_node["interval"] = 1;
  wom_node["week_of_month"] = "Last";
  wom_node["weekday"] = "Friday";
  auto wom_tf = CreateTimeFrameFromYAML(wom_node);
  REQUIRE(wom_tf.GetOffset() != nullptr);

  // bday with time_offset via YAML
  YAML::Node bday_node;
  bday_node["type"] = "bday";
  bday_node["interval"] = 1;
  bday_node["time_offset"]["minutes"] = 15;
  auto bday_tf = CreateTimeFrameFromYAML(bday_node);
  REQUIRE(bday_tf.GetOffset() != nullptr);

  // Session via YAML (NewYork after open +15m)
  YAML::Node session_node;
  session_node["type"] = "session";
  session_node["interval"] = 2;
  session_node["session"] = "NewYork";
  session_node["session_anchor"] = "AfterOpen";
  session_node["time_offset"]["minutes"] = 15;
  auto session_tf = CreateTimeFrameFromYAML(session_node);
  REQUIRE(session_tf.GetOffset() != nullptr);
}

TEST_CASE("TimeFrame hashing and set behavior", "[TimeFrame][hash]")
{
  auto d1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto d1_dup = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto h1 = CreateTimeFrame(epoch_core::EpochOffsetType::Hour, 1);

  TimeFrameSet set;
  set.insert(d1);
  set.insert(d1_dup);
  set.insert(h1);

  REQUIRE(set.size() == 2);
  REQUIRE(set.find(d1) != set.end());
  REQUIRE(set.find(h1) != set.end());
}

TEST_CASE("TimeFrame operator< - business day ordering", "[TimeFrame]")
{
  auto day1 = CreateTimeFrame(epoch_core::EpochOffsetType::Day, 1);
  auto bday1 = TimeFrame(epoch_frame::factory::offset::bday(1));

  REQUIRE(day1 < bday1);
}

TEST_CASE("TIMEFRAME_MAPPING - YAML scalar shortcuts",
          "[TimeFrame][YAML][Mapping]")
{
  // Simple minute mapping
  auto node_1min = YAML::Load("1Min");
  auto off_1min = node_1min.as<epoch_frame::DateOffsetHandlerPtr>();
  REQUIRE(off_1min != nullptr);
  REQUIRE(off_1min->type() == epoch_core::EpochOffsetType::Minute);
  REQUIRE(off_1min->n() == 1);

  // Hour mapping
  auto node_1h = YAML::Load("1H");
  auto off_1h = node_1h.as<epoch_frame::DateOffsetHandlerPtr>();
  REQUIRE(off_1h != nullptr);
  REQUIRE(off_1h->type() == epoch_core::EpochOffsetType::Hour);
  REQUIRE(off_1h->n() == 1);

  // Week with weekday mapping
  auto node_week_fri = YAML::Load("1W-FRI");
  auto off_week_fri = node_week_fri.as<epoch_frame::DateOffsetHandlerPtr>();
  REQUIRE(off_week_fri != nullptr);
  REQUIRE(off_week_fri->type() == epoch_core::EpochOffsetType::Week);
  REQUIRE(off_week_fri->n() == 1);

  // Week-of-month mapping (should produce a valid handler)
  auto node_wom = YAML::Load("1W-MON-2nd");
  auto off_wom = node_wom.as<epoch_frame::DateOffsetHandlerPtr>();
  REQUIRE(off_wom != nullptr);

  // Month end shortcut
  auto node_me = YAML::Load("1ME");
  auto off_me = node_me.as<epoch_frame::DateOffsetHandlerPtr>();
  REQUIRE(off_me != nullptr);
  REQUIRE(off_me->type() == epoch_core::EpochOffsetType::MonthEnd);
  REQUIRE(off_me->n() == 1);
}

TEST_CASE("TIMEFRAME_MAPPING - Glaze string shortcuts",
          "[TimeFrame][JSON][Mapping]")
{
  // Prepare a JSON string token by parsing a JSON string literal
  glz::generic j_1min;
  REQUIRE_FALSE(glz::read_json(j_1min, "\"1Min\""));
  auto off_1min = CreateDateOffsetHandlerFromJSON(j_1min);
  REQUIRE(off_1min != nullptr);
  REQUIRE(off_1min->type() == epoch_core::EpochOffsetType::Minute);
  REQUIRE(off_1min->n() == 1);

  glz::generic j_1h;
  REQUIRE_FALSE(glz::read_json(j_1h, "\"1H\""));
  auto off_1h = CreateDateOffsetHandlerFromJSON(j_1h);
  REQUIRE(off_1h != nullptr);
  REQUIRE(off_1h->type() == epoch_core::EpochOffsetType::Hour);
  REQUIRE(off_1h->n() == 1);

  glz::generic j_wom;
  REQUIRE_FALSE(glz::read_json(j_wom, "\"1W-MON-3rd\""));
  auto off_wom = CreateDateOffsetHandlerFromJSON(j_wom);
  REQUIRE(off_wom != nullptr);

  glz::generic j_me;
  REQUIRE_FALSE(glz::read_json(j_me, "\"1ME\""));
  auto off_me = CreateDateOffsetHandlerFromJSON(j_me);
  REQUIRE(off_me != nullptr);
  REQUIRE(off_me->type() == epoch_core::EpochOffsetType::MonthEnd);
  REQUIRE(off_me->n() == 1);
}
