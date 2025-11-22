#pragma once

#include "../bands.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Bollinger Bands PlotKind
 * Expects 3 outputs: bbands_upper, bbands_middle, bbands_lower
 */
class BbandsBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"bbands_upper", cfg.GetOutputId("bbands_upper")},
      {"bbands_middle", cfg.GetOutputId("bbands_middle")},
      {"bbands_lower", cfg.GetOutputId("bbands_lower")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    ValidateOutput(cfg, "bbands_upper", "BBands");
    ValidateOutput(cfg, "bbands_middle", "BBands");
    ValidateOutput(cfg, "bbands_lower", "BBands");
  }

  uint8_t GetZIndex() const override { return 1; }
  bool RequiresOwnAxis() const override { return false; }

};

} // namespace epoch_script::chart_metadata::plot_kinds
