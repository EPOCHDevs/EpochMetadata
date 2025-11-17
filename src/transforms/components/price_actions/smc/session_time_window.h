#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/core/time_frame.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include "date_time/date_offsets.h"

namespace epoch_script::transform {

/**
 * SessionTimeWindow - Detects when bars are exactly X minutes from session boundaries
 *
 * Returns true when a bar's timestamp equals the computed offset from session start/end
 */
class SessionTimeWindow : public ITransform {
public:
  explicit SessionTimeWindow(const TransformConfiguration &config)
      : ITransform(config),
        m_session_type(config.GetOptionValue("session_type").GetSelectOption<epoch_core::SessionType>()),
        m_minute_offset(config.GetOptionValue("minute_offset").GetInteger()),
        m_boundary_type(config.GetOptionValue("boundary_type").GetString()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    return epoch_frame::DataFrame{df.index(), Call(df)};
  }

  arrow::TablePtr Call(epoch_frame::DataFrame const &bars) const {
    using namespace epoch_frame;

    const size_t N = bars.num_rows();
    std::vector<bool> in_window(N, false);

    // Get the index (timestamps) from the dataframe
    auto index = bars.index();

    // Build session range from session_type
    epoch_frame::SessionRange range = epoch_script::kSessionRegistry.at(m_session_type);

    // Convert session times to UTC and compute boundaries
    const auto startHMS = range.start;
    const auto endHMS = range.end;

    // Cache for current session boundary
    std::optional<int64_t> cachedDate;
    int64_t targetTimestamp = 0;

    for (size_t i = 0; i < N; ++i) {
      auto dt = index->at(i);
      int64_t dtUTC = dt.timestamp().value;

      // Extract date component (UTC date at midnight)
      int64_t currentDate = (dtUTC / 86400000000LL) * 86400000000LL;

      // If date changed, recompute session boundaries
      if (!cachedDate || *cachedDate != currentDate) {
        cachedDate = currentDate;

        // Compute session start/end in UTC
        int64_t sessionStartUTC = currentDate +
          (startHMS.hour.count() * 3600LL + startHMS.minute.count() * 60LL + startHMS.second.count()) * 1000000LL;
        int64_t sessionEndUTC = currentDate +
          (endHMS.hour.count() * 3600LL + endHMS.minute.count() * 60LL + endHMS.second.count()) * 1000000LL;

        // Handle session crossing midnight
        if (sessionEndUTC <= sessionStartUTC) {
          sessionEndUTC += 86400000000LL; // Add 24 hours
        }

        // Apply minute offset based on boundary_type
        int64_t offsetMicroseconds = m_minute_offset * 60LL * 1000000LL;

        if (m_boundary_type == "start") {
          // X minutes from session start
          targetTimestamp = sessionStartUTC + offsetMicroseconds;
        } else if (m_boundary_type == "end") {
          // X minutes before session end (negative offset)
          targetTimestamp = sessionEndUTC - offsetMicroseconds;
        } else {
          throw std::runtime_error("Invalid boundary_type: must be 'start' or 'end'");
        }
      }

      // Check if current bar timestamp matches target
      in_window[i] = (dtUTC == targetTimestamp);
    }

    return AssertTableResultIsOk(arrow::Table::Make(
        arrow::schema({{GetOutputId("value"), arrow::boolean()}}),
        {factory::array::make_array(in_window)}));
  }

private:
  epoch_core::SessionType m_session_type;
  int64_t m_minute_offset;
  std::string m_boundary_type;
};

} // namespace epoch_script::transform
