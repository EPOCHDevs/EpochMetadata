//
// Created by assistant on 10/30/25.
//

#include "bar_gap.h"
#include <arrow/builder.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/index.h>
#include <epoch_metadata/bar_attribute.h>

namespace epoch_metadata::transform {

epoch_frame::DataFrame
BarGap::TransformData(epoch_frame::DataFrame const &bars) const {
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

  // First bar: append null (no previous bar to compare)
  gap_filled_builder.UnsafeAppendNull();
  gap_retrace_builder.UnsafeAppendNull();
  gap_size_builder.UnsafeAppendNull();
  psc_builder.UnsafeAppendNull();
  psc_timestamp_builder.UnsafeAppendNull();

  if (n > 0) {
    auto timeIt = t->begin() + 1;
    auto openIt = open->begin() + 1;
    auto highIt = high->begin() + 1;
    auto lowIt = low->begin() + 1;
    auto prevCloseIt = close->begin();
    auto prevTimeIt = t->begin();

    for (int64_t i = 1; i < n; ++i) {
      // Check for gap between current open and previous close
      // NO day boundary check - any consecutive bars
      if (*openIt && *prevCloseIt) {
        const double o = **openIt;
        const double pc = **prevCloseIt;

        if (std::isfinite(o) && std::isfinite(pc) && o != pc) {
          // Calculate gap size
          const double gap_abs = std::abs(o - pc);
          const double gap_pct = ((o - pc) / pc) * 100.0;  // Signed percentage

          // Check if gap exceeds minimum threshold
          if (std::abs(gap_pct) >= m_minGapSize) {
            // Gap detected
            gap_size_builder.UnsafeAppend(gap_pct);
            psc_builder.UnsafeAppend(pc);
            psc_timestamp_builder.UnsafeAppend(**prevTimeIt);

            // Check if gap is filled in current bar (immediate fill only)
            double gap_retrace = 0.0;
            bool filled = false;

            if (*highIt && *lowIt) {
              const double h = **highIt;
              const double l = **lowIt;

              if (gap_pct > 0) {
                // Up gap: check if low touched previous close
                if (l <= pc) {
                  gap_retrace = (o - l) / gap_abs;
                }
              } else {
                // Down gap: check if high touched previous close
                if (h >= pc) {
                  gap_retrace = (h - o) / gap_abs;
                }
              }

              // Check if threshold is met
              if (gap_retrace >= m_fillPercent) {
                filled = true;
              }
            }

            gap_filled_builder.UnsafeAppend(filled);
            gap_retrace_builder.UnsafeAppend(gap_retrace);
          } else {
            // Gap too small, below threshold
            gap_filled_builder.UnsafeAppendNull();
            gap_retrace_builder.UnsafeAppendNull();
            gap_size_builder.UnsafeAppendNull();
            psc_builder.UnsafeAppendNull();
            psc_timestamp_builder.UnsafeAppendNull();
          }
        } else {
          // No gap (open == prev close)
          gap_filled_builder.UnsafeAppendNull();
          gap_retrace_builder.UnsafeAppendNull();
          gap_size_builder.UnsafeAppendNull();
          psc_builder.UnsafeAppendNull();
          psc_timestamp_builder.UnsafeAppendNull();
        }
      } else {
        // Missing data
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
      ++prevTimeIt;
    }
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

} // namespace epoch_metadata::transform
