#pragma once

#include <epoch_script/transforms/core/metadata.h>

namespace epoch_script::transform {

// Factory function to create metadata for all datetime transforms
inline std::vector<epoch_script::transforms::TransformsMetaData>
MakeDatetimeTransforms() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // 1. Index Datetime Extract - Extract datetime components from bar timestamps
  metadataList.emplace_back(
      epoch_script::transforms::TransformsMetaData{
          .id = "index_datetime_extract",
          .category = epoch_core::TransformCategory::Utility,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Index Datetime Extract",
          .options =
              {
                  MetaDataOption{
                      .id = "component",
                      .name = "Datetime Component",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue = MetaDataOptionDefinition(std::string("year")),
                      .selectOption =
                          {
                              {"Year", "year"},
                              {"Month (1-12)", "month"},
                              {"Day of Month", "day"},
                              {"Hour (0-23)", "hour"},
                              {"Minute (0-59)", "minute"},
                              {"Second (0-59)", "second"},
                              {"Day of Week (0=Monday, 6=Sunday)", "day_of_week"},
                              {"Day of Year (1-366)", "day_of_year"},
                              {"Quarter (1-4)", "quarter"},
                              {"ISO Week Number", "week"},
                              {"Is Leap Year", "is_leap_year"},
                          },
                      .desc = "Select which datetime component to extract from bar timestamps"},
              },
          .isCrossSectional = false,
          .desc = "Extract a selected datetime component (year, month, day, hour, etc.) "
                  "from the bar index timestamps. No SLOT input required - "
                  "operates directly on the DataFrame index.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Integer, "value", "Datetime Component Value", false},
              },
          .atLeastOneInputRequired = false,
          .tags = {"datetime", "time-series", "calendar", "temporal"},
          .requiresTimeFrame = true,
          .requiredDataSources = {},
          .strategyTypes = {"calendar-based", "seasonal", "time-aware"},
          .assetRequirements = {},
          .usageContext =
              "Extract datetime components from bar timestamps for time-based "
              "filtering and analysis. Use for seasonal strategies, weekday effects, "
              "intraday patterns, or date-based filtering. "
              "Example: year = index_datetime_extract(component='year'); "
              "is_2020 = year.value == 2020",
          .limitations =
              "Operates on bar index only - cannot extract from custom timestamp columns. "
              "For custom timestamp columns, use column_datetime_extract instead. "
              "All outputs use ISO standards: weeks start Monday (0), months are 1-12.",
      });

  // 2. Column Datetime Extract - Extract datetime components from timestamp column
  metadataList.emplace_back(
      epoch_script::transforms::TransformsMetaData{
          .id = "column_datetime_extract",
          .category = epoch_core::TransformCategory::Utility,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Column Datetime Extract",
          .options =
              {
                  MetaDataOption{
                      .id = "component",
                      .name = "Datetime Component",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue = MetaDataOptionDefinition(std::string("year")),
                      .selectOption =
                          {
                              {"Year", "year"},
                              {"Month (1-12)", "month"},
                              {"Day of Month", "day"},
                              {"Hour (0-23)", "hour"},
                              {"Minute (0-59)", "minute"},
                              {"Second (0-59)", "second"},
                              {"Day of Week (0=Monday, 6=Sunday)", "day_of_week"},
                              {"Day of Year (1-366)", "day_of_year"},
                              {"Quarter (1-4)", "quarter"},
                              {"ISO Week Number", "week"},
                              {"Is Leap Year", "is_leap_year"},
                          },
                      .desc = "Select which datetime component to extract from the timestamp column"},
              },
          .isCrossSectional = false,
          .desc = "Extract a selected datetime component (year, month, day, hour, etc.) "
                  "from a timestamp column (SLOT input). Use this when you need "
                  "to analyze dates from data sources like observation_date or period_end.",
          .inputs =
              {
                  {epoch_core::IODataType::Timestamp, "SLOT", "Timestamp Column", false, false},
              },
          .outputs =
              {
                  {epoch_core::IODataType::Integer, "value", "Datetime Component Value", false},
              },
          .atLeastOneInputRequired = true,
          .tags = {"datetime", "fundamental-data", "calendar", "temporal"},
          .requiresTimeFrame = true,
          .requiredDataSources = {},
          .strategyTypes = {"fundamental-timing", "event-based", "earnings-calendar"},
          .assetRequirements = {},
          .usageContext =
              "Extract datetime components from timestamp columns like observation_date "
              "(FRED), period_end (fundamentals), or transaction_date (SEC). "
              "Use for fundamental timing strategies, earnings calendar analysis, "
              "or economic event filtering. "
              "Example: econ = economic_indicator(category=CPI); "
              "dt = column_datetime_extract(econ.observation_date, component='quarter'); "
              "q1_releases = dt.value == 1",
          .limitations =
              "Requires timestamp input - cannot operate on index. "
              "For bar timestamps, use index_datetime_extract instead. "
              "All outputs use ISO standards: weeks start Monday (0), months are 1-12.",
      });

  // 3. Timestamp Scalar - Create constant timestamp value
  metadataList.emplace_back(
      epoch_script::transforms::TransformsMetaData{
          .id = "timestamp_scalar",
          .category = epoch_core::TransformCategory::Utility,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Timestamp Scalar",
          .options =
              {
                  MetaDataOption{
                      .id = "value",
                      .name = "Timestamp Value",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("2020-01-01 00:00:00")),
                      .desc = "Timestamp in ISO format: 'YYYY-MM-DD', 'YYYY-MM-DD HH:MM:SS', or 'YYYY-MM-DDTHH:MM:SS'"},
              },
          .isCrossSectional = false,
          .desc = "Create a constant timestamp value for comparisons and filtering. "
                  "Supports ISO date/datetime formats. Useful for defining cutoff dates, "
                  "event timestamps, or reference points.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Timestamp, "value", "Timestamp Value", false},
              },
          .atLeastOneInputRequired = false,
          .tags = {"datetime", "scalar", "constant", "comparison"},
          .requiresTimeFrame = false,
          .requiredDataSources = {},
          .strategyTypes = {"event-based", "regime-change", "date-filtering"},
          .assetRequirements = {},
          .usageContext =
              "Create timestamp constants for date comparisons and event filtering. "
              "Use to filter data before/after specific dates, detect regime changes, "
              "or identify events. Supports date-only or full datetime. "
              "Example: cutoff = timestamp_scalar(value='2020-03-15'); "
              "recent = observation_date >= cutoff.value; "
              "covid_era = observation_date >= timestamp_scalar(value='2020-03-01').value",
          .limitations =
              "Timestamp string must be valid ISO format. Years must be 1970-2100. "
              "All times assumed UTC. Returns single scalar value (not time-series).",
      });

  // 4. Datetime Diff - Calculate time differences between timestamps
  metadataList.emplace_back(
      epoch_script::transforms::TransformsMetaData{
          .id = "datetime_diff",
          .category = epoch_core::TransformCategory::Utility,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Datetime Difference",
          .options =
              {
                  MetaDataOption{
                      .id = "unit",
                      .name = "Time Unit",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue = MetaDataOptionDefinition(std::string("days")),
                      .selectOption =
                          {
                              {"Days", "days"},
                              {"Hours", "hours"},
                              {"Minutes", "minutes"},
                              {"Seconds", "seconds"},
                              {"Milliseconds", "milliseconds"},
                              {"Microseconds", "microseconds"},
                              {"Weeks", "weeks"},
                              {"Months", "months"},
                              {"Quarters", "quarters"},
                              {"Years", "years"},
                          },
                      .desc = "Time unit for the difference calculation"},
              },
          .isCrossSectional = false,
          .desc = "Calculate time difference between two timestamp columns in the selected unit.",
          .inputs =
              {
                  {epoch_core::IODataType::Timestamp, "SLOT0", "First Timestamp (From)", false, false},
                  {epoch_core::IODataType::Timestamp, "SLOT1", "Second Timestamp (To)", false, false},
              },
          .outputs =
              {
                  {epoch_core::IODataType::Integer, "value", "Time Difference", false},
              },
          .atLeastOneInputRequired = true,
          .tags = {"datetime", "difference", "duration", "temporal"},
          .requiresTimeFrame = true,
          .requiredDataSources = {},
          .strategyTypes = {"fundamental-timing", "event-lag", "recency"},
          .assetRequirements = {},
          .usageContext =
              "Calculate time elapsed between events for recency analysis, lag detection, "
              "or timing strategies. Use with fundamental data to measure time since earnings, "
              "economic releases, or corporate events. "
              "Example: insider = sec_insider_trading(); "
              "diff = datetime_diff(insider.transaction_date, insider.period_end, unit='days'); "
              "recent_trades = diff.value <= 30",
          .limitations =
              "Requires two timestamp columns as input. Result is SLOT1 - SLOT0. "
              "Negative values indicate SLOT0 is after SLOT1. "
              "Month/quarter/year differences may not be exact calendar periods.",
      });

  return metadataList;
}

} // namespace epoch_script::transform
