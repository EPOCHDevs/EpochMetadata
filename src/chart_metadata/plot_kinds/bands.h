#pragma once

#include "base.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Bollinger Bands: 3 outputs (upper, middle, lower)
 */
struct BbandsDataMapping {
  std::string bbands_upper;
  std::string bbands_middle;
  std::string bbands_lower;
};

} // namespace epoch_script::chart_metadata::plot_kinds
