#pragma once

#include "epoch_script/transforms/core/transform_configuration.h"
#include "epoch_script/core/metadata_options.h"
#include "../data_column_resolver.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Abstract builder interface for PlotKind data mappings
 * Each PlotKind has a concrete builder that implements this interface
 */
class IPlotKindBuilder {
public:
  virtual ~IPlotKindBuilder() = default;

  /**
   * @brief Build data mapping from transform configuration
   * @param cfg Transform configuration containing output definitions
   * @return Map of data field names to column names (e.g., {"macd": "transform_42_macd"})
   * @throws std::runtime_error if required outputs are missing
   */
  virtual std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const = 0;

  /**
   * @brief Validate that transform has all required outputs for this PlotKind
   * @param cfg Transform configuration to validate
   * @throws std::runtime_error if validation fails
   */
  virtual void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const = 0;

  /**
   * @brief Get the z-index for this PlotKind (layering priority for rendering)
   * @return Z-index value (0-100, higher values render on top)
   *
   * Common values:
   * - 0: Background elements (columns that need own axis)
   * - 5: Standard overlays and panel indicators
   * - 10: High-priority overlays (flags, patterns)
   * - 100: Highest priority (trade signals)
   */
  virtual uint8_t GetZIndex() const = 0;

  /**
   * @brief Check if this PlotKind requires its own chart axis/panel
   * @return true if requires separate axis, false if overlays on price chart
   *
   * Examples:
   * - true: Oscillators (RSI, MACD), volume columns
   * - false: Moving averages, Bollinger Bands, patterns
   */
  virtual bool RequiresOwnAxis() const = 0;

  /**
   * @brief Get default config options for this PlotKind
   * @param cfg Transform configuration
   * @return Map of default config option keys to their values
   *
   * Each PlotKind can provide its own defaults (e.g., zone has color/label,
   * flag has icon/text). These are only applied if not already configured.
   */
  virtual epoch_script::MetaDataArgDefinitionMapping GetDefaultConfigOptions(
    const epoch_script::transform::TransformConfiguration & /*cfg*/
  ) const {
    // Default: no config options
    return {};
  }

protected:
  /**
   * @brief Helper to validate a single output exists
   * @param cfg Transform configuration
   * @param outputId Output ID to check
   * @param plotKindName Name of PlotKind for error message
   * @throws std::runtime_error if output is missing
   */
  static void ValidateOutput(
    const epoch_script::transform::TransformConfiguration &cfg,
    const std::string &outputId,
    const std::string &plotKindName
  ) {
    if (!cfg.ContainsOutputId(outputId)) {
      throw std::runtime_error(
        plotKindName + " transform missing required output: " + outputId
      );
    }
  }
};

} // namespace epoch_script::chart_metadata::plot_kinds
