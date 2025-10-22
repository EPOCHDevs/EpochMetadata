//
// Created for generic calendar/seasonality effects
//

#pragma once

#include <epoch_core/enum_wrapper.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/index.h>
#include <epoch_metadata/transforms/itransform.h>
#include <arrow/compute/api.h>

CREATE_ENUM(CalendarEffectType,
            TurnOfMonth,     // Last N / First N trading days of month
            DayOfWeek,       // Specific weekday (Monday effect, Friday effect, etc.)
            MonthOfYear,     // Specific month (January effect, etc.)
            Quarter,         // Specific quarter
            Holiday,         // Days before/after holidays
            WeekOfMonth);    // First/last week of month

namespace epoch_metadata::transform
{

    /**
     * @brief Generic Calendar Effect Transform
     *
     * Handles all calendar-based trading anomalies through configuration:
     * - Turn of Month: Trading near month boundaries
     * - Day of Week: Monday/Friday effects
     * - Month of Year: January effect, seasonal patterns
     * - Quarter: Quarter-end effects
     * - Holiday Effects: Before/after holidays
     * - Week of Month: First/last week patterns
     *
     * @tparam effect_type The calendar effect type to apply
     */
    template <epoch_core::CalendarEffectType effect_type>
    class CalendarEffect : public ITransform
    {
      public:
        explicit CalendarEffect(const TransformConfiguration& config);

        epoch_frame::DataFrame TransformData(const epoch_frame::DataFrame& bars) const override;

      private:
        // Configuration parameters (vary by effect type)
        int64_t m_days_before{0};      // For turn-of-month, holiday effects
        int64_t m_days_after{0};       // For turn-of-month, holiday effects
        int64_t m_target_value{0};     // For day-of-week (0-6), month (1-12), quarter (1-4)
        std::string m_country{"US"};   // For holiday calendar

        // Helper methods for each effect type
        epoch_frame::Series ApplyTurnOfMonth(const epoch_frame::DataFrame& bars) const;
        epoch_frame::Series ApplyDayOfWeek(const epoch_frame::DataFrame& bars) const;
        epoch_frame::Series ApplyMonthOfYear(const epoch_frame::DataFrame& bars) const;
        epoch_frame::Series ApplyQuarter(const epoch_frame::DataFrame& bars) const;
        epoch_frame::Series ApplyHoliday(const epoch_frame::DataFrame& bars) const;
        epoch_frame::Series ApplyWeekOfMonth(const epoch_frame::DataFrame& bars) const;
    };

    // Concrete type aliases for each effect
    using TurnOfMonthEffect = CalendarEffect<epoch_core::CalendarEffectType::TurnOfMonth>;
    using DayOfWeekEffect = CalendarEffect<epoch_core::CalendarEffectType::DayOfWeek>;
    using MonthOfYearEffect = CalendarEffect<epoch_core::CalendarEffectType::MonthOfYear>;
    using QuarterEffect = CalendarEffect<epoch_core::CalendarEffectType::Quarter>;
    using HolidayEffect = CalendarEffect<epoch_core::CalendarEffectType::Holiday>;
    using WeekOfMonthEffect = CalendarEffect<epoch_core::CalendarEffectType::WeekOfMonth>;

} // namespace epoch_metadata::transform
