#pragma once

#include "base.h"
#include <unordered_map>
#include <memory>

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Registry for PlotKind builders using factory pattern
 * Singleton that maps PlotKind enums to their builders
 */
class PlotKindBuilderRegistry {
public:
  /**
   * @brief Get singleton instance
   */
  static PlotKindBuilderRegistry& Instance();

  /**
   * @brief Register a builder for a PlotKind
   * @param plotKind PlotKind enum value
   * @param builder Unique pointer to builder instance
   */
  void Register(
    epoch_core::TransformPlotKind plotKind,
    std::unique_ptr<IPlotKindBuilder> builder
  );

  /**
   * @brief Get builder for a PlotKind
   * @param plotKind PlotKind enum value
   * @return Reference to registered builder
   * @throws std::runtime_error if PlotKind not registered
   */
  const IPlotKindBuilder& GetBuilder(epoch_core::TransformPlotKind plotKind) const;

  /**
   * @brief Build data mapping for a transform
   * @param plotKind PlotKind enum from transform metadata
   * @param cfg Transform configuration
   * @return Map of data field names to column names
   * @throws std::runtime_error if PlotKind not registered or validation fails
   */
  std::unordered_map<std::string, std::string> Build(
    epoch_core::TransformPlotKind plotKind,
    const epoch_script::transform::TransformConfiguration &cfg
  ) const {
    return GetBuilder(plotKind).Build(cfg);
  }

  /**
   * @brief Check if a PlotKind is registered
   */
  bool IsRegistered(epoch_core::TransformPlotKind plotKind) const {
    return builders_.find(plotKind) != builders_.end();
  }

  /**
   * @brief Get z-index for a PlotKind (rendering layer priority)
   * @param plotKind PlotKind enum value
   * @return Z-index value (0-100, higher renders on top)
   * @throws std::runtime_error if PlotKind not registered
   */
  uint8_t GetZIndex(epoch_core::TransformPlotKind plotKind) const {
    return GetBuilder(plotKind).GetZIndex();
  }

  /**
   * @brief Check if PlotKind requires its own chart axis/panel
   * @param plotKind PlotKind enum value
   * @return true if requires separate axis, false if overlays on price
   * @throws std::runtime_error if PlotKind not registered
   */
  bool RequiresOwnAxis(epoch_core::TransformPlotKind plotKind) const {
    return GetBuilder(plotKind).RequiresOwnAxis();
  }

private:
  PlotKindBuilderRegistry();  // Private constructor for singleton
  void InitializeBuilders();   // Register all 46 builders

  std::unordered_map<epoch_core::TransformPlotKind, std::unique_ptr<IPlotKindBuilder>> builders_;
};

} // namespace epoch_script::chart_metadata::plot_kinds
