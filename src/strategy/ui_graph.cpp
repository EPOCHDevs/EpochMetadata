#include "epoch_metadata/strategy/ui_graph.h"
#include "epoch_metadata/strategy/validation.h"
#include "epoch_metadata/transforms/metadata.h"
#include "epoch_metadata/transforms/registry.h"
#include <cstdio>
#include <limits>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "epoch_metadata/strategy/metadata.h"

// Graphviz headers
extern "C" {
#include <cgraph.h>
#include <gvc.h>
extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
}

#include "epoch_metadata/time_frame.h"

using namespace epoch_metadata::transforms;

constexpr auto MARKET_DATA_SOURCE = "market_data_source";
constexpr auto TRADE_SIGNAL_EXECUTOR = "trade_signal_executor";
constexpr double POSTSCRIPT_POINTS_PER_INCH = 72.0;

namespace epoch_metadata::strategy {

std::string JoinId(std::string const &id1, std::string const &id2) {
  return std::format("{}#{}", id1, id2);
}

// High-performance topological sort using Kahn's algorithm - O(V+E)
std::vector<AlgorithmNode>
CreateSortedEdges(const std::unordered_map<std::string, AlgorithmNode> &nodes) {

  std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
  std::unordered_map<std::string, int> inDegreeMap;
  std::unordered_set<std::string> nodeIds;

  // Initialize in-degree map and node IDs
  for (const auto &[id, algoNode] : nodes) {
    nodeIds.insert(id);
    inDegreeMap[id] = 0;
    adjacencyList[id] = {}; // Initialize empty adjacency list
  }

  // Build adjacency list and calculate in-degrees
  for (const auto &[id, algoNode] : nodes) {
    for (auto const &inputList : algoNode.inputs | std::ranges::views::values) {
      for (auto const &input : inputList) {
        auto handlePos = input.find('#');
        if (handlePos == std::string::npos) {
          continue;
        }
        const std::string srcNodeId = input.substr(0, handlePos);
        if (nodeIds.contains(srcNodeId)) {
          adjacencyList[srcNodeId].push_back(id);
          inDegreeMap[id]++;
        }
      }
    }
  }

  // Kahn's algorithm for topological sorting
  std::vector<std::string> sortedNodeIds;
  std::queue<std::string> queue;

  // Find nodes with zero in-degree
  for (const auto &[nodeId, inDegree] : inDegreeMap) {
    if (inDegree == 0) {
      queue.push(nodeId);
    }
  }

  // Process nodes in topological order - O(V+E) using adjacency list
  while (!queue.empty()) {
    std::string currentNodeId = queue.front();
    queue.pop();
    sortedNodeIds.push_back(currentNodeId);

    // Process all neighbors of current node
    for (const std::string &neighbor : adjacencyList[currentNodeId]) {
      inDegreeMap[neighbor]--;
      if (inDegreeMap[neighbor] == 0) {
        queue.push(neighbor);
      }
    }
  }

  // Check for cycles
  AssertFromStream(sortedNodeIds.size() == nodes.size(),
                   "Topological sort failed - graph contains cycles");

  // Convert back to algorithm nodes in sorted order
  std::vector<epoch_metadata::strategy::AlgorithmNode> algorithm;
  algorithm.reserve(nodes.size());

  for (const std::string &nodeId : sortedNodeIds) {
    algorithm.emplace_back(nodes.at(nodeId));
  }

  return algorithm;
}

// Graphviz-based layout function with static plugin registration
std::expected<UIData, std::string> PerformAutoLayout(const UIData &inputData) {

  UIData layoutedData = inputData; // Copy input data

  if (layoutedData.nodes.empty() && layoutedData.annotations.empty()) {
    return layoutedData; // No layout needed for empty data
  }

  // Single item case - place at origin
  if (layoutedData.nodes.size() + layoutedData.annotations.size() == 1) {
    if (!layoutedData.nodes.empty()) {
      layoutedData.nodes[0].metadata.position.x = 0;
      layoutedData.nodes[0].metadata.position.y = 0;
      // Keep existing dimensions or use reasonable defaults
      if (layoutedData.nodes[0].metadata.width <= 0) {
        layoutedData.nodes[0].metadata.width = 150.0;
      }
      if (layoutedData.nodes[0].metadata.height <= 0) {
        layoutedData.nodes[0].metadata.height = 50.0;
      }
    } else if (!layoutedData.annotations.empty()) {
      layoutedData.annotations[0].position.x = 0;
      layoutedData.annotations[0].position.y = 0;
      // Keep existing dimensions or use reasonable defaults
      if (layoutedData.annotations[0].width <= 0) {
        layoutedData.annotations[0].width = 100.0;
      }
      if (layoutedData.annotations[0].height <= 0) {
        layoutedData.annotations[0].height = 30.0;
      }
    }
    return layoutedData;
  }

  try {
    // Initialize Graphviz
    GVC_t *gvc = gvContext();
    if (!gvc) {
      return std::unexpected("Failed to initialize Graphviz context");
    }
    gvAddLibrary(
        gvc,
        &gvplugin_dot_layout_LTX_library); // one line does it!
                                           // :contentReference[oaicite:0]{index=0}

    // Create graph with LR direction
    Agraph_t *g = agopen(const_cast<char *>("G"), Agdirected, nullptr);
    if (!g) {
      gvFreeContext(gvc);
      return std::unexpected("Failed to create Graphviz graph");
    }

    // Set minimal graph attributes for dot layout with defaults
    agsafeset(g, const_cast<char *>("rankdir"), const_cast<char *>("LR"),
              const_cast<char *>(""));

    // Essential settings for node-and-flow diagrams - let dot handle optimal
    // spacing
    agsafeset(g, const_cast<char *>("overlap"), const_cast<char *>("false"),
              const_cast<char *>("")); // Prevent node overlaps
    agsafeset(g, const_cast<char *>("splines"), const_cast<char *>("true"),
              const_cast<char *>("")); // Use curved edges
    // Force bounding box computation for clusters
    agsafeset(g, const_cast<char *>("compound"), const_cast<char *>("true"),
              const_cast<char *>("")); // Enable cluster processing
    agsafeset(g, const_cast<char *>("clusterrank"), const_cast<char *>("local"),
              const_cast<char *>("")); // Better cluster layout

    std::unordered_map<std::string, Agnode_t *> nodeMap;

    // Add regular nodes to Graphviz graph with proper dimensions
    for (const auto &uiNode : layoutedData.nodes) {
      Agnode_t *node = agnode(g, const_cast<char *>(uiNode.id.c_str()), 1);
      if (!node)
        continue;

      nodeMap[uiNode.id] = node;

      // Set node shape and dimensions
      agsafeset(node, const_cast<char *>("shape"), const_cast<char *>("box"),
                const_cast<char *>(""));

      // Convert pixel dimensions to inches with reasonable scaling
      double widthInches = uiNode.metadata.width / POSTSCRIPT_POINTS_PER_INCH;
      double heightInches = uiNode.metadata.height / POSTSCRIPT_POINTS_PER_INCH;

      // Set reasonable minimum sizes
      widthInches = std::max(widthInches, 0.8);
      heightInches = std::max(heightInches, 0.4);

      std::string widthStr = std::format("{:.2f}", widthInches);
      std::string heightStr = std::format("{:.2f}", heightInches);

      agsafeset(node, const_cast<char *>("width"),
                const_cast<char *>(widthStr.c_str()), const_cast<char *>(""));
      agsafeset(node, const_cast<char *>("height"),
                const_cast<char *>(heightStr.c_str()), const_cast<char *>(""));
      agsafeset(node, const_cast<char *>("fixedsize"),
                const_cast<char *>("true"), const_cast<char *>(""));
    }

    // Add annotation nodes to Graphviz graph (treat as regular nodes)
    {
      for (const auto &annotation : layoutedData.annotations) {
        Agnode_t *node =
            agnode(g, const_cast<char *>(annotation.id.c_str()), 1);
        if (!node)
          continue;

        nodeMap[annotation.id] = node;

        // Set annotation node shape and dimensions
        agsafeset(node, const_cast<char *>("shape"), const_cast<char *>("note"),
                  const_cast<char *>(""));

        // Convert pixel dimensions to inches
        double widthInches = annotation.width / POSTSCRIPT_POINTS_PER_INCH;
        double heightInches = annotation.height / POSTSCRIPT_POINTS_PER_INCH;

        // Set reasonable minimum sizes for annotations
        widthInches = std::max(widthInches, 0.6);
        heightInches = std::max(heightInches, 0.3);

        std::string widthStr = std::format("{:.2f}", widthInches);
        std::string heightStr = std::format("{:.2f}", heightInches);

        agsafeset(node, const_cast<char *>("width"),
                  const_cast<char *>(widthStr.c_str()), const_cast<char *>(""));
        agsafeset(node, const_cast<char *>("height"),
                  const_cast<char *>(heightStr.c_str()),
                  const_cast<char *>(""));
        agsafeset(node, const_cast<char *>("fixedsize"),
                  const_cast<char *>("true"), const_cast<char *>(""));
      }
    }

    // Add edges to Graphviz graph
    for (const auto &uiEdge : layoutedData.edges) {
      auto sourceIt = nodeMap.find(uiEdge.source.id);
      auto targetIt = nodeMap.find(uiEdge.target.id);
      if (sourceIt != nodeMap.end() && targetIt != nodeMap.end()) {
        agedge(g, sourceIt->second, targetIt->second, nullptr, 1);
      }
    }

    // Add clusters/subgraphs for groups and assign nodes to clusters
    std::unordered_map<std::string, Agraph_t *> clusterMap;
    for (const auto &group : layoutedData.groups) {
      std::string clusterName = "cluster_" + group.id;
      Agraph_t *cluster = agsubg(g, const_cast<char *>(clusterName.c_str()), 1);
      if (cluster) {
        clusterMap[group.id] = cluster;
        agsafeset(cluster, const_cast<char *>("label"),
                  const_cast<char *>(group.label.c_str()),
                  const_cast<char *>(""));
        agsafeset(cluster, const_cast<char *>("style"),
                  const_cast<char *>("rounded"), const_cast<char *>(""));
      }
    }

    // Assign nodes to their respective clusters (only if clusters exist)
    if (!clusterMap.empty()) {
      // Assign regular nodes to clusters
      for (const auto &uiNode : layoutedData.nodes) {
        if (uiNode.metadata.parentId.has_value() &&
            clusterMap.contains(uiNode.metadata.parentId.value()) &&
            nodeMap.contains(uiNode.id)) {
          Agraph_t *cluster = clusterMap[uiNode.metadata.parentId.value()];
          Agnode_t *node = nodeMap[uiNode.id];
          agsubnode(cluster, node, 1);
        }
      }

      // Assign annotation nodes to clusters
      for (const auto &annotation : layoutedData.annotations) {
        if (annotation.parentId.has_value() &&
            clusterMap.contains(annotation.parentId.value()) &&
            nodeMap.contains(annotation.id)) {
          Agraph_t *cluster = clusterMap[annotation.parentId.value()];
          Agnode_t *node = nodeMap[annotation.id];
          agsubnode(cluster, node, 1);
        }
      }
    }

    // Apply layout using dot algorithm AFTER clusters are created and populated
    int layoutResult = gvLayout(gvc, g, "dot");
    if (layoutResult != 0) {
      agclose(g);
      gvFreeContext(gvc);
      return std::unexpected("Graphviz dot layout failed - layout engine not "
                             "available or graph invalid");
    }

    // Extract positions and set dimensions in single loop
    constexpr double DPI = 96.0; // Standard desktop DPI
    constexpr double GRAPHVIZ_POINTS_PER_INCH =
        72.0; // Graphviz uses 72 points per inch
    constexpr double GRAPHVIZ_PAD_POINTS = 4.0; // Default pad in points

    // Convert from Graphviz points to pixels: points * (DPI / points_per_inch)
    const double GRAPHVIZ_PT_TO_PX = DPI / GRAPHVIZ_POINTS_PER_INCH;

    // Process regular nodes
    for (auto &uiNode : layoutedData.nodes) {
      // Extract coordinates if node exists in Graphviz
      if (nodeMap.contains(uiNode.id)) {
        Agnode_t *node = nodeMap[uiNode.id];

        // 1. Get Graphviz center coordinates in points, add pad, and convert to
        // pixels
        const double cx =
            (ND_coord(node).x + GRAPHVIZ_PAD_POINTS) * GRAPHVIZ_PT_TO_PX;
        const double cy =
            (ND_coord(node).y + GRAPHVIZ_PAD_POINTS) * GRAPHVIZ_PT_TO_PX;

        // 2. Convert center coordinates to top-left coordinates
        uiNode.metadata.position.x = cx - uiNode.metadata.width / 2.0;
        uiNode.metadata.position.y = cy - uiNode.metadata.height / 2.0;
      }
    }

    // Process annotation nodes
    for (auto &annotation : layoutedData.annotations) {
      // Extract coordinates if annotation exists in Graphviz
      if (nodeMap.contains(annotation.id)) {
        Agnode_t *node = nodeMap[annotation.id];

        // 1. Get Graphviz center coordinates in points, add pad, and convert to
        // pixels
        const double cx =
            (ND_coord(node).x + GRAPHVIZ_PAD_POINTS) * GRAPHVIZ_PT_TO_PX;
        const double cy =
            (ND_coord(node).y + GRAPHVIZ_PAD_POINTS) * GRAPHVIZ_PT_TO_PX;

        // 2. Convert center coordinates to top-left coordinates
        annotation.position.x = cx - annotation.width / 2.0;
        annotation.position.y = cy - annotation.height / 2.0;
      }
    }

    // Extract cluster bounding boxes to compute group dimensions
    for (auto &group : layoutedData.groups) {
      // Compute bounding box from contained nodes
      double minX = std::numeric_limits<double>::max();
      double minY = std::numeric_limits<double>::max();
      double maxX = std::numeric_limits<double>::lowest();
      double maxY = std::numeric_limits<double>::lowest();
      bool foundNodes = false;

      // Check all nodes that belong to this group
      for (const auto &uiNode : layoutedData.nodes) {
        if (uiNode.metadata.parentId.has_value() &&
            uiNode.metadata.parentId.value() == group.id) {
          foundNodes = true;
          minX = std::min(minX, uiNode.metadata.position.x);
          minY = std::min(minY, uiNode.metadata.position.y);
          maxX = std::max(maxX,
                          uiNode.metadata.position.x + uiNode.metadata.width);
          maxY = std::max(maxY,
                          uiNode.metadata.position.y + uiNode.metadata.height);
        }
      }

      // Check annotations too
      for (const auto &annotation : layoutedData.annotations) {
        if (annotation.parentId.has_value() &&
            annotation.parentId.value() == group.id) {
          foundNodes = true;
          minX = std::min(minX, annotation.position.x);
          minY = std::min(minY, annotation.position.y);
          maxX = std::max(maxX, annotation.position.x + annotation.width);
          maxY = std::max(maxY, annotation.position.y + annotation.height);
        }
      }

      if (foundNodes) {
        // Add some padding around the group - increased for better visual
        // spacing
        const double padding = 40.0;       // Increased from 20.0 to 40.0
        const double bottomPadding = 60.0; // Extra padding for bottom

        group.position.x = minX - padding;
        group.position.y = minY - padding;
        group.width = (maxX - minX) + (2 * padding);
        group.height =
            (maxY - minY) + padding + bottomPadding; // More padding at bottom
      } else {
        // Set default dimensions for empty groups
        group.position.x = 0;
        group.position.y = 0;
        group.width = 100;
        group.height = 50;
      }
    }

    // Clean up Graphviz resources
    gvFreeLayout(gvc, g);
    agclose(g);
    gvFreeContext(gvc);

    return layoutedData;

  } catch (const std::exception &e) {
    return std::unexpected(
        std::format("Graphviz layout failed with exception: {}", e.what()));
  }
}

std::unordered_map<std::string, epoch_metadata::MetaDataOption>
CreateOptionMapping(const TransformsMetaData &transformMetaData) {
  std::unordered_map<std::string, epoch_metadata::MetaDataOption>
      optionsMapping;
  for (auto const &targetMetaData : transformMetaData.options) {
    optionsMapping[targetMetaData.id] = targetMetaData;
  }
  return optionsMapping;
}

/**
 * Creates an AlgorithmNode from a given UINode, mapping its options and setting
 * the node's properties.
 */
epoch_metadata::strategy::AlgorithmNode CreateAlgorithmNode(
    const UINode &node,
    std::unordered_map<std::string, epoch_metadata::MetaDataOption> const
        &optionsMapping,
    std::vector<epoch_metadata::MetaDataOption> &options) {
  epoch_metadata::strategy::AlgorithmNode newAlgo;
  newAlgo.id = node.id;
  newAlgo.type = node.type;

  // For every option in the target node...
  for (const auto &[id, value, name, isExposed] : node.options) {
    if (isExposed) {
      AssertFromStream(name.has_value(), "Exposed option must have a name.");
      AssertFalseFromStream(node.type == TRADE_SIGNAL_EXECUTOR,
                            "TradeSignalExecutor options cannot be exposed.");

      epoch_metadata::MetaDataOption transformOption;
      // if option value is provided, use that, otherwise use the default
      if (optionsMapping.contains(id)) {
        transformOption = optionsMapping.at(id);
        if (value) {
          transformOption.defaultValue = value;
        }
      } else {
        transformOption.defaultValue = value;
        transformOption.type = epoch_core::MetaDataOptionType::Decimal;
      }

      transformOption.name = name.value();
      transformOption.id = JoinId(node.id, id);

      options.push_back(transformOption);
      // Set the algorithm option to a reference string.
      newAlgo.options[id] = epoch_metadata::MetaDataArgRef{transformOption.id};
    } else if (value.has_value()) {
      newAlgo.options[id] = value.value();
    }
  }

  // Set the timeframe if present
  if (auto timeframe = node.timeframe) {
    newAlgo.timeframe = timeframe;
  }

  return newAlgo;
}

std::expected<std::optional<epoch_metadata::strategy::AlgorithmNode>,
              std::string>
CreateAlgorithmNode(
    const UINode &node,
    std::vector<epoch_metadata::MetaDataOption> &metadataOptions) {
  const auto &registry = ITransformRegistry::GetInstance().GetMetaData();

  if (!registry.contains(node.type)) {
    return std::unexpected(std::format("Unknown node type: {}", node.type));
  }

  const auto &transformMetadata = registry.at(node.type);
  const auto optionsMapping = CreateOptionMapping(transformMetadata);
  try {
    return CreateAlgorithmNode(node, optionsMapping, metadataOptions);
  } catch (const std::exception &e) {
    return std::unexpected(std::string(e.what()));
  }
  std::unreachable();
}

struct LookUpData {
  std::unordered_map<std::string, epoch_metadata::strategy::AlgorithmNode>
      algorithmMap;
  std::unordered_map<std::string, UINode> nodeMap;
};

std::expected<LookUpData, std::string>
CreateNodeLookup(const UIData &data,
                 std::vector<epoch_metadata::MetaDataOption> &metadataOptions) {
  LookUpData lookupData;

  for (const auto &node : data.nodes) {
    lookupData.nodeMap[node.id] = node;
    auto result = CreateAlgorithmNode(node, metadataOptions);
    if (!result) {
      return std::unexpected(result.error());
    }
    if (result.value()) {
      lookupData.algorithmMap[node.id] = result.value().value();
    }
  }

  return lookupData;
}

std::string ProcessEdge(const UIEdge &edge, LookUpData &lookupData) {
  const auto &registry = ITransformRegistry::GetInstance().GetMetaData();

  auto const &[source, target] = edge;
  if (!lookupData.nodeMap.contains(source.id) ||
      !lookupData.nodeMap.contains(target.id)) {
    return std::format("Invalid edge: {} -> {}", source.id, target.id);
  }

  const auto &sourceNode = lookupData.nodeMap.at(source.id);
  const auto &targetNode = lookupData.nodeMap.at(target.id);

  if (!registry.contains(targetNode.type)) {
    return std::format(
        "Found Unknown target node type: {} while parse edge: {} -> {}",
        targetNode.type, source.id, target.id);
  }
  const auto optionsMapping = CreateOptionMapping(registry.at(targetNode.type));
  // Look up algorithm node for the target.
  auto &algo = lookupData.algorithmMap.at(targetNode.id);

  // Assign the input connection for this edge.
  algo.inputs[target.handle].push_back(JoinId(source.id, source.handle));

  // If there is no timeframe, use the inherited timeframe
  const auto &sourceAlgoIter = lookupData.algorithmMap.find(sourceNode.id);

  if (algo.timeframe) {
    if (sourceAlgoIter != lookupData.algorithmMap.end() &&
        sourceAlgoIter->second.timeframe) {
      const auto sourceNodeTF = sourceAlgoIter->second.timeframe->ToString();
      const auto currentAlgoTF = algo.timeframe->ToString();
      if (sourceNodeTF != currentAlgoTF) {
        return std::format("source is connected from a node with {} timeframe "
                           "while algorithm timeframe is {}.",
                           sourceNodeTF, currentAlgoTF);
      }
    }
    return "";
  }

  if (sourceAlgoIter == lookupData.algorithmMap.end()) {
    if (sourceNode.timeframe) {
      algo.timeframe = sourceNode.timeframe;
    }
  } else if (sourceAlgoIter->second.timeframe) {
    algo.timeframe = sourceAlgoIter->second.timeframe;
  }
  return {};
}

PartialTradeSignalMetaData FinalizeAlgorithmMetaData(
    const std::vector<epoch_metadata::strategy::AlgorithmNode> &algorithm) {
  std::unordered_set<std::string> dataSourceIdList;
  PartialTradeSignalMetaData result;
  result.algorithm.reserve(algorithm.size());

  size_t totalExecutors = 0;
  for (const auto &algo : algorithm) {
    if (algo.type != MARKET_DATA_SOURCE && algo.type != TRADE_SIGNAL_EXECUTOR) {
      auto &algoRef = result.algorithm.emplace_back(algo);
      // remove data source form handle id: data source is provided by system
      for (auto &source_handles : algoRef.inputs | std::views::values) {
        for (auto &source_handle : source_handles) {
          if (auto source_node_id =
                  source_handle.substr(0, source_handle.find('#'));
              dataSourceIdList.contains(source_node_id)) {
            source_handle = source_handle.substr(source_handle.find('#') + 1);
          }
        }
      }
    } else if (algo.type == TRADE_SIGNAL_EXECUTOR) {
      result.executor = algo;
      ++totalExecutors;
    } else {
      dataSourceIdList.insert(algo.id);
    }
  }
  AssertFromStream(totalExecutors == 1,
                   "Expected exactly one executor. Found: " << totalExecutors);
  return result;
}

std::expected<PartialTradeSignalMetaData, ValidationIssues>
CreateAlgorithmMetaData(const UIData &data) {
  return CompileAlgorithmMetaData(data);
}

std::expected<std::vector<UIOption>, std::string> ConvertOptions(
    const epoch_metadata::MetaDataArgDefinitionMapping &options,
    const std::unordered_map<std::string, epoch_metadata::MetaDataOption>
        &optionsMapping = {}) {
  std::vector<UIOption> uiOptions;

  std::stringstream errorStream;
  for (const auto &[id, arg] : options) {
    UIOption opt;
    opt.id = id;
    if (arg.IsType<epoch_metadata::MetaDataArgRef>()) {
      auto ref = arg.GetRef();
      const auto &iter = optionsMapping.find(ref);
      if (iter == optionsMapping.end()) {
        errorStream << "Failed to find ref in metadata options: " << ref
                    << std::endl;
        continue;
      }
      if (iter->second.defaultValue) {
        if (iter->second.defaultValue
                ->IsType<epoch_metadata::MetaDataArgRef>()) {
          errorStream << "metadata default value is not allowed to be a ref: "
                      << ref << std::endl;
          continue;
        }
        opt.value = iter->second.defaultValue->GetVariant();
      }
      opt.isExposed = true;
      opt.name = iter->second.name;
    } else {
      opt.value = arg.GetVariant();
      opt.isExposed = false;
    }

    // For this example, we leave opt.name empty.
    uiOptions.push_back(opt);
  }
  auto error = errorStream.str();
  if (error.empty()) {
    return uiOptions;
  }
  return std::unexpected(error);
}

std::expected<UIData, std::string>
CreateUIData(const PartialTradeSignalMetaData &meta) {
  UIData data;

  // We'll use a lambda to add a UINode if it doesn't already exist.
  // We index nodes by their id.
  std::unordered_map<std::string, std::reference_wrapper<UINode>> nodeLookup;
  auto addNode = [&](const std::string &id, const std::string &type,
                     const std::vector<UIOption> &options = {}) {
    if (nodeLookup.contains(id)) {
      return;
    }

    UINode node;
    node.id = id;
    node.type = type;
    node.options = options;
    // metadata can be default-initialized.
    nodeLookup.emplace(id, data.nodes.emplace_back(std::move(node)));
  };

  // Create the executor node.
  auto executorOptions = ConvertOptions(meta.executor.options);
  if (!executorOptions) {
    return std::unexpected(executorOptions.error());
  }
  addNode(meta.executor.id, meta.executor.type, executorOptions.value());

  std::unordered_map<std::string, epoch_metadata::MetaDataOption> optionsMap;
  for (const auto &option : meta.options) {
    optionsMap[option.id] = option;
  }

  // Create nodes for each algorithm.
  for (const auto &algo : meta.algorithm) {
    auto algoOptions = ConvertOptions(algo.options, optionsMap);
    if (!algoOptions) {
      return std::unexpected(algoOptions.error());
    }
    // Add the algorithm node.
    addNode(algo.id, algo.type, algoOptions.value());
  }

  // Helper lambda to process inputs for a given algorithm node.
  auto processInputs =
      [&](const epoch_metadata::strategy::AlgorithmNode &node) {
        // For each input mapping, create an edge from the source vertex to the
        // node.
        for (const auto &[targetHandle, sourceRefList] : node.inputs) {
          for (const auto &sourceRef : sourceRefList) {
            UIVertex targetVertex{node.id, targetHandle};
            UIVertex sourceVertex;
            auto pos = sourceRef.find('#');
            if (pos != std::string::npos) {
              // Format "sourceNodeId#sourceHandle"
              sourceVertex.id = sourceRef.substr(0, pos);
              sourceVertex.handle = sourceRef.substr(pos + 1);
            } else {
              // Input comes directly from a PriceBar.
              // Create a synthetic PriceBar node with an id based on the
              // handle.
              sourceVertex.id = MARKET_DATA_SOURCE;
              sourceVertex.handle = sourceRef;
              addNode(sourceVertex.id, MARKET_DATA_SOURCE, {});
            }
            data.edges.push_back({sourceVertex, targetVertex});
          }
        }
      };

  // Process inputs for the executor.
  processInputs(meta.executor);
  // Process inputs for each algorithm.
  for (const auto &algo : meta.algorithm) {
    processInputs(algo);
  }

  return data;
}

// Public function to perform horizontal alignment with dot layout
std::expected<UIData, std::string> AlignHorizontally(const UIData &data) {
  return PerformAutoLayout(data);
}

} // namespace epoch_metadata::strategy
