#pragma once

#include "line_builder.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for panel_line PlotKind (generic panel line)
 */
class PanelLineBuilder : public LineBuilder {
public:
  // Inherit Build() and Validate() from LineBuilder

  // Only override what's different from LineBuilder defaults
  bool RequiresOwnAxis() const override { return true; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
