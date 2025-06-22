#include "common.h"
#include "epoch_metadata/time_frame.h"
#include <catch2/catch_all.hpp>
#include <epoch_frame/factory/date_offset_factory.h>

using namespace epoch_metadata;

// Helper function to create TimeFrame objects for testing
TimeFrame CreateTimeFrame(epoch_core::EpochOffsetType type, int interval) {
  switch (type) {
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
          "[TimeFrame]") {
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
          "[TimeFrame]") {
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
          "[TimeFrame]") {
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
          "[TimeFrame]") {
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
          "[TimeFrame]") {
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
          "[TimeFrame]") {
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
  for (size_t i = 0; i < timeframes.size(); ++i) {
    for (size_t j = i + 1; j < timeframes.size(); ++j) {
      REQUIRE(timeframes[i] < timeframes[j]);
      REQUIRE_FALSE(timeframes[j] < timeframes[i]);
    }
  }
}

TEST_CASE("TimeFrame operator< - practical trading timeframes", "[TimeFrame]") {
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
