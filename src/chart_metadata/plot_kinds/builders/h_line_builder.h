#pragma once

#include "line_builder.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for h_line PlotKind (horizontal line)
 */
class HLineBuilder : public LineBuilder {
public:
  // Inherit Build(), Validate(), GetZIndex(), and RequiresOwnAxis() from LineBuilder
  // All defaults match: z-index=5, RequiresOwnAxis=false
};

} // namespace epoch_script::chart_metadata::plot_kinds
