//
// Created by dewe on 12/4/23.
//
#include "adjustment_base.h"
#include "epoch_frame/factory/array_factory.h"
#include "epoch_frame/series.h"
#include <arrow/table.h>
#include <arrow/type_fwd.h>

namespace epoch_script::futures {
// Function to construct the adjusted table
arrow::TablePtr AdjustmentMethodBase::ConstructAdjustedTable(
    FuturesConstructedBars &bars,
    const FuturesConstructedBars &unAdjustedFrontBarData) {
  arrow::FieldVector fields;
  fields.reserve(g_adjustedAttributeType.size() +
                 g_unAdjustedAttributeType.size());
  arrow::ChunkedArrayVector data;
  data.reserve(fields.capacity());

  for (epoch_script::BarAttribute::Type const &barAttributeType :
       g_adjustedAttributeType) {
    fields.emplace_back(
        arrow::field(epoch_script::BarAttribute::fromType(barAttributeType),
                     arrow::float64()));
    data.emplace_back(
        epoch_frame::factory::array::make_array(bars[barAttributeType]));
  }

  for (epoch_script::BarAttribute::Type const &barAttributeType :
       g_unAdjustedAttributeType) {
    const auto barAttributeStr =
        epoch_script::BarAttribute::fromType(barAttributeType);
    if (barAttributeType != epoch_script::BarAttribute::Type::Contract) {
      fields.emplace_back(arrow::field(barAttributeStr, arrow::float64()));
      data.emplace_back(epoch_frame::factory::array::make_array(
          unAdjustedFrontBarData[barAttributeType]));
    } else {
      fields.emplace_back(arrow::field(barAttributeStr, arrow::utf8()));
      data.emplace_back(
          epoch_frame::factory::array::make_array(unAdjustedFrontBarData.s));
    }
  }

  return arrow::Table::Make(arrow::schema(fields), data);
}

// Function to prepare the bars container
FuturesConstructedBars
AdjustmentMethodBase::PrepareBarsContainer(int64_t nRows) {
  FuturesConstructedBars bars;
  for (epoch_script::BarAttribute::Type const &barAttribute :
       g_adjustedAttributeType) {
    bars[barAttribute].resize(nRows);
  }
  return bars;
}

// Function to calculate roll index ranges
std::vector<std::pair<int64_t, int64_t>>
AdjustmentMethodBase::CalculateRollIndexRanges(
    const std::vector<int64_t> &rollIndexes, int64_t nRows) {
  // Handle empty roll indexes case by returning a single range covering the
  // entire dataset
  if (rollIndexes.empty()) {
    return {{0, nRows}};
  }

  std::vector<std::pair<int64_t, int64_t>> rollIndexRange;
  rollIndexRange.reserve(rollIndexes.size());

  rollIndexRange.emplace_back(0, rollIndexes.front());
  std::transform(rollIndexes.begin(), rollIndexes.end() - 1,
                 rollIndexes.begin() + 1, std::back_inserter(rollIndexRange),
                 [](int64_t start, int64_t end) -> std::pair<int64_t, int64_t> {
                   return {start, end - start};
                 });
  rollIndexRange.emplace_back(rollIndexes.back(), nRows - rollIndexes.back());

  return rollIndexRange;
}
} // namespace epoch_script::futures