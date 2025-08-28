#pragma once

#include "algorithm_builder.h"
#include "algorithm_validator.h"
#include "metadata.h"
#include "ui_data.h"
#include "validation_error.h"

namespace epoch_metadata::strategy {

/// Complete compiler pipeline: validate UIData then compile to metadata
/// This is the main entry point for the "compiler"
std::expected<PartialTradeSignalMetaData, ValidationIssues>
CompileAlgorithmMetaData(const UIData &sourceGraph, bool strictMode);

/// Utility function to format validation issues for display
std::string FormatValidationIssues(const ValidationIssues &issues);

} // namespace epoch_metadata::strategy