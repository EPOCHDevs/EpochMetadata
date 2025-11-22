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

    // Start with index column
    std::unordered_map<std::string, std::string> dataMapping = {
      {"index", INDEX_COLUMN}
    };

    // Add ALL output columns for template substitution (e.g., {column_name})
    for (const auto &output : cfg.GetOutputs()) {
      dataMapping[output.id] = cfg.GetOutputId(output.id);
    }

    return dataMapping;
  }

  void Validate(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    const auto &outputs = cfg.GetOutputs();

    // Flag transforms must have at least one output for template substitution
    if (outputs.empty()) {
      throw std::runtime_error("Flag transform has no outputs");
    }

    // All outputs must be accessible (validation confirms structure)
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

  epoch_script::MetaDataArgDefinitionMapping GetDefaultConfigOptions(
    const epoch_script::transform::TransformConfiguration &cfg
  ) const override {
    const auto &metadata = cfg.GetTransformDefinition().GetMetadata();
    epoch_script::MetaDataArgDefinitionMapping defaults;

    // Flag PlotKind MUST have flagSchema defined
    if (!metadata.flagSchema.has_value()) {
      throw std::runtime_error(
        "Flag transform '" + cfg.GetId() + "' missing required flagSchema"
      );
    }

    const auto& schema = metadata.flagSchema.value();

    // Flag title
    std::string title = schema.title.has_value()
      ? schema.title.value()
      : metadata.name;
    defaults["flagTitle"] = epoch_script::MetaDataOptionDefinition{title};

    // Flag schema fields
    defaults["flagText"] = epoch_script::MetaDataOptionDefinition{schema.text};
    defaults["flagTextIsTemplate"] = epoch_script::MetaDataOptionDefinition{schema.textIsTemplate};
    defaults["flagIcon"] = epoch_script::MetaDataOptionDefinition{
      epoch_core::IconWrapper::ToString(schema.icon)
    };
    defaults["flagColor"] = epoch_script::MetaDataOptionDefinition{
      epoch_core::ColorWrapper::ToString(schema.color)
    };

    return defaults;
  }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
