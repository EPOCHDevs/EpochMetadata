#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Fair Value Gap PlotKind
 * Expects 4 outputs: fvg, top, bottom, mitigated_index
 */
class FVGBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"fvg", cfg.GetOutputId("fvg")},
      {"top", cfg.GetOutputId("top")},
      {"bottom", cfg.GetOutputId("bottom")},
      {"mitigated_index", cfg.GetOutputId("mitigated_index")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    ValidateOutput(cfg, "fvg", "FVG");
    ValidateOutput(cfg, "top", "FVG");
    ValidateOutput(cfg, "bottom", "FVG");
    ValidateOutput(cfg, "mitigated_index", "FVG");
  }

  uint8_t GetZIndex() const override { return 5; }
  bool RequiresOwnAxis() const override { return false; }

};

} // namespace epoch_script::chart_metadata::plot_kinds
