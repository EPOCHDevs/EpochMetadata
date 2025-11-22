#pragma once

#include "../single_value.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for h_line PlotKind (horizontal line)
 */
class HLineBuilder : public IPlotKindBuilder {
public:
  std::unordered_map<std::string, std::string> Build(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    Validate(cfg);

    const auto &outputs = cfg.GetOutputs();

    // Try result first, then value, then use first output
    std::string valueCol;
    if (cfg.ContainsOutputId("result")) {
      valueCol = cfg.GetOutputId("result");
    } else if (cfg.ContainsOutputId("value")) {
      valueCol = cfg.GetOutputId("value");
    } else {
      valueCol = cfg.GetOutputId(outputs[0].id);
    }

    return {
      {"index", INDEX_COLUMN},
      {"value", valueCol}
    };
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    const auto &outputs = cfg.GetOutputs();

    // Must have at least one output
    if (outputs.empty()) {
      throw std::runtime_error("HLine transform has no outputs");
    }

    // If more than one output, must have "result" or "value"
    if (outputs.size() > 1) {
      if (!cfg.ContainsOutputId("result") && !cfg.ContainsOutputId("value")) {
        throw std::runtime_error(
          "HLine transform with multiple outputs must have 'result' or 'value' output"
        );
      }
    }
  }

  uint8_t GetZIndex() const override { return 5; }
  bool RequiresOwnAxis() const override { return false; }
};

} // namespace epoch_script::chart_metadata::plot_kinds
