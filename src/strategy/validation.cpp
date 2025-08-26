#include "epoch_metadata/strategy/validation.h"
#include <format>
#include <sstream>

namespace epoch_metadata::strategy {

std::expected<PartialTradeSignalMetaData, ValidationIssues>
CompileAlgorithmMetaData(const UIData &sourceGraph, bool enforceOrphanedNodeCheck, bool enforceExecutorPresence) {
  // Step 1: Validate the source code (UIData) - returns sorted nodes on success
  auto validationResult = ValidateUIData(sourceGraph, enforceOrphanedNodeCheck, enforceExecutorPresence);
  if (!validationResult) {
    return std::unexpected(validationResult.error());
  }

  // Step 2: Compile the validated and sorted source to metadata
  // Use the sorted nodes for efficient compilation
  const auto &sortedNodes = validationResult.value();
  auto compilationResult = CompileUIData(sortedNodes, sourceGraph);
  if (!compilationResult) {
    // Compilation failure after successful validation is unexpected
    // Wrap as validation issue for consistent error handling
    return std::unexpected(ValidationIssues{
        {ValidationCode::UnknownTransformType, std::monostate{},
         std::format("Compilation failed: {}", compilationResult.error()),
         std::nullopt}});
  }

  // Success - return the compiled metadata
  return compilationResult.value();
}

std::string FormatValidationIssues(const ValidationIssues &issues) {
  if (issues.empty()) {
    return "No validation issues";
  }

  std::stringstream ss;
  ss << "Validation failed with " << issues.size() << " issue(s):\n";

  for (size_t i = 0; i < issues.size(); ++i) {
    const auto &issue = issues[i];
    ss << "  " << (i + 1) << ". ";

    // Add context information using std::visit
    std::visit(
        [&ss](const auto &ctx) {
          using T = std::decay_t<decltype(ctx)>;
          if constexpr (std::is_same_v<T, UINode>) {
            ss << "[Node '" << ctx.id << "': " << ctx.type << "] ";
          } else if constexpr (std::is_same_v<T, UIEdge>) {
            ss << "[Edge: " << ctx.source.id << "#" << ctx.source.handle
               << " -> " << ctx.target.id << "#" << ctx.target.handle << "] ";
          } else if constexpr (std::is_same_v<T, UIGroupNode>) {
            ss << "[Group '" << ctx.id << "'] ";
          } else if constexpr (std::is_same_v<T, UIAnnotationNode>) {
            ss << "[Annotation '" << ctx.id << "'] ";
          }
          // std::monostate case - no context info added
        },
        issue.ctx);

    // Add the message
    ss << issue.message;

    if (i < issues.size() - 1) {
      ss << "\n";
    }
  }

  return ss.str();
}

} // namespace epoch_metadata::strategy