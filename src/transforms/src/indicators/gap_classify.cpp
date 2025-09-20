//
// Created by assistant on 9/1/24.
//

#include "gap_classify.h"
#include <arrow/builder.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/index.h>
#include <epoch_metadata/bar_attribute.h>

namespace epoch_metadata::transform {

static inline int64_t floor_to_day(std::optional<int64_t> const &timestamp) {
  return static_cast<int64_t>(std::floor(*timestamp / 86400000000000));
}

epoch_frame::DataFrame
GapClassify::TransformData(epoch_frame::DataFrame const &bars) const {
  using namespace epoch_frame;
  const auto &C = EpochStratifyXConstants::instance();

  auto t = bars.index()->array().to_timestamp_view();
  auto open = bars[C.OPEN()].contiguous_array().to_view<double>();
  auto high = bars[C.HIGH()].contiguous_array().to_view<double>();
  auto low = bars[C.LOW()].contiguous_array().to_view<double>();
  auto close = bars[C.CLOSE()].contiguous_array().to_view<double>();

  const int64_t n = open->length();

  // Arrow builders for nullable arrays
  arrow::BooleanBuilder gap_up_builder;
  arrow::BooleanBuilder gap_filled_builder;
  arrow::DoubleBuilder fill_fraction_builder;
  arrow::DoubleBuilder gap_size_builder;
  arrow::DoubleBuilder psc_builder;
  arrow::TimestampBuilder psc_timestamp_builder(
      arrow::timestamp(arrow::TimeUnit::NANO, "UTC"),
      arrow::default_memory_pool());

  // Reserve space for optimal performance
  (void)gap_up_builder.Reserve(n);
  (void)gap_filled_builder.Reserve(n);
  (void)fill_fraction_builder.Reserve(n);
  (void)gap_size_builder.Reserve(n);
  (void)psc_builder.Reserve(n);
  (void)psc_timestamp_builder.Reserve(n);

  // First pass: append null for index 0
  gap_up_builder.UnsafeAppendNull();
  gap_filled_builder.UnsafeAppendNull();
  fill_fraction_builder.UnsafeAppendNull();
  gap_size_builder.UnsafeAppendNull();
  psc_builder.UnsafeAppendNull();
  psc_timestamp_builder.UnsafeAppendNull();

  if (n > 0) {
    auto timeIt = t->begin() + 1;
    auto openIt = open->begin() + 1;
    auto highIt = high->begin() + 1;
    auto lowIt = low->begin() + 1;
    auto prevCloseIt = close->begin();

    for (int64_t i = 1; i < n; ++i) {
      // Optional heuristic: consider a gap only when the bar boundary is a new
      // day
      const bool new_day = floor_to_day(*(timeIt - 1)) != floor_to_day(*timeIt);
      if (new_day && *openIt && *prevCloseIt && *highIt && *lowIt) {
        const double o = **openIt;
        const double h = **highIt;
        const double l = **lowIt;
        const double pc = **prevCloseIt;
        if (std::isfinite(o) && std::isfinite(pc) && std::isfinite(h) &&
            std::isfinite(l) && o != pc) {
          // We have a gap - record all the gap information
          const bool is_up_gap = o > pc;
          const double gap_abs = std::abs(o - pc);
          const double gap_pct = (gap_abs / pc) * 100.0;

          gap_up_builder.UnsafeAppend(is_up_gap);
          gap_size_builder.UnsafeAppend(gap_pct);  // Now storing percentage
          psc_builder.UnsafeAppend(pc);
          psc_timestamp_builder.UnsafeAppend(**(timeIt - 1));

          // Calculate gap fill
          bool gap_filled = false;
          double fill_fraction = 0.0;

          if (is_up_gap) {
            // Up gap: filled if low reaches or goes below prior close
            if (l <= pc) {
              gap_filled = true;
              fill_fraction = 1.0;
            } else if (l < o) {
              // Partial fill
              fill_fraction = (o - l) / gap_abs;
            }
          } else {
            // Down gap: filled if high reaches or goes above prior close
            if (h >= pc) {
              gap_filled = true;
              fill_fraction = 1.0;
            } else if (h > o) {
              // Partial fill
              fill_fraction = (h - o) / gap_abs;
            }
          }

          gap_filled_builder.UnsafeAppend(gap_filled);
          fill_fraction_builder.UnsafeAppend(fill_fraction);
        } else {
          // No gap - append nulls
          gap_up_builder.UnsafeAppendNull();
          gap_filled_builder.UnsafeAppendNull();
          fill_fraction_builder.UnsafeAppendNull();
          gap_size_builder.UnsafeAppendNull();
          psc_builder.UnsafeAppendNull();
          psc_timestamp_builder.UnsafeAppendNull();
        }
      } else {
        // No new day or missing data - append nulls
        gap_up_builder.UnsafeAppendNull();
        gap_filled_builder.UnsafeAppendNull();
        fill_fraction_builder.UnsafeAppendNull();
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

  // Build the Arrow arrays
  std::shared_ptr<arrow::Array> gap_up_array;
  std::shared_ptr<arrow::Array> gap_filled_array;
  std::shared_ptr<arrow::Array> fill_fraction_array;
  std::shared_ptr<arrow::Array> gap_size_array;
  std::shared_ptr<arrow::Array> psc_array;
  std::shared_ptr<arrow::Array> psc_timestamp_array;

  (void)gap_up_builder.Finish(&gap_up_array);
  (void)gap_filled_builder.Finish(&gap_filled_array);
  (void)fill_fraction_builder.Finish(&fill_fraction_array);
  (void)gap_size_builder.Finish(&gap_size_array);
  (void)psc_builder.Finish(&psc_array);
  (void)psc_timestamp_builder.Finish(&psc_timestamp_array);

  // Create chunked arrays
  auto gap_up_chunked = std::make_shared<arrow::ChunkedArray>(gap_up_array);
  auto gap_filled_chunked =
      std::make_shared<arrow::ChunkedArray>(gap_filled_array);
  auto fill_fraction_chunked =
      std::make_shared<arrow::ChunkedArray>(fill_fraction_array);
  auto gap_size_chunked = std::make_shared<arrow::ChunkedArray>(gap_size_array);
  auto psc_chunked = std::make_shared<arrow::ChunkedArray>(psc_array);
  auto psc_timestamp_chunked =
      std::make_shared<arrow::ChunkedArray>(psc_timestamp_array);

  // Construct output dataframe
  auto df = epoch_frame::make_dataframe(
      bars.index(),
      {gap_up_chunked, gap_filled_chunked, fill_fraction_chunked,
       gap_size_chunked, psc_chunked, psc_timestamp_chunked},
      {GetOutputId("gap_up"), GetOutputId("gap_filled"),
       GetOutputId("fill_fraction"), GetOutputId("gap_size"),
       GetOutputId("psc"), GetOutputId("psc_timestamp")});

  return df;
}

} // namespace epoch_metadata::transform
