#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>

namespace epoch_script::transform {

/**
 * Cross-Sectional Z-Score
 *
 * Normalizes each asset's value ACROSS assets at each timestamp.
 * Different from regular zscore which normalizes over TIME.
 *
 * At each timestamp t:
 *   z_score[asset_i, t] = (value[asset_i, t] - mean_across_all_assets[t]) / std_across_all_assets[t]
 *
 * Input: Multi-column DataFrame (one column per asset)
 * Output: Multi-column DataFrame with z-scores
 *
 * Example:
 *   sector_returns = roc(period=20)(src.c)
 *   normalized_returns = cs_zscore()(sector_returns)
 *   // At each timestamp, find sectors > 2 std devs from cross-sectional mean
 *   outliers = normalized_returns > 2.0
 *
 * Use Cases:
 *   - Normalize metrics across assets for comparison
 *   - Identify statistical outliers (high/low relative to peers)
 *   - Create cross-sectional signals (long top zscore, short bottom zscore)
 *   - Combine multiple factors with different scales
 */
class CSZScore final : public ITransform {
public:
  explicit CSZScore(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;

    if (df.empty() || df.num_cols() == 0) {
      throw std::runtime_error("CSZScore requires multi-column DataFrame");
    }

    // Strategy: For each row (timestamp), calculate mean and std across all columns (assets)
    // Then normalize each value: (value - row_mean) / row_std

    // Apply row-wise (across columns/assets)
    auto zscores = df.apply(
        [](const Array& row_array) -> Array {
          // row_array contains values for all assets at this timestamp
          const Series row_series(row_array.value());

          // Calculate cross-sectional statistics
          const Scalar mean = row_series.mean();
          const Scalar std = row_series.stddev(arrow::compute::VarianceOptions(1));  // Sample std

          // Normalize each value
          const Series normalized = (row_series - mean) / std;

          return Array(normalized.array());
        },
        AxisType::Row
    );

    return zscores;
  }
};

} // namespace epoch_script::transform
