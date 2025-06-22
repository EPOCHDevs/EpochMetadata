#include "epoch_metadata/strategy/algorithm_builder.h"
#include "epoch_metadata/strategy/algorithm_validator.h"
#include "epoch_metadata/strategy/metadata.h"
#include "epoch_metadata/transforms/metadata.h"
#include "epoch_metadata/transforms/registry.h"
#include <format>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

using namespace epoch_metadata::transforms;

namespace epoch_metadata::strategy {

// Import functions from ui_graph.cpp - these are the core compilation routines
extern std::string JoinId(std::string const &id1, std::string const &id2);

extern std::unordered_map<std::string, epoch_metadata::MetaDataOption>
CreateOptionMapping(const TransformsMetaData &transformMetaData);

extern epoch_metadata::strategy::AlgorithmNode CreateAlgorithmNode(
    const UINode &node,
    std::unordered_map<std::string, epoch_metadata::MetaDataOption> const
        &optionsMapping,
    std::vector<epoch_metadata::MetaDataOption> &options);

extern std::vector<AlgorithmNode>
CreateSortedEdges(const std::unordered_map<std::string, AlgorithmNode> &nodes);

extern PartialTradeSignalMetaData CreatePartialTradeSignalMetaData(
    const std::vector<epoch_metadata::strategy::AlgorithmNode> &algorithm);

struct CompilerData {
  std::unordered_map<std::string, epoch_metadata::strategy::AlgorithmNode>
      algorithmMap;
  std::unordered_map<std::string, UINode> nodeMap;
};

std::expected<epoch_metadata::strategy::AlgorithmNode, std::string>
CompileAlgorithmNode(
    const UINode &node,
    std::vector<epoch_metadata::MetaDataOption> &metadataOptions) {
  const auto &registry = ITransformRegistry::GetInstance().GetMetaData();

  // Assume validation has already checked this, but double-check for safety
  if (!registry.contains(node.type)) {
    return std::unexpected(std::format("Unknown node type: {}", node.type));
  }

  const auto &transformMetadata = registry.at(node.type);
  const auto optionsMapping = CreateOptionMapping(transformMetadata);

  try {
    return CreateAlgorithmNode(node, optionsMapping, metadataOptions);
  } catch (const std::exception &e) {
    return std::unexpected(e.what());
  }
}

std::expected<CompilerData, std::string>
CompileNodes(const UIData &validatedGraph,
             std::vector<epoch_metadata::MetaDataOption> &metadataOptions) {
  CompilerData compilerData;

  for (const auto &node : validatedGraph.nodes) {
    compilerData.nodeMap[node.id] = node;

    auto result = CompileAlgorithmNode(node, metadataOptions);
    if (!result) {
      return std::unexpected(result.error());
    }

    compilerData.algorithmMap[node.id] = result.value();
  }

  return compilerData;
}

std::expected<std::monostate, std::string>
CompileEdges(const UIData &validatedGraph, CompilerData &compilerData) {
  for (const auto &edge : validatedGraph.edges) {
    const auto &sourceNode = compilerData.nodeMap.at(edge.source.id);
    const auto &targetNode = compilerData.nodeMap.at(edge.target.id);

    // Get algorithm node for the target
    auto &algo = compilerData.algorithmMap.at(targetNode.id);

    // Assign the input connection
    algo.inputs[edge.target.handle].push_back(
        JoinId(edge.source.id, edge.source.handle));

    // Handle timeframe inheritance (validation should have caught conflicts)
    const auto &sourceAlgoIter = compilerData.algorithmMap.find(sourceNode.id);

    if (!algo.timeframe) {
      if (sourceAlgoIter != compilerData.algorithmMap.end() &&
          sourceAlgoIter->second.timeframe) {
        algo.timeframe = sourceAlgoIter->second.timeframe;
      } else if (sourceNode.timeframe) {
        algo.timeframe = sourceNode.timeframe;
      }
    }
  }

  return std::monostate{};
}

std::expected<PartialTradeSignalMetaData, std::string>
CompileUIData(const std::vector<UINode> &sortedNodes,
              const UIData &validatedGraph) {
  epoch_metadata::MetaDataOptionList options;

  // Step 1: Compile nodes using the pre-sorted order from validation
  CompilerData compilerData;

  for (const auto &node : sortedNodes) {
    compilerData.nodeMap[node.id] = node;

    auto result = CompileAlgorithmNode(node, options);
    if (!result) {
      return std::unexpected(result.error());
    }

    compilerData.algorithmMap[node.id] = result.value();
  }

  // Step 2: Compile edges
  auto edgeResult = CompileEdges(validatedGraph, compilerData);
  if (!edgeResult) {
    return std::unexpected("Failed to compile edges");
  }

  // Step 3: Generate final metadata
  try {
    auto metadata = CreatePartialTradeSignalMetaData(
        CreateSortedEdges(compilerData.algorithmMap));
    metadata.options = options;
    return metadata;
  } catch (const std::exception &e) {
    return std::unexpected(std::format("Compilation failed: {}", e.what()));
  }
}

} // namespace epoch_metadata::strategy