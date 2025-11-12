#pragma once
//
// Created by dewe on 12/5/23.
//
#include "../style/adjustment_style.h"
#include "data/futures_continuation/constructed_bars.h"
#include "epoch_script/core/bar_attribute.h"
#include <epoch_frame/series.h>

namespace epoch_script::futures {
template <typename Style>
  requires std::is_base_of_v<IAdjustmentStyle, Style>
struct BackwardAdjustmentDirection {
  static void AdjustPriceAttributes(
      FuturesConstructedBars &bars,
      const std::initializer_list<epoch_script::BarAttribute::Type>
          &adjustedAttributeType,
      const std::vector<std::pair<int64_t, int64_t>> &rollRange,
      const FuturesConstructedBars &unAdjustedFrontBarData,
      const FuturesConstructedBars &unAdjustedBackBarData) {
    // TODO: Verify RollRanges sum into number of rows;

    // Iterate backwards through the roll index range
    // Calculate the adjustment factor at the end of the current segment
    for (epoch_script::BarAttribute::Type const &barAttributeType :
         adjustedAttributeType) {
      Style adjustmentStyle;

      const auto &backArray = unAdjustedBackBarData[barAttributeType];
      const auto &frontArray = unAdjustedFrontBarData[barAttributeType];

      auto &currentBarList = bars[barAttributeType];
      const auto frontSpan = std::span(frontArray);
      auto [start, length] = rollRange.back();

      std::ranges::copy(frontSpan.subspan(start, length),
                        currentBarList.begin() + start);
      std::for_each(
          rollRange.rbegin() + 1, rollRange.rend(), [&](const auto &range) {
            std::tie(start, length) = range;
            int64_t end = start + length;

            // Use the difference at the end of the roll period
            adjustmentStyle.ComputeAdjustmentFactor(frontArray[end],
                                                    backArray[end]);

            // Apply the cumulative adjustment factor to the entire segment
            std::ranges::transform(
                frontSpan.subspan(start, length),
                currentBarList.begin() + start,
                [&adjustmentStyle](double value) {
                  return adjustmentStyle.ApplyCumulativeAdjustment(value);
                });
          });
    }
  }
};
} // namespace epoch_script::futures