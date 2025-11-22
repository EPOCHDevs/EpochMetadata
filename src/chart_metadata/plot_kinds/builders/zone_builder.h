#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Zone PlotKind
 * Maps a single boolean output (either "result" or "value")
 * Used for time-based highlighting like day_of_week, session_time_window, etc.
 */
class ZoneBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN},
      {"value", cfg.GetOutputId("result")},
      {"value", cfg.GetOutputId("value")}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    const auto &outputs = cfg.GetOutputs();

    // Must have at least one output
    if (outputs.empty()) {
      throw std::runtime_error("Zone transform has no outputs");
    }

    // Must have "result" or "value" output
    if (!cfg.ContainsOutputId("result") && !cfg.ContainsOutputId("value")) {
      throw std::runtime_error(
        "Zone transform must have either 'result' or 'value' output"
      );
    }
  }

  uint8_t GetZIndex() const override { return 3; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
