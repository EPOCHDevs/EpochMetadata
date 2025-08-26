#pragma once
#include "epoch_metadata/transforms/metadata.h"
#include "ui_data.h"
#include "validation_error.h"

namespace epoch_metadata::strategy {

using HandleReference = std::unordered_map<std::string, std::vector<UIVertex>>;

struct ValidationCache {
  std::vector<std::string> sortedNodeIds;
  std::unordered_set<std::string> validatedNodeIds;
  std::unordered_map<
      std::string,
      std::pair<UINode, std::optional<transforms::TransformsMetaData>>>
      nodeMap;
  std::unordered_map<std::string, HandleReference> inputHandleReferencesPerNode;
  std::unordered_map<std::string, HandleReference>
      outputHandleReferencesPerNode;
};

/// Semantic validation of UIData (source code validation)
/// This validates the graph structure, type consistency, etc. before
/// compilation
ValidationResult ValidateUIData(const UIData &graph, bool enforceOrphanedNodeCheck, bool enforceExecutorPresence);

/// Individual validation phases
void ValidateNode(const UIData &graph,
                  bool enforceOrphanedNodeCheck,
                  ValidationCache &cache,
                  ValidationIssues &issues);
void ValidateEdgeReferences(const std::vector<UIEdge> &edges,
                            const ValidationCache &cache,
                            ValidationIssues &issues);
void ValidateExecutorPresence(const UIData &graph, ValidationIssues &issues);
void ValidateAcyclic(const UIData &graph, ValidationCache &cache,
                     ValidationIssues &issues);
void ValidateTimeframeConsistency(ValidationCache &cache,
                                  ValidationIssues &issues);

/// Optimization functions for UIData
/// Optimizes the algorithm by removing orphan nodes, fixing stuck bool nodes,
/// applying default values, clamping values, and other improvements
UIData OptimizeUIData(const UIData &graph, bool optimizeOrphanedNodes);

/// Individual optimization phases
void RemoveOrphanNodes(UIData &graph);
void RemoveStuckBoolNodesFromExecutor(UIData &graph);
void ApplyDefaultOptions(UIData &graph);
void ClampOptionValues(UIData &graph);
void RemoveUnnecessaryTimeframes(UIData &graph);

} // namespace epoch_metadata::strategy