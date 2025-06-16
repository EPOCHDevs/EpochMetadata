#pragma once
#include "metadata.h"
#include "ui_data.h"
#include "validation_error.h"

namespace epoch_metadata::strategy {
// Main compiler interface - validate then compile
std::expected<PartialTradeSignalMetaData, ValidationIssues>
CreateAlgorithmMetaData(const UIData &data);

// Reverse compilation - convert metadata back to UI representation
std::expected<UIData, std::string>
CreateUIData(const PartialTradeSignalMetaData &data);

// Layout function using Graphviz dot algorithm with defaults
std::expected<UIData, std::string> AlignHorizontally(const UIData &data);
} // namespace epoch_metadata::strategy