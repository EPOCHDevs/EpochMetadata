#pragma once

#include "epoch_script/chart_metadata/chart_metadata_provider.h"

namespace epoch_script {

/**
 * @brief Technical indicator data mapping (legacy, may be deprecated)
 */
struct TechnicalIndicatorDataMapping {
  std::string type;
  std::vector<std::string> outputs;
};

/**
 * @brief Generates chart metadata from transform configurations
 *
 * Takes a list of transform configurations and produces complete chart metadata
 * including axes, series, and data mappings for visualization.
 */
class ChartMetadataProvider final : public IChartMetadataProvider {
public:
  /**
   * @brief Construct provider from timeframes and transforms
   * @param timeframes Set of timeframe strings to generate metadata for
   * @param transforms List of transform configurations to visualize
   */
  explicit ChartMetadataProvider(
      const std::unordered_set<std::string> &timeframes,
      const epoch_script::transform::TransformConfigurationList &transforms);

  /**
   * @brief Get complete chart metadata for all timeframes
   * @return Map of timeframe → chart pane metadata
   */
  TimeFrameChartMetadata GetMetaData() const override {
    return m_chartMetaData;
  }

  /**
   * @brief Create series info from a transform configuration
   * @param cfg Transform configuration
   * @param chosenAxis Y-axis index
   * @param linkedTo Optional series ID to link to
   * @param seriesId Unique series identifier
   * @return Configured SeriesInfo
   */
  static SeriesInfo CreateSeries(
      const epoch_script::transform::TransformConfiguration &cfg,
      uint8_t chosenAxis,
      const std::optional<std::string> &linkedTo,
      const std::string &seriesId);

private:
  TimeFrameChartMetadata m_chartMetaData;
  std::unordered_map<std::string, TechnicalIndicatorDataMapping>
      m_technicalIndicatorTypeMappings;

  static std::string GetTechnicalIndicatorMetaData(
      const epoch_script::transform::TransformConfiguration &cfg,
      std::vector<std::string> &outputs);
};

} // namespace epoch_script
