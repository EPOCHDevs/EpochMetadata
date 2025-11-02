#pragma once

#include "../data_source.h"

namespace epoch_script::transform {

// Type aliases for SEC data sources
// All SEC data sources use the generic DataSourceTransform
using Form13FHoldingsTransform = DataSourceTransform;
using InsiderTradingTransform = DataSourceTransform;

} // namespace epoch_script::transform
