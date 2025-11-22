#pragma once

#include "../complex.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for Trade Signal PlotKind
 * Special case: maps from cfg.GetInputs() instead of outputs
 * This allows trade signals to reference other transform outputs as inputs
 */
class TradeSignalBuilder : public IPlotKindBuilder {
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
    const auto &inputs = cfg.GetInputs();

    // Must have at least one input
    if (inputs.empty()) {
      throw std::runtime_error("TradeSignal transform has no inputs");
    }

    // Validate each input has a handle
    for (const auto &input : inputs) {
      if (input.second.empty()) {
        throw std::runtime_error(
          "TradeSignal input '" + input.first + "' has empty handle"
        );
      }
    }
  }

  uint8_t GetZIndex() const override { return 100; }
  bool RequiresOwnAxis() const override { return false; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
