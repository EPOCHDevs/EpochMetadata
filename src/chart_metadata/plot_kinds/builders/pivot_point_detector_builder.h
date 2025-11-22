#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for pivot_point_detector PlotKind
 * Outputs: pivot_type, pivot_level, pivot_index
 */
class PivotPointDetectorBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"pivot_type", cfg.GetOutputId("pivot_type")},
      {"pivot_level", cfg.GetOutputId("pivot_level")},
      {"pivot_index", cfg.GetOutputId("pivot_index")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    // Validate all required outputs exist
    if (!cfg.ContainsOutputId("pivot_type")) {
      throw std::runtime_error("PivotPointDetector transform missing required output: pivot_type");
    }

    if (!cfg.ContainsOutputId("pivot_level")) {
      throw std::runtime_error("PivotPointDetector transform missing required output: pivot_level");
    }
    if (!cfg.ContainsOutputId("pivot_index")) {
      throw std::runtime_error("PivotPointDetector transform missing required output: pivot_index");
    }
  }

  uint8_t GetZIndex() const override { return 5; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
