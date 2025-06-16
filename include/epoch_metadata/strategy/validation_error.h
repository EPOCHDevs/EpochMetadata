#pragma once
#include "epoch_metadata/metadata_options.h"
#include "epoch_metadata/strategy/generic_function.h"
#include "epoch_metadata/transforms/metadata.h"
#include "ui_data.h"
#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>

CREATE_ENUM(ValidationCode, UnknownNodeType, UnknownTransformType, InvalidEdge,
            TimeframeMismatch, CycleDetected, MissingExecutor,
            MultipleExecutors, MissingRequiredInput, MissingRequiredOption,
            InvalidOptionReference, InvalidNodeId, EmptyGraph, OrphanedNode,
            InvalidNodeConnection, MissingRequiredHandle, OptionValueOutOfRange,
            InvalidOptionCombination, NoPathToExecutor, SecurityViolation,
            ResourceLimitExceeded, CircularOptionReference);

namespace epoch_metadata::strategy {
using epoch_core::ValidationCode;
using epoch_core::ValidationCodeWrapper;

// Reuse existing UI structures for context
using ValidationContext =
    std::variant<std::monostate,   // No specific context
                 UINode,           // Issue with a specific node
                 UIEdge,           // Issue with a specific edge
                 UIGroupNode,      // Issue with a group
                 UIAnnotationNode, // Issue with an annotation
                 MetaDataOption, std::string>;

struct ValidationIssue {
  ValidationCode code;
  ValidationContext ctx;
  std::string message;
  std::optional<std::string> suggestion; // For LLM models to fix issues
};

using ValidationIssues = std::vector<ValidationIssue>;

// Validation result for UIData (source code validation)
// On success, returns topologically sorted UINodes for efficient compilation
using ValidationResult = std::expected<std::vector<UINode>, ValidationIssues>;

} // namespace epoch_metadata::strategy