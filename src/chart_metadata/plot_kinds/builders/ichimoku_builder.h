#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Ichimoku Cloud PlotKind
 * Expects 5 outputs: tenkan, kijun, senkou_a, senkou_b, chikou
 */
class IchimokuBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"tenkan", cfg.GetOutputId("tenkan")},
      {"kijun", cfg.GetOutputId("kijun")},
      {"senkou_a", cfg.GetOutputId("senkou_a")},
      {"senkou_b", cfg.GetOutputId("senkou_b")},
      {"chikou", cfg.GetOutputId("chikou")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    ValidateOutput(cfg, "tenkan", "Ichimoku");
    ValidateOutput(cfg, "kijun", "Ichimoku");
    ValidateOutput(cfg, "senkou_a", "Ichimoku");
    ValidateOutput(cfg, "senkou_b", "Ichimoku");
    ValidateOutput(cfg, "chikou", "Ichimoku");
  }

  uint8_t GetZIndex() const override { return 1; }
  bool RequiresOwnAxis() const override { return false; }

};

} // namespace epoch_script::chart_metadata::plot_kinds
