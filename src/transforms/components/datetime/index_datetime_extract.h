#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <methods/temporal.h>
#include <epoch_core/enum_wrapper.h>

CREATE_ENUM(DatetimeComponent, year, month, day, hour, minute, second, day_of_week, day_of_year, quarter, week, is_leap_year);

namespace epoch_script::transform {

/**
 * DatetimeExtract - Extract a datetime component from timestamps
 *
 * Template parameter UseIndex controls the timestamp source:
 * - UseIndex=true: Operates on DataFrame index (no SLOT input)
 * - UseIndex=false: Operates on SLOT timestamp column input
 *
 * Example usage:
 *   year = index_datetime_extract(component="year")
 *   is_2020 = year.value == 2020
 *
 *   econ = economic_indicator(category=CPI)
 *   dt = column_datetime_extract(econ.observation_date, component="month")
 *   jan_only = dt.value == 1
 */
template<bool UseIndex>
class DatetimeExtract : public ITransform {
public:
  explicit DatetimeExtract(const TransformConfiguration &config)
      : ITransform(config),
        m_component(config.GetOptionValue("component").GetSelectOption<epoch_core::DatetimeComponent>()) {}

private:
  epoch_core::DatetimeComponent m_component;

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;

    // Get timestamp array based on template parameter
    Array timestamp_array;
    if constexpr (UseIndex) {
      timestamp_array = bars.index()->array();
    } else {
      timestamp_array = bars[GetInputId(ARG)].contiguous_array();
    }

    // Extract datetime components using TemporalOperation
    TemporalOperation<true> temporal(timestamp_array);

    // Extract selected component
    std::shared_ptr<arrow::Array> result;
    arrow::DataTypePtr result_type;

    switch (m_component) {
      case epoch_core::DatetimeComponent::Null:
        // Should never happen - enum has no Null value
        throw std::runtime_error("Invalid datetime component: Null");
        break;
      case epoch_core::DatetimeComponent::year:
        result = temporal.year().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::month:
        result = temporal.month().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::day:
        result = temporal.day().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::hour:
        result = temporal.hour().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::minute:
        result = temporal.minute().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::second:
        result = temporal.second().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::day_of_week: {
        arrow::compute::DayOfWeekOptions dow_options;
        dow_options.count_from_zero = true;  // 0=Monday, 6=Sunday (ISO)
        dow_options.week_start = 1;           // Monday = 1 (ISO standard)
        result = temporal.day_of_week(dow_options).value();
        result_type = arrow::int64();
        break;
      }
      case epoch_core::DatetimeComponent::day_of_year:
        result = temporal.day_of_year().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::quarter:
        result = temporal.quarter().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::week:
        result = temporal.iso_week().value();
        result_type = arrow::int64();
        break;
      case epoch_core::DatetimeComponent::is_leap_year:
        result = temporal.is_leap_year().value();
        result_type = arrow::boolean();
        break;
    }

    // Build output schema with single value
    auto schema = arrow::schema({{GetOutputId("value"), result_type}});
    std::vector<std::shared_ptr<arrow::Array>> arrays = {result};

    return AssertTableResultIsOk(arrow::Table::Make(schema, arrays));
  }
};

// Type aliases for the two instantiations
using IndexDatetimeExtract = DatetimeExtract<true>;
using ColumnDatetimeExtract = DatetimeExtract<false>;

} // namespace epoch_script::transform
