#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for gap PlotKind (Gap indicator)
 * Outputs: gap_filled, gap_retrace, gap_size, psc, psc_timestamp
 */
class GapBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"gap_filled", cfg.GetOutputId("gap_filled")},
      {"gap_retrace", cfg.GetOutputId("gap_retrace")},
      {"gap_size", cfg.GetOutputId("gap_size")},
      {"psc", cfg.GetOutputId("psc")},
      {"psc_timestamp", cfg.GetOutputId("psc_timestamp")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    // Validate all required outputs exist
    if (!cfg.ContainsOutputId("gap_filled")) {
      throw std::runtime_error("Gap transform missing required output: gap_filled");
    }
    if (!cfg.ContainsOutputId("gap_retrace")) {
      throw std::runtime_error("Gap transform missing required output: gap_retrace");
    }
    if (!cfg.ContainsOutputId("gap_size")) {
      throw std::runtime_error("Gap transform missing required output: gap_size");
    }
    if (!cfg.ContainsOutputId("psc")) {
      throw std::runtime_error("Gap transform missing required output: psc");
    }
    if (!cfg.ContainsOutputId("psc_timestamp")) {
      throw std::runtime_error("Gap transform missing required output: psc_timestamp");
    }
  }

  uint8_t GetZIndex() const override { return 1; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
