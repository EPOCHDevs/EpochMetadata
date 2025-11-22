#pragma once

#include "line_builder.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for column PlotKind (column plot)
 */
class ColumnBuilder : public LineBuilder {
public:
  // Inherit Build() and Validate() from LineBuilder

  // Only override what's different from LineBuilder defaults
  uint8_t GetZIndex() const override { return 0; }
};

}  // namespace epoch_script::chart_metadata::plot_kinds
