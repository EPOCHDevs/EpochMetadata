#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <DataFrame/DataFrameStatsVisitors.h>

namespace epoch_script::transform {

/**
 * Cross-Sectional Rolling Correlation
 *
 * Calculates rolling correlation between each asset and a benchmark.
 *
 * Inputs:
 *   - asset_series (cross-sectional): Multi-column DataFrame (one column per asset)
 *   - benchmark (single): Reference series to correlate against (e.g., SPY returns)
 *
 * Output: Multi-column DataFrame with correlation values for each asset vs benchmark
 *
 * Example:
 *   sector_returns = roc(period=20)(src.c)  // Cross-sectional
 *   spy_returns = roc(period=20)(spy.c)     // Single benchmark
 *   correlations = cs_rolling_corr(window=60)(sector_returns, spy_returns)
 *   // Output: Correlation of each sector with SPY over rolling 60-period window
 */
class CSRollingCorr final : public ITransform {
public:
  explicit CSRollingCorr(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;

    // Get benchmark series (single column, from second input)
    const std::string benchmarkInputId = GetInputId("benchmark");
    if (!df.contains(benchmarkInputId)) {
      throw std::runtime_error("CSRollingCorr requires 'benchmark' input");
    }
    const Series benchmark = df[benchmarkInputId];

    // Get asset series columns (all columns except benchmark)
    std::vector<Series> assetSeriesList;
    std::vector<std::string> assetNames;

    for (const auto& colName : df.column_names()) {
      if (colName == benchmarkInputId) {
        continue;  // Skip benchmark column
      }
      assetSeriesList.push_back(df[colName]);
      assetNames.push_back(colName);
    }

    if (assetSeriesList.empty()) {
      throw std::runtime_error("CSRollingCorr: No asset series found");
    }

    // Calculate rolling correlation for each asset vs benchmark
    std::vector<Series> correlationResults;

    for (size_t i = 0; i < assetSeriesList.size(); ++i) {
      const Series& assetSeries = assetSeriesList[i];

      const Series correlation =
          assetSeries.rolling_apply({.window_size = m_window})
              .apply([&](const Series &assetWindow) {
                // Get corresponding benchmark window
                const Series benchmarkWindow = benchmark.loc(assetWindow.index());

                // Calculate Pearson correlation
                hmdf::CorrVisitor<double, int64_t> visitor(
                    hmdf::correlation_type::pearson,
                    false,  // biased = false (sample correlation)
                    true,   // skip_nan = true
                    false   // stable_algo = false
                );

                const SeriesSpan<> assetSpan{assetWindow};
                const SeriesSpan<> benchmarkSpan{benchmarkWindow};
                run_visit(assetWindow, visitor, assetSpan, benchmarkSpan);

                return Scalar{visitor.get_result()};
              });

      correlationResults.push_back(correlation.rename(assetNames[i]));
    }

    // Concatenate all correlation series into multi-column DataFrame
    std::vector<epoch_frame::FrameOrSeries> frames;
    for (const auto& series : correlationResults) {
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
