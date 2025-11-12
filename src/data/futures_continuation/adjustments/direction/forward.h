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
struct ForwardAdjustmentDirection {
  static void AdjustPriceAttributes(
      FuturesConstructedBars &bars,
      const std::initializer_list<epoch_script::BarAttribute::Type>
          &adjustedAttributeType,
      const std::vector<std::pair<int64_t, int64_t>> &rollRange,
      const FuturesConstructedBars &unAdjustedFrontBarData,
      const FuturesConstructedBars &unAdjustedBackBarData) {
    for (epoch_script::BarAttribute::Type const &barAttributeType :
         adjustedAttributeType) {
      Style adjustmentStyle;

      const auto &backArray = unAdjustedBackBarData[barAttributeType];
      const auto &frontArray = unAdjustedFrontBarData[barAttributeType];
      const auto frontSpan = std::span(frontArray);

      auto &currentBarList = bars[barAttributeType];

      auto [start, length] = rollRange.front();
      std::ranges::copy(frontSpan.subspan(start, length),
                        currentBarList.begin());

      std::for_each(
          rollRange.begin() + 1, rollRange.end(), [&](const auto &range) {
            std::tie(start, length) = range;
            adjustmentStyle.ComputeAdjustmentFactor(frontArray[start],
                                                    backArray[start]);

            std::ranges::transform(
                frontSpan.subspan(start, length),
                currentBarList.begin() + start,
                [&adjustmentStyle](const double value) {
                  return adjustmentStyle.ApplyCumulativeAdjustment(value);
                });
          });
    }
  }
};
} // namespace epoch_script::futures