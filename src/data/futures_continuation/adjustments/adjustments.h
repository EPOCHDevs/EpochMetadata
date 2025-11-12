#pragma once
//
// Created by dewe on 12/4/23.
//
#include "adjustment_base.h"
#include "epoch_frame/factory/index_factory.h"
#include "direction/backward.h"
#include "direction/forward.h"
#include "style/adjustment_style.h"
#include <epoch_frame/dataframe.h>

namespace epoch_script::futures {
    template <class Direction, epoch_core::AdjustmentType type>
    struct GenericAdjustmentMethod final : AdjustmentMethodBase {
        epoch_frame::DataFrame
      AdjustContracts(FuturesConstructedBars const &unAdjustedFrontBarData,
                      FuturesConstructedBars const &unAdjustedBackBarData,
                      std::vector<int64_t> const &rollIndexes) override {
            const int64_t nRows = unAdjustedBackBarData.t.size();
            auto rollIndexRange = CalculateRollIndexRanges(rollIndexes, nRows);

            auto bars = PrepareBarsContainer(nRows);
            Direction::AdjustPriceAttributes(bars, g_adjustedAttributeType,
                                             rollIndexRange, unAdjustedFrontBarData,
                                             unAdjustedBackBarData);

            const auto table = ConstructAdjustedTable(bars, unAdjustedFrontBarData);

            return epoch_frame::DataFrame{epoch_frame::factory::index::make_datetime_index(unAdjustedBackBarData.t, "", "UTC"), table};
        }

        epoch_core::AdjustmentType GetType() const override { return type; }
    };

    using BackwardPanamaMethod =
        GenericAdjustmentMethod<BackwardAdjustmentDirection<PanamaCanal>,
                                epoch_core::AdjustmentType::BackwardPanamaCanal>;
    using ForwardPanamaMethod =
        GenericAdjustmentMethod<ForwardAdjustmentDirection<PanamaCanal>,
                                epoch_core::AdjustmentType::ForwardPanamaCanal>;
    using BackwardRatioMethod =
        GenericAdjustmentMethod<BackwardAdjustmentDirection<Ratio>,
                                epoch_core::AdjustmentType::BackwardRatio>;
    using ForwardRatioMethod =
        GenericAdjustmentMethod<ForwardAdjustmentDirection<Ratio>,
                                epoch_core::AdjustmentType::ForwardRatio>;
} // namespace epoch_script::futures