#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <DataFrame/DataFrameStatsVisitors.h>

namespace epoch_script::transform {

/**
 * Cross-Sectional Rolling Covariance
 *
 * Calculates rolling covariance between each asset and a benchmark.
 *
 * Inputs:
 *   - asset_series (cross-sectional): Multi-column DataFrame (one column per asset)
 *   - benchmark (single): Reference series to calculate covariance against
 *
 * Output: Multi-column DataFrame with covariance values for each asset vs benchmark
 *
 * Example:
 *   sector_returns = roc(period=20)(src.c)
 *   spy_returns = roc(period=20)(spy.c)
 *   covariances = cs_rolling_cov(window=60)(sector_returns, spy_returns)
 *   // Use for beta calculation: beta = cov(asset, market) / var(market)
 */
class CSRollingCov final : public ITransform {
public:
  explicit CSRollingCov(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;

    // Get benchmark series
    const std::string benchmarkInputId = GetInputId("benchmark");
    if (!df.contains(benchmarkInputId)) {
      throw std::runtime_error("CSRollingCov requires 'benchmark' input");
    }
    const Series benchmark = df[benchmarkInputId];

    // Get asset series columns
    std::vector<Series> assetSeriesList;
    std::vector<std::string> assetNames;

    for (const auto& colName : df.column_names()) {
      if (colName == benchmarkInputId) {
        continue;
      }
      assetSeriesList.push_back(df[colName]);
      assetNames.push_back(colName);
    }

    if (assetSeriesList.empty()) {
      throw std::runtime_error("CSRollingCov: No asset series found");
    }

    // Calculate rolling covariance for each asset vs benchmark
    std::vector<Series> covarianceResults;

    for (size_t i = 0; i < assetSeriesList.size(); ++i) {
      const Series& assetSeries = assetSeriesList[i];

      const Series covariance =
          assetSeries.rolling_apply({.window_size = m_window})
              .apply([&](const Series &assetWindow) {
                // Get corresponding benchmark window
                const Series benchmarkWindow = benchmark.loc(assetWindow.index());

                // Calculate covariance
                hmdf::CovVisitor<double, int64_t> visitor(
                    false,  // biased = false (sample covariance)
                    true,   // skip_nan = true
                    false   // stable_algo = false
                );

                const SeriesSpan<> assetSpan{assetWindow};
                const SeriesSpan<> benchmarkSpan{benchmarkWindow};
                run_visit(assetWindow, visitor, assetSpan, benchmarkSpan);

                return Scalar{visitor.get_result()};
              });

      covarianceResults.push_back(covariance.rename(assetNames[i]));
    }

    // Concatenate all covariance series
    std::vector<epoch_frame::FrameOrSeries> frames;
    for (const auto& series : covarianceResults) {
      frames.push_back(series);
    }

    auto result = epoch_frame::concat(
        epoch_frame::ConcatOptions{
            .frames = frames,
            .axis = epoch_frame::AxisType::Column
        });

    return result;
  }

private:
  int64_t m_window;
};

} // namespace epoch_script::transform
