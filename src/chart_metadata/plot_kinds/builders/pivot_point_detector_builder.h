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
    ValidateOutput(cfg, "pivot_type", "PivotPointDetector");
    ValidateOutput(cfg, "pivot_level", "PivotPointDetector");
    ValidateOutput(cfg, "pivot_index", "PivotPointDetector");
  }

  uint8_t GetZIndex() const override { return 5; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
