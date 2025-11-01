//
// Created for generic calendar/seasonality effects
//

#include "calendar_effect.h"
#include <arrow/compute/api.h>
#include <epoch_frame/calendar_common.h>
#include <epoch_frame/scalar.h>
#include <methods/temporal.h>
#include <unordered_map>

namespace epochflow::transform
{
    // Helper functions for string-to-integer conversions
    namespace {
        int64_t WeekdayStringToInt(const std::string& weekday) {
            static const std::unordered_map<std::string, int64_t> weekday_map = {
                {"Monday", 0}, {"Tuesday", 1}, {"Wednesday", 2},
                {"Thursday", 3}, {"Friday", 4}, {"Saturday", 5}, {"Sunday", 6}
            };
            auto it = weekday_map.find(weekday);
            return (it != weekday_map.end()) ? it->second : 0;
        }

        int64_t MonthStringToInt(const std::string& month) {
            static const std::unordered_map<std::string, int64_t> month_map = {
                {"January", 1}, {"February", 2}, {"March", 3}, {"April", 4},
                {"May", 5}, {"June", 6}, {"July", 7}, {"August", 8},
                {"September", 9}, {"October", 10}, {"November", 11}, {"December", 12}
            };
            auto it = month_map.find(month);
            return (it != month_map.end()) ? it->second : 1;
        }

        int64_t QuarterStringToInt(const std::string& quarter) {
            static const std::unordered_map<std::string, int64_t> quarter_map = {
                {"Q1", 1}, {"Q2", 2}, {"Q3", 3}, {"Q4", 4}
            };
            auto it = quarter_map.find(quarter);
            return (it != quarter_map.end()) ? it->second : 1;
        }

        int64_t WeekStringToInt(const std::string& week) {
            static const std::unordered_map<std::string, int64_t> week_map = {
                {"First", 1}, {"Second", 2}, {"Third", 3}, {"Fourth", 4}, {"Last", 5}
            };
            auto it = week_map.find(week);
            return (it != week_map.end()) ? it->second : 1;
        }
    }

    template <epoch_core::CalendarEffectType effect_type>
    CalendarEffect<effect_type>::CalendarEffect(const TransformConfiguration& config)
        : ITransform(config)
    {
        // Load configuration based on effect type
        if constexpr (effect_type == epoch_core::CalendarEffectType::TurnOfMonth)
        {
            m_days_before = config.GetOptionValue("days_before").GetInteger();
            m_days_after = config.GetOptionValue("days_after").GetInteger();
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::DayOfWeek)
        {
            // Get weekday as string and convert to integer
            auto weekday_str = config.GetOptionValue("weekday").GetSelectOption();
            m_target_value = WeekdayStringToInt(weekday_str);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::MonthOfYear)
        {
            // Get month as string and convert to integer
            auto month_str = config.GetOptionValue("month").GetSelectOption();
            m_target_value = MonthStringToInt(month_str);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::Quarter)
        {
            // Get quarter as string and convert to integer
            auto quarter_str = config.GetOptionValue("quarter").GetSelectOption();
            m_target_value = QuarterStringToInt(quarter_str);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::Holiday)
        {
            m_days_before = config.GetOptionValue("days_before").GetInteger();
            m_days_after = config.GetOptionValue("days_after").GetInteger();
            m_country = config.GetOptionValue("country").GetSelectOption();
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::WeekOfMonth)
        {
            // Get week as string and convert to integer
            auto week_str = config.GetOptionValue("week").GetSelectOption();
            m_target_value = WeekStringToInt(week_str);
        }
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::DataFrame
    CalendarEffect<effect_type>::TransformData(const epoch_frame::DataFrame& bars) const
    {
        epoch_frame::Series result;

        if constexpr (effect_type == epoch_core::CalendarEffectType::TurnOfMonth)
        {
            result = ApplyTurnOfMonth(bars);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::DayOfWeek)
        {
            result = ApplyDayOfWeek(bars);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::MonthOfYear)
        {
            result = ApplyMonthOfYear(bars);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::Quarter)
        {
            result = ApplyQuarter(bars);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::Holiday)
        {
            result = ApplyHoliday(bars);
        }
        else if constexpr (effect_type == epoch_core::CalendarEffectType::WeekOfMonth)
        {
            result = ApplyWeekOfMonth(bars);
        }

        return result.to_frame(GetOutputId());
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyTurnOfMonth(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Extract day of month using TemporalOperation
        TemporalOperation<true> temporal(bars.index()->array());
        auto day_array = temporal.day();
        Series day_series(bars.index(), day_array.as_chunked_array());

        // Extract month to detect month boundaries
        auto month_array = temporal.month();
        Series month_series(bars.index(), month_array.as_chunked_array());

        // Shift month forward by 1 to find next month
        auto next_month = month_series.shift(-1);

        // Days before end of month: check if month changes in next row
        auto is_near_end = (month_series != next_month);

        // For days_before, we need last N days of month
        // Create rolling window to check if we're in last N days
        Series mask = Series(bars.index(),
                             factory::array::make_array(std::vector<bool>(bars.size(), false)));

        if (m_days_before > 0)
        {
            // Mark last m_days_before days of each month
            for (int64_t i = 1; i <= m_days_before && i < static_cast<int64_t>(bars.size()); ++i)
            {
                auto shifted_month_change = is_near_end.shift(i);
                mask = mask || shifted_month_change;
            }
        }

        if (m_days_after > 0)
        {
            // Mark first m_days_after days of each month
            auto prev_month = month_series.shift(1);
            auto is_month_start = (month_series != prev_month);

            for (int64_t i = 0; i < m_days_after && i < static_cast<int64_t>(bars.size()); ++i)
            {
                auto shifted_month_start = is_month_start.shift(-i);
                mask = mask || shifted_month_start;
            }
        }

        return mask;
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyDayOfWeek(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Extract day of week (0=Monday, 6=Sunday by default with ISO calendar)
        arrow::compute::DayOfWeekOptions dow_options;
        dow_options.count_from_zero = true;     // 0-6 instead of 1-7
        dow_options.week_start = 1;             // Monday = 1 (ISO standard)

        TemporalOperation<true> temporal(bars.index()->array());
        auto dow_array = temporal.day_of_week(dow_options);
        Series dow_series(bars.index(), dow_array.as_chunked_array());

        // Create mask for target weekday
        auto mask = (dow_series == Scalar(MakeScalar(static_cast<int64_t>(m_target_value))));

        return mask;
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyMonthOfYear(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Extract month (1-12)
        TemporalOperation<true> temporal(bars.index()->array());
        auto month_array = temporal.month();
        Series month_series(bars.index(), month_array.as_chunked_array());

        // Create mask for target month
        auto mask = (month_series == Scalar(MakeScalar(static_cast<int64_t>(m_target_value))));

        return mask;
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyQuarter(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Extract quarter (1-4)
        TemporalOperation<true> temporal(bars.index()->array());
        auto quarter_array = temporal.quarter();
        Series quarter_series(bars.index(), quarter_array.as_chunked_array());

        // Create mask for target quarter
        auto mask = (quarter_series == Scalar(MakeScalar(static_cast<int64_t>(m_target_value))));

        return mask;
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyHoliday(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Initialize mask with all false
        std::vector<bool> mask_data(bars.size(), false);

        try {
            // Get the holiday calendar from registry
            auto holiday_cal = calendar::getHolidayCalendar(m_country);

            // Get the date range from the index
            if (bars.size() == 0) {
                return Series(bars.index(),
                             factory::array::make_array(mask_data));
            }

            // Extract start and end dates from index
            auto first_ts = bars.index()->at(0);
            auto last_ts = bars.index()->at(bars.size() - 1);

            // Convert scalars to DateTime
            auto first_val = first_ts.value<int64_t>();
            auto last_val = last_ts.value<int64_t>();
            if (!first_val || !last_val) {
                return Series(bars.index(), factory::array::make_array(mask_data));
            }
            auto start_dt = DateTime(first_val.value());
            auto end_dt = DateTime(last_val.value());

            // Get holidays in this date range
            auto holidays_index = holiday_cal->holidays(start_dt, end_dt);

            if (holidays_index && holidays_index->size() > 0) {
                // Convert holidays to a set for fast lookup
                auto holidays_vec = holidays_index->to_vector<int64_t>();
                std::unordered_set<int64_t> holiday_set(holidays_vec.begin(), holidays_vec.end());

                // Mark days before/after holidays
                for (size_t i = 0; i < bars.size(); ++i) {
                    auto bar_ts = bars.index()->at(i);
                    auto bar_time_opt = bar_ts.value<int64_t>();
                    if (!bar_time_opt) continue;
                    auto bar_time = bar_time_opt.value();

                    // Check if this bar is within the window around any holiday
                    for (int64_t offset = -m_days_before; offset <= m_days_after; ++offset) {
                        if (offset == 0) continue;  // Skip the holiday itself

                        // Calculate the offset timestamp
                        int64_t offset_ts = bar_time + (offset * 86400000000000LL); // nanoseconds in a day

                        // Check if this offset lands on a holiday
                        if (holiday_set.count(offset_ts) > 0) {
                            mask_data[i] = true;
                            break;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            // If calendar not found or other error, return all false
            // This allows the transform to work even without a registered calendar
        }

        auto mask = Series(bars.index(), factory::array::make_array(mask_data));

        return mask;
    }

    template <epoch_core::CalendarEffectType effect_type>
    epoch_frame::Series
    CalendarEffect<effect_type>::ApplyWeekOfMonth(const epoch_frame::DataFrame& bars) const
    {
        using namespace epoch_frame;

        // Extract day of month
        TemporalOperation<true> temporal(bars.index()->array());
        auto day_array = temporal.day();
        Series day_series(bars.index(), day_array.as_chunked_array());

        // Calculate week of month (1-5): week = ceil(day / 7)
        auto week_of_month = ((day_series - Scalar(MakeScalar(1.0))) / Scalar(MakeScalar(7.0))).ceil() + Scalar(MakeScalar(1.0));

        // Create mask for target week
        auto mask = (week_of_month == Scalar(MakeScalar(static_cast<int64_t>(m_target_value))));

        return mask;
    }

    // Explicit template instantiations
    template class CalendarEffect<epoch_core::CalendarEffectType::TurnOfMonth>;
    template class CalendarEffect<epoch_core::CalendarEffectType::DayOfWeek>;
    template class CalendarEffect<epoch_core::CalendarEffectType::MonthOfYear>;
    template class CalendarEffect<epoch_core::CalendarEffectType::Quarter>;
    template class CalendarEffect<epoch_core::CalendarEffectType::Holiday>;
    template class CalendarEffect<epoch_core::CalendarEffectType::WeekOfMonth>;

} // namespace epochflow::transform
