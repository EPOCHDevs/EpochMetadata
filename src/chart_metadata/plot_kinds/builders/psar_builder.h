#pragma once

#include "line_builder.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Builder for psar PlotKind (Parabolic SAR)
 */
class PSARBuilder : public LineBuilder {
public:
  // Inherit Build(), Validate(), GetZIndex(), and RequiresOwnAxis() from LineBuilder
  // All defaults match: z-index=5, RequiresOwnAxis=false
};

}  // namespace epoch_script::chart_metadata::plot_kinds
