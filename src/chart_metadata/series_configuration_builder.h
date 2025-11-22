#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "epoch_script/transforms/core/transform_configuration.h"
#include "epoch_script/core/metadata_options.h"

namespace epoch_frame {
struct SessionRange;
}

namespace epoch_script {
struct SeriesInfo;

namespace chart_metadata {

/**
 * @brief Builds SeriesInfo objects from transform configurations
 *
 * This class is responsible for creating properly configured SeriesInfo
 * objects with all required fields including chart type, data columns,
 * axis assignment, and linking information.
 */
class SeriesConfigurationBuilder {
public:
  /**
   * @brief Build a SeriesInfo object from transform configuration
   * @param cfg The transform configuration
   * @param chosenAxis The assigned Y-axis index
   * @param linkedTo Optional series ID to link to
   * @param seriesId The unique identifier for this series
   * @return Configured SeriesInfo object
   */
  static SeriesInfo BuildSeries(const epoch_script::transform::TransformConfiguration &cfg,
                                uint8_t chosenAxis,
                                const std::optional<std::string> &linkedTo,
                                const std::string &seriesId);

  /**
   * @brief Create candlestick chart series info
   * @param timeframe The timeframe string
   * @return SeriesInfo for candlestick chart
   */
  static SeriesInfo BuildCandlestickSeries(const std::string &timeframe);

  /**
   * @brief Create volume chart series info
   * @param timeframe The timeframe string
   * @return SeriesInfo for volume chart
   */
  static SeriesInfo BuildVolumeSeries(const std::string &timeframe);

  // static SeriesInfo BuildPositionQuantitySeries(const std::string
  // &timeframe);
  //
  // static SeriesInfo BuildExitLevelsSeries(const std::string &timeframe);

  /**
   * @brief Build descriptive name with parameters (TradingView style)
   * @param cfg The transform configuration
   * @return Descriptive name like "SMA 10", "ATR 14", etc.
   */
  static std::string
  BuildDescriptiveName(const epoch_script::transform::TransformConfiguration &cfg);

private:
  /**
   * @brief Check if a timeframe string represents an intraday timeframe
   * @param timeframe The timeframe string to check
   * @return True if intraday, false otherwise
   */
  static bool IsIntradayTimeframe(const std::string &timeframe);

  /**
   * @brief Build config options from transform configuration
   * @param cfg The transform configuration
   * @return Map of config options with original variant types for UI
   * annotations/thresholds
   */
  static epoch_script::MetaDataArgDefinitionMapping
  BuildConfigOptions(const epoch_script::transform::TransformConfiguration &cfg);
};

} // namespace chart_metadata
} // namespace epoch_stratifyx