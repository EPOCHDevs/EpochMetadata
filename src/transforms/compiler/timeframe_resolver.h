//
// Created by Claude Code
// EpochFlow Timeframe Resolution Utility
//
// Resolves timeframes for nodes based on their inputs.
// Follows the pattern from trade_signals.cpp TimeframeResolutionCache.
//

#pragma once

#include <epochflow/strategy/metadata.h>
#include <epochflow/core/time_frame.h>
#include <unordered_map>
#include <vector>
#include <optional>
#include <string>

namespace epochflow
{

    // Timeframe resolution cache - resolves and caches node timeframes
    // Algorithm: If node has explicit timeframe, use it. Otherwise, resolve from inputs
    // by taking the maximum (lowest resolution) timeframe from all inputs.
    class TimeframeResolver
    {
    public:
        TimeframeResolver() = default;

        // Resolve timeframe for a node given its input IDs and optional base timeframe
        // Returns cached result if available, otherwise computes and caches
        std::optional<epochflow::TimeFrame> ResolveTimeframe(
            const std::string &nodeId,
            const std::vector<std::string> &inputIds,
            const std::optional<epochflow::TimeFrame> &baseTimeframe);

        // Resolve timeframe for a single AlgorithmNode
        std::optional<epochflow::TimeFrame> ResolveNodeTimeframe(
            const epochflow::strategy::AlgorithmNode &node,
            const std::optional<epochflow::TimeFrame> &baseTimeframe);

        // Cache of resolved timeframes: nodeId -> resolved timeframe
        std::unordered_map<std::string, std::optional<epochflow::TimeFrame>> nodeTimeframes;

    private:
        // Extract input node IDs from "node_id#handle" format
        std::vector<std::string> extractInputNodeIds(const std::vector<std::string> &inputIds);
    };

} // namespace epochflow
