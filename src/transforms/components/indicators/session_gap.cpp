//
// Created by assistant on 9/1/24.
//

#include "session_gap.h"
#include <arrow/builder.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/index.h>
#include <epoch_script/core/bar_attribute.h>

namespace epoch_script::transform {

  struct ActiveGap {
    bool exists = false;
    double prior_close = 0.0;
    double gap_open = 0.0;  // Store the opening price when gap occurred
    double gap_abs = 0.0;  // Absolute gap size for fill calculations
    double gap_pct = 0.0;  // Signed gap percentage (+ up, - down)
    int64_t gap_day = -1;
    int64_t prior_close_timestamp = 0;
    bool filled = false;  // Track if gap has been filled
  };

static inline int64_t floor_to_day(std::optional<int64_t> const &timestamp) {
  return static_cast<int64_t>(std::floor(*timestamp / 86400000000000));
}

epoch_frame::DataFrame
SessionGap::TransformData(epoch_frame::DataFrame const &bars) const {
  using namespace epoch_frame;
  const auto &C = EpochStratifyXConstants::instance();

  auto t = bars.index()->array().to_timestamp_view();
  auto open = bars[C.OPEN()].contiguous_array().to_view<double>();
  auto high = bars[C.HIGH()].contiguous_array().to_view<double>();
  auto low = bars[C.LOW()].contiguous_array().to_view<double>();
  auto close = bars[C.CLOSE()].contiguous_array().to_view<double>();

  const int64_t n = open->length();

  // Arrow builders for nullable arrays
  arrow::BooleanBuilder gap_filled_builder;
  arrow::DoubleBuilder gap_retrace_builder;
  arrow::DoubleBuilder gap_size_builder;
  arrow::DoubleBuilder psc_builder;
  arrow::TimestampBuilder psc_timestamp_builder(
      arrow::timestamp(arrow::TimeUnit::NANO, "UTC"),
      arrow::default_memory_pool());

  // Reserve space for optimal performance
  (void)gap_filled_builder.Reserve(n);
  (void)gap_retrace_builder.Reserve(n);
  (void)gap_size_builder.Reserve(n);
  (void)psc_builder.Reserve(n);
  (void)psc_timestamp_builder.Reserve(n);

  // First pass: append null for index 0
  gap_filled_builder.UnsafeAppendNull();
  gap_retrace_builder.UnsafeAppendNull();
  gap_size_builder.UnsafeAppendNull();
  psc_builder.UnsafeAppendNull();
  psc_timestamp_builder.UnsafeAppendNull();

  if (n > 0) {
    // Track active gap for the current day
    ActiveGap active_gap;

    auto timeIt = t->begin() + 1;
    auto openIt = open->begin() + 1;
    auto highIt = high->begin() + 1;
    auto lowIt = low->begin() + 1;
    auto prevCloseIt = close->begin();

    for (int64_t i = 1; i < n; ++i) {
      const int64_t current_day = floor_to_day(*timeIt);
      const bool new_day = floor_to_day(*(timeIt - 1)) != current_day;

      // Check if we're starting a new day with a gap
      if (new_day && *openIt && *prevCloseIt) {
        const double o = **openIt;
        const double pc = **prevCloseIt;

        if (std::isfinite(o) && std::isfinite(pc) && o != pc) {
          // New gap detected at market open
          active_gap.exists = true;
          active_gap.prior_close = pc;
          active_gap.gap_open = o;  // Store the gap open price
          active_gap.gap_abs = std::abs(o - pc);
          active_gap.gap_pct = ((o - pc) / pc) * 100.0;  // Signed percentage
          active_gap.gap_day = current_day;
          active_gap.prior_close_timestamp = **(timeIt - 1);
          active_gap.filled = false;

          // Record gap size only at market open (signed: + up, - down)
          gap_size_builder.UnsafeAppend(active_gap.gap_pct);
          psc_builder.UnsafeAppend(pc);
          psc_timestamp_builder.UnsafeAppend(active_gap.prior_close_timestamp);

          // Check if gap is filled in the opening bar
          double gap_retrace = 0.0;

          if (*highIt && *lowIt) {
            const double h = **highIt;
            const double l = **lowIt;

            if (active_gap.gap_pct > 0) {
              // Up gap: calculate fill fraction
              if (l < o) {
                gap_retrace = (o - l) / active_gap.gap_abs;
              }
            } else {
              // Down gap: calculate fill fraction
              if (h > o) {
                gap_retrace = (h - o) / active_gap.gap_abs;
              }
            }

            // Check if threshold is met
            if (!active_gap.filled && gap_retrace >= m_fillPercent) {
              active_gap.filled = true;
            }
          }

          gap_filled_builder.UnsafeAppend(active_gap.filled);
          gap_retrace_builder.UnsafeAppend(gap_retrace);
        } else {
          // No gap at market open
          active_gap.exists = false;
          gap_filled_builder.UnsafeAppendNull();
          gap_retrace_builder.UnsafeAppendNull();
          gap_size_builder.UnsafeAppendNull();
          psc_builder.UnsafeAppendNull();
          psc_timestamp_builder.UnsafeAppendNull();
        }
      } else if (active_gap.exists && current_day == active_gap.gap_day) {
        // We have an active gap from today - continue tracking
        gap_size_builder.UnsafeAppendNull();  // Gap size only at open

        // Keep psc and timestamp available throughout the day
        psc_builder.UnsafeAppend(active_gap.prior_close);
        psc_timestamp_builder.UnsafeAppend(active_gap.prior_close_timestamp);

        // Continue calculating retrace throughout the day for statistics
        double gap_retrace = 0.0;

        if (*highIt && *lowIt) {
          const double h = **highIt;
          const double l = **lowIt;

          if (active_gap.gap_pct > 0) {
            // Up gap: calculate cumulative fill fraction
            if (l < active_gap.gap_open) {
              gap_retrace = (active_gap.gap_open - l) / active_gap.gap_abs;
            }
          } else {
            // Down gap: calculate cumulative fill fraction
            if (h > active_gap.gap_open) {
              gap_retrace = (h - active_gap.gap_open) / active_gap.gap_abs;
            }
          }

          // Check if threshold is met (only once)
          if (!active_gap.filled && gap_retrace >= m_fillPercent) {
            active_gap.filled = true;
          }

          gap_filled_builder.UnsafeAppend(active_gap.filled);
          gap_retrace_builder.UnsafeAppend(gap_retrace);
        } else {
          gap_filled_builder.UnsafeAppend(active_gap.filled);
          gap_retrace_builder.UnsafeAppendNull();
        }
      } else {
        // No active gap or different day - append nulls
        gap_filled_builder.UnsafeAppendNull();
        gap_retrace_builder.UnsafeAppendNull();
        gap_size_builder.UnsafeAppendNull();
        psc_builder.UnsafeAppendNull();
        psc_timestamp_builder.UnsafeAppendNull();
      }

      ++timeIt;
      ++openIt;
      ++highIt;
      ++lowIt;
      ++prevCloseIt;
    }
  } else {
    // Handle case where n == 0 - no additional entries needed
  }

  // Build the Arrow arrays and immediately wrap in ChunkedArray for safety
  std::shared_ptr<arrow::Array> gap_filled_array_temp;
  std::shared_ptr<arrow::Array> gap_retrace_array_temp;
  std::shared_ptr<arrow::Array> gap_size_array_temp;
  std::shared_ptr<arrow::Array> psc_array_temp;
  std::shared_ptr<arrow::Array> psc_timestamp_array_temp;

  (void)gap_filled_builder.Finish(&gap_filled_array_temp);
  (void)gap_retrace_builder.Finish(&gap_retrace_array_temp);
  (void)gap_size_builder.Finish(&gap_size_array_temp);
  (void)psc_builder.Finish(&psc_array_temp);
  (void)psc_timestamp_builder.Finish(&psc_timestamp_array_temp);

  // Create chunked arrays immediately for safe access
  auto gap_filled_chunked =
      std::make_shared<arrow::ChunkedArray>(gap_filled_array_temp);
  auto gap_retrace_chunked =
      std::make_shared<arrow::ChunkedArray>(gap_retrace_array_temp);
  auto gap_size_chunked = std::make_shared<arrow::ChunkedArray>(gap_size_array_temp);
  auto psc_chunked = std::make_shared<arrow::ChunkedArray>(psc_array_temp);
  auto psc_timestamp_chunked =
      std::make_shared<arrow::ChunkedArray>(psc_timestamp_array_temp);

  // Construct output dataframe
  auto df = epoch_frame::make_dataframe(
      bars.index(),
      {gap_filled_chunked, gap_retrace_chunked,
       gap_size_chunked, psc_chunked, psc_timestamp_chunked},
      {GetOutputId("gap_filled"),
       GetOutputId("gap_retrace"), GetOutputId("gap_size"),
       GetOutputId("psc"), GetOutputId("psc_timestamp")});

  return df;
}

} // namespace epoch_script::transform
