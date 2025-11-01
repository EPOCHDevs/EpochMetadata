//
// Created by Claude Code
// EpochScript Timeframe Resolution Utility Implementation
//

#include "timeframe_resolver.h"
#include <algorithm>

namespace epoch_script
{

    std::vector<std::string> TimeframeResolver::extractInputNodeIds(const std::vector<std::string> &inputIds)
    {
        std::vector<std::string> nodeIds;
        nodeIds.reserve(inputIds.size());

        for (const auto &handleId : inputIds)
        {
            // Extract node ID from "node_id#handle" format
            auto hashPos = handleId.find("#");
            if (hashPos != std::string::npos)
            {
                nodeIds.push_back(handleId.substr(0, hashPos));
            }
            else
            {
                // Fallback: treat entire string as node ID
                nodeIds.push_back(handleId);
            }
        }

        return nodeIds;
    }

    std::optional<epoch_script::TimeFrame> TimeframeResolver::ResolveTimeframe(
        const std::string &nodeId,
        const std::vector<std::string> &inputIds,
        const std::optional<epoch_script::TimeFrame> &baseTimeframe)
    {
        // Check cache first
        if (nodeTimeframes.contains(nodeId))
        {
            return nodeTimeframes[nodeId];
        }

        std::optional<epoch_script::TimeFrame> resolvedTimeframe;

        // If we have inputs, resolve from them
        if (!inputIds.empty())
        {
            std::vector<epoch_script::TimeFrame> inputTimeframes;
            inputTimeframes.reserve(inputIds.size());

            // Extract node IDs from "node_id#handle" format
            auto nodeIds = extractInputNodeIds(inputIds);

            for (const auto &inputNodeId : nodeIds)
            {
                if (nodeTimeframes.contains(inputNodeId) && nodeTimeframes[inputNodeId])
                {
                    inputTimeframes.push_back(nodeTimeframes[inputNodeId].value());
                }
            }

            // Find the lowest resolution (highest timeframe value) using operator<
            // TimeFrame::operator< is defined such that higher resolution < lower resolution
            // So std::max_element gives us the lowest resolution timeframe
            if (!inputTimeframes.empty())
            {
                auto maxTimeframe = *std::max_element(inputTimeframes.begin(), inputTimeframes.end());
                resolvedTimeframe = maxTimeframe;
            }
        }

        // Fall back to base timeframe if no inputs or no input timeframes found
        if (!resolvedTimeframe)
        {
            resolvedTimeframe = baseTimeframe;
        }

        // Cache the result
        nodeTimeframes[nodeId] = resolvedTimeframe;
        return resolvedTimeframe;
    }

    std::optional<epoch_script::TimeFrame> TimeframeResolver::ResolveNodeTimeframe(
        const epoch_script::strategy::AlgorithmNode &node,
        const std::optional<epoch_script::TimeFrame> &baseTimeframe)
    {
        // If node has explicit timeframe, use it and cache it
        if (node.timeframe)
        {
            nodeTimeframes[node.id] = node.timeframe;
            return node.timeframe;
        }

        // Extract input IDs from the node (flatten all input vectors)
        std::vector<std::string> inputIds;
        for (const auto &[key, values] : node.inputs)
        {
            inputIds.insert(inputIds.end(), values.begin(), values.end());
        }

        // Use cache to resolve timeframe
        return ResolveTimeframe(node.id, inputIds, baseTimeframe);
    }

} // namespace epoch_script
