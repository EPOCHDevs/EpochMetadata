#include "epoch_metadata/strategy/algorithm_validator.h"
#include "epoch_metadata/metadata_options.h"
#include "epoch_metadata/strategy/metadata.h"
#include "epoch_metadata/transforms/metadata.h"
#include "epoch_metadata/transforms/registry.h"
#include <format>
#include <initializer_list>
#include <optional>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <vector>

using namespace epoch_metadata::transforms;
using namespace epoch_metadata::strategy;

constexpr auto MARKET_DATA_SOURCE = "market_data_source";
constexpr auto TRADE_SIGNAL_EXECUTOR = "trade_signal_executor";

namespace epoch_metadata::strategy {
extern std::string JoinId(std::string const &id1, std::string const &id2);

void ValidateNodeOptions(const UINode &node,
                         const TransformsMetaData &transformMetaData,
                         ValidationIssues &issues) {
  std::unordered_map<std::string, UIOption> nodeOptions;

  for (const auto &option : node.options) {
    nodeOptions[option.id] = option;
  }

  for (const auto &option : transformMetaData.options) {
    const bool optionsValuePassed = nodeOptions.contains(option.id) &&
                                    nodeOptions.at(option.id).value.has_value();
    if (!optionsValuePassed && option.isRequired) {
      issues.push_back(
          {ValidationCode::MissingRequiredOption, node,
           std::format("Node '{}' of type '{}' is missing required option '{}' "
                       "({}). This option is mandatory for the transform to "
                       "function correctly.",
                       node.id, node.type, option.id,
                       option.name.empty() ? "No description" : option.name),
           std::format(
               "Add option '{}' with type {} to node '{}'. Suggested value: {}",
               option.id,
               epoch_core::MetaDataOptionTypeWrapper::ToString(option.type),
               node.id,
               option.defaultValue ? option.defaultValue->ToString()
                                   : "required")});
    } else if (optionsValuePassed) {
      const auto &nodeOption = nodeOptions.at(option.id);
      const MetaDataOptionDefinition optionDefinition(nodeOption.value.value());
      if (!optionDefinition.IsType(option.type)) {
        issues.push_back(
            {ValidationCode::InvalidOptionReference, node,
             std::format(
                 "Option '{}' has invalid type for node '{}'. Expected type: "
                 "{} but got: {}. This will cause runtime errors.",
                 option.id, node.id,
                 epoch_core::MetaDataOptionTypeWrapper::ToString(option.type),
                 optionDefinition.ToString()),
             std::format(
                 "Change option '{}' value in node '{}' to type {}. Example: "
                 "{}",
                 option.id, node.id,
                 epoch_core::MetaDataOptionTypeWrapper::ToString(option.type),
                 option.defaultValue ? option.defaultValue->ToString()
                                     : "valid_value")});

      }
      // Validate numeric range for Integer and Decimal types
      else if (option.type == epoch_core::MetaDataOptionType::Integer ||
          option.type == epoch_core::MetaDataOptionType::Decimal) {
        double numericValue = optionDefinition.GetNumericValue();
        if (numericValue < option.min || numericValue > option.max) {
          issues.push_back(
              {ValidationCode::OptionValueOutOfRange, node,
               std::format(
                   "Option '{}' value {} is out of range for node '{}'. "
                   "Expected range: [{}, {}]. This may cause unexpected "
                   "behavior.",
                   option.id, numericValue, node.id, option.min, option.max),
               std::format(
                   "Set option '{}' in node '{}' to a value between {} and "
                   "{}. "
                   "Suggested value: {}",
                   option.id, node.id, option.min, option.max,
                   option.defaultValue
                       ? option.defaultValue->ToString()
                       : std::to_string((option.min + option.max) / 2))});
        }
      }

      if (nodeOption.isExposed) {
        if (!nodeOption.name || nodeOption.name->empty()) {
          issues.push_back(
              {ValidationCode::InvalidOptionReference, node,
               std::format(
                   "Exposed option '{}' for node '{}' has empty display name. "
                   "Users won't know what this option controls.",
                   option.id, node.id),
               std::format("Set a descriptive name for exposed option '{}' in "
                           "node '{}'. Example: \"{}\"",
                           option.id, node.id,
                           option.name.empty() ? "Parameter Name"
                                               : option.name)});
        }
      }
      nodeOptions.erase(option.id);
    }
  }

  for (const auto &option : nodeOptions) {
    issues.push_back(
        {ValidationCode::InvalidOptionReference, node,
         std::format("Unknown option '{}' for node '{}' of type '{}'. This "
                     "option is not supported by this transform.",
                     option.first, node.id, node.type),
         std::format("Remove option '{}' from node '{}' or check if you meant "
                     "a different option name",
                     option.first, node.id)});
  }
}

void BuildNodeConnections(const std::vector<UIEdge> &edges,
                          ValidationCache &cache) {
  cache.inputHandleReferencesPerNode.clear();
  cache.outputHandleReferencesPerNode.clear();

  for (const auto &edge : edges) {
    cache.inputHandleReferencesPerNode[edge.target.id][edge.target.handle]
        .push_back(edge.source);

    cache.outputHandleReferencesPerNode[edge.source.id][edge.source.handle]
        .push_back(edge.target);
  }
}

void ValidateNodeConnections(const UINode &node,
                             const TransformsMetaData &transformMetaData,
                             ValidationCache &cache, ValidationIssues &issues) {

  const auto &inputConnectionsIter =
      cache.inputHandleReferencesPerNode.find(node.id);
  const auto &outputConnectionsIter =
      cache.outputHandleReferencesPerNode.find(node.id);

  using T = decltype(outputConnectionsIter->second);

  auto inputConnections =
      (inputConnectionsIter == cache.inputHandleReferencesPerNode.end())
          ? T{}
          : inputConnectionsIter->second;
  auto outputConnections =
      (outputConnectionsIter == cache.outputHandleReferencesPerNode.end())
          ? T{}
          : outputConnectionsIter->second;

  if (inputConnections.empty() && outputConnections.empty()) {
    issues.push_back({ValidationCode::OrphanedNode, node,
                      std::format("Node '{}' has no connections", node.id),
                      std::format("Connect node '{}' to other nodes via inputs "
                                  "or outputs to make it functional",
                                  node.id)});
  }

  if (transformMetaData.atLeastOneInputRequired && inputConnections.empty()) {
    issues.push_back(
        {ValidationCode::MissingRequiredInput, node,
         std::format("Node '{}' has no input connections", node.id),
         std::format("Connect an appropriate data source or transform output "
                     "to node '{}'",
                     node.id)});
  } else if (!transformMetaData.atLeastOneInputRequired &&
             transformMetaData.inputs.size() != inputConnections.size()) {
    issues.push_back(
        {ValidationCode::InvalidNodeConnection, node,
         std::format("Node '{}' has {} input connections, expected {}", node.id,
                     inputConnections.size(), transformMetaData.inputs.size()),
         std::format("Adjust connections for node '{}' to match the transform "
                     "specification ({} inputs required)",
                     node.id, transformMetaData.inputs.size())});
  }

  // Test only association
  for (auto const &[isInput, ioConnection] :
       std::initializer_list<std::pair<bool, decltype(inputConnections)>>{
           {true, inputConnections}, {false, outputConnections}}) {
    auto handles =
        isInput ? transformMetaData.inputs : transformMetaData.outputs;
    for (const auto &handle : handles) {
      if (!ioConnection.contains(handle.id)) {
        continue;
      }

      const auto &connectedHandles = ioConnection.at(handle.id);
      if (connectedHandles.size() > 1 && !handle.allowMultipleConnections) {
        issues.push_back(
            {ValidationCode::InvalidNodeConnection, node,
             std::format("Node '{}' has multiple {} connections "
                         "but handle '{}' requires only one",
                         node.id, isInput ? "input" : "output", handle.id),
             std::format("Remove extra connections to handle '{}' of node '{}' "
                         "- only one connection allowed",
                         handle.id, node.id)});
      }
    }
    // maybe test all connections to handle are smae type
  }
}

void ValidateNode(const UIData &graph, ValidationCache &cache,
                  ValidationIssues &issues) {
  const auto &registry = ITransformRegistry::GetInstance().GetMetaData();
  BuildNodeConnections(graph.edges, cache);

  for (const auto &node : graph.nodes) {
    if (node.id.empty()) {
      issues.push_back({ValidationCode::InvalidNodeId, node,
                        "Node has empty id",
                        "Set a unique, non-empty ID for this node"});
    }

    if (!registry.contains(node.type)) {
      issues.push_back(
          {ValidationCode::UnknownNodeType, node,
           std::format("Unknown node type '{}' for node '{}'", node.type,
                       node.id),
           std::format("Change node '{}' type to a valid transform type or "
                       "register the '{}' transform",
                       node.id, node.type)});
      cache.nodeMap[node.id] = {node, std::nullopt};
    } else {
      const auto &transformMetaData = registry.at(node.type);
      cache.nodeMap[node.id] = {node, transformMetaData};
      ValidateNodeOptions(node, transformMetaData, issues);
      ValidateNodeConnections(node, transformMetaData, cache, issues);
    }

    if (cache.validatedNodeIds.contains(node.id)) {
      issues.push_back(
          {ValidationCode::InvalidNodeId, node,
           std::format("Duplicate node id '{}'", node.id),
           std::format("Change node id '{}' to a unique identifier", node.id)});
    }

    cache.validatedNodeIds.insert(node.id);
  }
}

void ValidateEdgeDataType(const UIEdge &edge,
                          const epoch_core::IODataType &sourceHandleDataType,
                          const epoch_core::IODataType &targetHandleDataType,
                          ValidationIssues &issues) {

  std::unordered_set<epoch_core::IODataType> types{sourceHandleDataType,
                                                   targetHandleDataType};

  if (types.size() == 1) {
    return;
  }

  if (types.contains(epoch_core::IODataType::Any)) {
    return;
  }

  for (auto mustMatchTypes :
       {epoch_core::IODataType::Boolean, epoch_core::IODataType::String}) {
    if (types.contains(mustMatchTypes) && types.size() == 2) {
      issues.push_back(
          {ValidationCode::InvalidEdge, edge,
           std::format(
               "Edge references source handle '{}' for node '{}' "
               "with type '{}' but expected type '{}'",
               edge.source.handle, edge.source.id,
               epoch_core::IODataTypeWrapper::ToString(sourceHandleDataType),
               epoch_core::IODataTypeWrapper::ToString(targetHandleDataType)),
           std::format(
               "Connect a {} output to node '{}' instead of {} - these types "
               "must match exactly",
               epoch_core::IODataTypeWrapper::ToString(targetHandleDataType),
               edge.target.id,
               epoch_core::IODataTypeWrapper::ToString(sourceHandleDataType))});
      return;
    }
  }

  if (types.contains(epoch_core::IODataType::Number)) {
    std::unordered_set<epoch_core::IODataType> compatibleNumberTypes = {
        epoch_core::IODataType::Number, epoch_core::IODataType::Decimal,
        epoch_core::IODataType::Integer};

    if (compatibleNumberTypes.contains(sourceHandleDataType) &&
        compatibleNumberTypes.contains(targetHandleDataType)) {
      return;
    }
    if (!compatibleNumberTypes.contains(sourceHandleDataType)) {
      issues.push_back(
          {ValidationCode::InvalidEdge, edge,
           std::format(
               "Edge references source handle '{}' for node '{}' "
               "with type '{}' but expected type '{}'",
               edge.source.handle, edge.source.id,
               epoch_core::IODataTypeWrapper::ToString(sourceHandleDataType),
               epoch_core::IODataTypeWrapper::ToString(targetHandleDataType)),
           std::format(
               "Connect a numeric output (Number/Decimal/Integer) to node '{}' "
               "instead of {}",
               edge.target.id,
               epoch_core::IODataTypeWrapper::ToString(sourceHandleDataType))});
      return;
    }
    if (!compatibleNumberTypes.contains(targetHandleDataType)) {
      issues.push_back(
          {ValidationCode::InvalidEdge, edge,
           std::format(
               "Edge references target handle '{}' for node '{}' "
               "with type '{}' but expected type '{}'",
               edge.target.handle, edge.target.id,
               epoch_core::IODataTypeWrapper::ToString(targetHandleDataType),
               epoch_core::IODataTypeWrapper::ToString(sourceHandleDataType)),
           std::format("Change target node '{}' to accept numeric types or "
                       "connect to a different handle",
                       edge.target.id)});
      return;
    }
  }
}

void ValidateEdgeReferences(const std::vector<UIEdge> &edges,
                            const ValidationCache &cache,
                            ValidationIssues &issues) {

  std::unordered_map<std::string,
                     std::unordered_map<std::string, epoch_core::IODataType>>
      ioDataTypePerHandle;

  for (const auto &[nodeId, nodePair] : cache.nodeMap) {
    auto const &[node, transformMetaData] = nodePair;
    if (!transformMetaData) {
      continue;
    }

    for (const auto &handle : transformMetaData->inputs) {
      ioDataTypePerHandle[node.id][handle.id] = handle.type;
    }
    for (const auto &handle : transformMetaData->outputs) {
      ioDataTypePerHandle[node.id][handle.id] = handle.type;
    }
  }

  // Validate each edge
  for (const auto &edge : edges) {
    for (auto const &[tag, uiVertex] :
         {std::pair{"source", edge.source}, std::pair{"target", edge.target}}) {

      if (!ioDataTypePerHandle.contains(uiVertex.id)) {
        issues.push_back({ValidationCode::InvalidEdge, edge,
                          std::format("Edge references unknown {} node '{}'",
                                      tag, uiVertex.id),
                          std::format("Check that node '{}' exists in the "
                                      "graph or fix the edge reference",
                                      uiVertex.id)});
      } else if (!ioDataTypePerHandle.at(uiVertex.id)
                      .contains(uiVertex.handle)) {
        issues.push_back(
            {ValidationCode::InvalidEdge, edge,
             std::format("Edge references unknown {} handle '{}' for node '{}'",
                         tag, uiVertex.handle, uiVertex.id),
             std::format("Check that handle '{}' exists on node '{}' or fix "
                         "the edge reference",
                         uiVertex.handle, uiVertex.id)});
      }

      // Check for empty handles
      if (uiVertex.handle.empty()) {
        issues.push_back(
            {ValidationCode::InvalidEdge, edge,
             std::format("Edge has empty {} handle for node '{}'", tag,
                         uiVertex.id),
             std::format(
                 "Specify a valid handle for the {} connection on node '{}'",
                 tag, uiVertex.id)});
      }
    }

    if (edge.source.id == edge.target.id) {
      issues.push_back({ValidationCode::InvalidEdge, edge,
                        std::format("Edge has same source and target node '{}' "
                                    "and handle '{}'",
                                    edge.source.id, edge.source.handle),
                        std::format("Remove self-loop edge from node '{}' or "
                                    "connect to a different node",
                                    edge.source.id)});
    }

    if (ioDataTypePerHandle.contains(edge.source.id) &&
        ioDataTypePerHandle.contains(edge.target.id) &&
        ioDataTypePerHandle.at(edge.source.id).contains(edge.source.handle) &&
        ioDataTypePerHandle.at(edge.target.id).contains(edge.target.handle)) {
      const auto &sourceHandleDataType =
          ioDataTypePerHandle.at(edge.source.id).at(edge.source.handle);
      const auto &targetHandleDataType =
          ioDataTypePerHandle.at(edge.target.id).at(edge.target.handle);

      ValidateEdgeDataType(edge, sourceHandleDataType, targetHandleDataType,
                           issues);
    }
  }
}

void ValidateExecutorPresence(const UIData &graph, ValidationIssues &issues) {
  size_t executorCount = 0;
  UINode executorNode;

  for (const auto &node : graph.nodes) {
    if (node.type == TRADE_SIGNAL_EXECUTOR) {
      executorCount++;
      executorNode = node;
    }
  }

  if (executorCount == 0) {
    issues.push_back({ValidationCode::MissingExecutor, std::monostate{},
                      "No TradeSignalExecutor found in graph",
                      "Add a TradeSignalExecutor node to the graph to execute "
                      "trading decisions"});
  } else if (executorCount > 1) {
    issues.push_back(
        {ValidationCode::MultipleExecutors, std::monostate{},
         std::format("Found {} TradeSignalExecutors, expected exactly 1",
                     executorCount),
         "Remove extra TradeSignalExecutor nodes - only one executor is "
         "allowed per strategy"});
  }
}

void ValidateAcyclic(const UIData &graph, ValidationCache &cache,
                     ValidationIssues &issues) {
  if (graph.nodes.empty()) {
    issues.push_back({ValidationCode::EmptyGraph, std::monostate{},
                      "Graph contains no nodes",
                      "Add nodes to the graph to create a trading strategy"});
    return;
  }

  // Build adjacency lists for cycle detection
  std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
  std::unordered_map<std::string, int> inDegreeMap;
  std::unordered_set<std::string> nodeIds;

  // Initialize
  for (const auto &node : graph.nodes) {
    nodeIds.insert(node.id);
    inDegreeMap[node.id] = 0;
    adjacencyList[node.id] = {};
  }

  // Build adjacency list from edges
  for (const auto &edge : graph.edges) {
    if (nodeIds.contains(edge.source.id) && nodeIds.contains(edge.target.id)) {
      adjacencyList[edge.source.id].push_back(edge.target.id);
      inDegreeMap[edge.target.id]++;
    }
  }

  // Kahn's algorithm for cycle detection
  std::queue<std::string> queue;

  for (const auto &[nodeId, inDegree] : inDegreeMap) {
    if (inDegree == 0) {
      queue.push(nodeId);
    }
  }

  while (!queue.empty()) {
    std::string currentNodeId = queue.front();
    queue.pop();
    cache.sortedNodeIds.push_back(
        currentNodeId); // Fill cache with sorted nodes

    for (const std::string &neighbor : adjacencyList[currentNodeId]) {
      inDegreeMap[neighbor]--;
      if (inDegreeMap[neighbor] == 0) {
        queue.push(neighbor);
      }
    }
  }

  // Check for cycles
  if (cache.sortedNodeIds.size() != graph.nodes.size()) {
    issues.push_back(
        {ValidationCode::CycleDetected, std::monostate{},
         std::format("Cycle detected in graph. Processed {} nodes out of {}",
                     cache.sortedNodeIds.size(), graph.nodes.size()),
         "Remove circular dependencies between nodes - data should flow in one "
         "direction"});
  }
}

void ValidateTimeframeConsistency(ValidationCache &cache,
                                  ValidationIssues &issues) {
  for (const auto &nodeId : cache.sortedNodeIds) {
    auto [node, transformMetaData] = cache.nodeMap.at(nodeId);
    if (node.type == MARKET_DATA_SOURCE ||
        !cache.inputHandleReferencesPerNode.contains(nodeId)) {
      continue;
    }

    auto inputConnections = cache.inputHandleReferencesPerNode.at(nodeId);
    if (inputConnections.empty()) {
      continue;
    }

    auto targetTimeframe = node.timeframe;
    std::unordered_set<std::string> timeframes;
    std::optional<TimeFrame> sourceTimeframe;
    std::string sourceNodeId;

    for (const auto &[handleId, inputNodes] : inputConnections) {
      for (const auto &inputNodeVertex : inputNodes) {
        if (!cache.nodeMap.contains(inputNodeVertex.id)) {
          continue;
        }

        auto [inputNode, _] = cache.nodeMap.at(inputNodeVertex.id);
        timeframes.insert(inputNode.timeframe ? inputNode.timeframe->ToString()
                                              : "None");
        sourceTimeframe = inputNode.timeframe;
        sourceNodeId = inputNodeVertex.id;
      }
    }

    if (timeframes.size() > 1) {
      issues.push_back(
          {ValidationCode::TimeframeMismatch, node,
           std::format("Node '{}' has multiple input timeframes: {}", node.id,
                       timeframes.size()),
           std::format("Ensure all inputs to node '{}' have the same timeframe "
                       "or add resampling nodes",
                       node.id)});
      continue;
    }
    if (!targetTimeframe) {
      cache.nodeMap[nodeId].first.timeframe = sourceTimeframe;
    } else if (sourceTimeframe) {
      if (sourceTimeframe->ToString() != targetTimeframe->ToString()) {
        issues.push_back(
            {ValidationCode::TimeframeMismatch, node,
             std::format("Timeframe mismatch: source node '{}' has timeframe "
                         "'{}', target node '{}' has timeframe '{}'",
                         sourceNodeId, sourceTimeframe->ToString(), node.id,
                         targetTimeframe->ToString()),
             std::format("Change node '{}' timeframe to match its input '{}' "
                         "or add a resampling node between them",
                         node.id, sourceTimeframe->ToString())});
      }
    } else {
      issues.push_back(
          {ValidationCode::TimeframeMismatch, node,
           std::format("Timeframe mismatch: source node '{}' has no timeframe, "
                       "target node '{}' has timeframe '{}'",
                       sourceNodeId, node.id, targetTimeframe->ToString()),
           std::format(
               "Change node '{}' timeframe to match its input '{}'. "
               "or let this node inherit the timeframe from the source node ",
               node.id, sourceNodeId)});
    }
  }
}

ValidationResult ValidateUIData(const UIData &graph) {
  ValidationIssues allIssues;
  ValidationCache cache;

  // Run validation phases in proper order:
  // 1. Basic node validation - builds cache
  ValidateNode(graph, cache, allIssues);

  // 2. Edge validation - uses cache
  ValidateEdgeReferences(graph.edges, cache, allIssues);

  // 3. Executor presence check
  ValidateExecutorPresence(graph, allIssues);

  // 4. Cycle detection - IMPORTANT: fills sortedNodeIds in cache
  ValidateAcyclic(graph, cache, allIssues);

  // 5. Timeframe consistency - uses sortedNodeIds and updates timeframes
  ValidateTimeframeConsistency(cache, allIssues);

  if (allIssues.empty()) {
    // Build sorted UINodes vector from cache
    std::vector<UINode> sortedNodes;
    sortedNodes.reserve(cache.sortedNodeIds.size());

    for (const auto &nodeId : cache.sortedNodeIds) {
      auto [node, transformMetaData] = cache.nodeMap.at(nodeId);
      sortedNodes.push_back(node);
    }

    return sortedNodes; // Success - return topologically sorted nodes
  } else {
    return std::unexpected(std::move(allIssues));
  }
}

} // namespace epoch_metadata::strategy