#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Flag PlotKind
 * Special case: maps ALL outputs dynamically for template substitution
 * Used for generic event markers like candle patterns, fundamentals, etc.
 */
class FlagBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    return {
      {"index", INDEX_COLUMN}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    const auto &outputs = cfg.GetOutputs();

    // Must have at least one output
    if (outputs.empty()) {
      throw std::runtime_error("Flag transform has no outputs");
    }

    // All outputs should be accessible
    for (const auto &output : outputs) {
      if (!cfg.ContainsOutputId(output.id)) {
        throw std::runtime_error(
          "Flag transform missing output: " + output.id
        );
      }
    }
  }

  uint8_t GetZIndex() const override { return 10; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
