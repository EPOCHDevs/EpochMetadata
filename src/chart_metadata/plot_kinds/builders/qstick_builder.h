#pragma once

#include "line_builder.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for qstick PlotKind (QStick)
 */
class QStickBuilder : public LineBuilder {
public:
  // Inherit Build() and Validate() from LineBuilder

  // Only override what's different from LineBuilder defaults
  uint8_t GetZIndex() const override { return 0; }
  bool RequiresOwnAxis() const override { return true; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
