#pragma once

#include "base.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Generic single-value data mapping
 * Used by most indicators: RSI, CCI, ATR, ADX, etc.
 * Maps to a single "result" or "value" output
 */
struct SingleValueDataMapping {
  std::string value;  // The single output column
};

// Type aliases for semantic clarity (all use SingleValueDataMapping)
using RSIDataMapping = SingleValueDataMapping;
using CCIDataMapping = SingleValueDataMapping;
using ATRDataMapping = SingleValueDataMapping;

} // namespace epoch_script::chart_metadata::plot_kinds
