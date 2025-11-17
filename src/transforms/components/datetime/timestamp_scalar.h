#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/datetime.h>
#include <arrow/builder.h>
#include <regex>
#include <stdexcept>

namespace epoch_script::transform {

// Helper function to create single-row index from input bars (imported from scalar.h pattern)
extern epoch_frame::IndexPtr CreateScalarIndex(const epoch_frame::DataFrame& bars);

/**
 * TimestampScalar - Create a constant timestamp value
 *
 * Creates a single timestamp scalar from a strict datetime string format.
 * Useful for timestamp comparisons and filtering.
 *
 * Format: "YYYY-MM-DD HH:MM:SS" (strict, space-separated, 24-hour time)
 *
 * Example usage:
 *   cutoff = timestamp_scalar(value="2020-01-01 00:00:00")
 *   recent = observation_date >= cutoff.value
 *
 *   event_time = timestamp_scalar(value="2021-03-15 14:30:00")
 *   during_event = trade_time == event_time.value
 */
class TimestampScalar : public ITransform {
public:
  explicit TimestampScalar(const TransformConfiguration &config)
      : ITransform(config),
        m_timestamp_string(config.GetOptionValue("value").GetString()),
        m_timestamp_nanos(ParseTimestampString(m_timestamp_string)) {}


  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    // Use last timestamp from input bars as the scalar's timestamp
    auto singleRowIndex = CreateScalarIndex(bars);

    // Build single-element timestamp array
    arrow::TimestampBuilder builder(
        arrow::timestamp(arrow::TimeUnit::NANO, "UTC"),
        arrow::default_memory_pool());

    auto status = builder.Append(m_timestamp_nanos);
    if (!status.ok()) {
      throw std::runtime_error("Failed to build timestamp scalar: " + status.ToString());
    }

    auto array_result = builder.Finish();
    if (!array_result.ok()) {
      throw std::runtime_error("Failed to finish timestamp array: " + array_result.status().ToString());
    }

    auto chunkedArray = std::make_shared<arrow::ChunkedArray>(array_result.MoveValueUnsafe());
    return epoch_frame::make_dataframe(singleRowIndex, {chunkedArray}, {GetOutputId()});
  }

private:
  std::string m_timestamp_string;
  int64_t m_timestamp_nanos;

  /**
   * Parse ISO format timestamp string to nanoseconds since epoch
   *
   * Supports:
   *  - YYYY-MM-DD
   *  - YYYY-MM-DD HH:MM:SS
   *  - YYYY-MM-DDTHH:MM:SS
   */
  static int64_t ParseTimestampString(const std::string& timestamp_str) {
    try {
      std::string normalized_str = timestamp_str;

      // If date-only format (YYYY-MM-DD), append time component
      if (timestamp_str.length() == 10 && timestamp_str[4] == '-' && timestamp_str[7] == '-') {
        normalized_str = timestamp_str + " 00:00:00";
      }

      // from_str expects strict "YYYY-MM-DD HH:MM:SS" format in UTC
      epoch_frame::DateTime dt = epoch_frame::DateTime::from_str(normalized_str, "UTC");
      return dt.m_nanoseconds.count();
    } catch (const std::exception& e) {
      throw std::runtime_error(
          "Invalid timestamp format: '" + timestamp_str + "'. " +
         "Expected formats: 'YYYY-MM-DD', 'YYYY-MM-DD HH:MM:SS', or 'YYYY-MM-DDTHH:MM:SS'. " +
     "Error: " + e.what());
    }
  }
};

} // namespace epoch_script::transform
