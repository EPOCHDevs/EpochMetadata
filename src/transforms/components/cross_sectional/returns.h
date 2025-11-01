#pragma once
//
// Created by dewe on 4/14/23.
//
#include <epochflow/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epochflow/core/constants.h>

namespace epochflow::transform {
/**
 * @brief Cross-sectional returns operation
 *
 * This transform calculates cross-sectional returns across multiple assets.
 * It computes the mean percentage change across all assets (columns) at each
 * time point, then calculates the cumulative product of these mean returns
 * plus 1.
 *
 * The operation is performed per time step across all assets, and the result
 * is broadcasted back to each asset in the output.
 *
 * Input: DataFrame containing percentage changes for multiple assets
 * Output: DataFrame containing cumulative cross-sectional returns
 */

struct CrossSectionalMomentumOperation final : ITransform {

  explicit CrossSectionalMomentumOperation(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &pctChange) const override {
    using namespace epoch_frame;

    const Series csMean = pctChange.mean(AxisType::Column);
    const Series csReturns = (csMean + 1_scalar).cumulative_prod();
    return csReturns.to_frame(GetOutputId());
  }
};
} // namespace epochflow::transform
