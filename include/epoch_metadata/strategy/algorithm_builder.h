#pragma once
#include "metadata.h"
#include "ui_data.h"
#include <expected>

namespace epoch_metadata::strategy {

/// Pure compilation of validated UIData to PartialTradeSignalMetaData
/// Assumes UIData has already been validated - this is the "code generation"
/// phase. Takes pre-sorted nodes from validation for efficient compilation.
/// Only returns errors for truly unexpected compilation failures
std::expected<PartialTradeSignalMetaData, std::string>
CompileUIData(const std::vector<UINode> &sortedNodes,
              const UIData &validatedGraph);

} // namespace epoch_metadata::strategy