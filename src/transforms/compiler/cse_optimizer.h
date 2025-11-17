//
// Created by Claude Code
// Common Subexpression Elimination (CSE) Optimizer
//
// Optimizes the compiled algorithm graph by deduplicating semantically
// identical transform nodes. This reduces redundant computation without
// requiring any changes to user code.
//

#pragma once

#include "compilation_context.h"
#include <epoch_script/strategy/metadata.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace epoch_script
{

/**
 * @brief Common Subexpression Elimination optimization pass.
 *
 * This optimizer identifies and eliminates duplicate transform nodes
 * that have identical semantics (same type, parameters, inputs, timeframe,
 * and session). It performs three steps:
 *
 * 1. **Identify duplicates**: Use semantic hashing to find nodes that
 *    compute the same result
 * 2. **Rewrite references**: Update all "node_id#handle" references to
 *    point to the canonical (first) occurrence
 * 3. **Remove duplicates**: Delete redundant nodes from the graph
 *
 * Benefits:
 * - Automatic optimization (transparent to users)
 * - Handles all patterns: redundant transforms, multi-output deduplication,
 *   lag operation caching
 * - Significant performance improvement for complex strategies
 *
 * Example transformation:
 * @code
 * // Before CSE:
 * signal1 = ema(period=20)(src.c) > 100
 * signal2 = ema(period=20)(src.c) > ema(period=50)(src.c)
 * signal3 = src.c > ema(period=20)(src.c)
 * // Creates 3 ema(20) nodes: ema_0, ema_1, ema_2
 *
 * // After CSE:
 * // Creates 1 ema(20) node: ema_0 (reused by all 3 signals)
 * // Creates 1 ema(50) node: ema_3
 * @endcode
 */
class CSEOptimizer
{
public:
    /**
     * @brief Constructs a CSE optimizer.
     */
    explicit CSEOptimizer();

    /**
     * @brief Optimizes an algorithm graph by eliminating duplicate nodes.
     *
     * This is the main entry point. It modifies the algorithms vector
     * in-place and updates the compilation context to reflect the changes.
     *
     * The optimization preserves:
     * - Semantic correctness (only deduplicates truly identical transforms)
     * - Topological ordering dependencies
     * - Executor nodes (never deduplicated, as they have side effects)
     *
     * @param algorithms The vector of algorithm nodes to optimize (modified in-place)
     * @param context The compilation context (used_node_ids is updated)
     */
    void Optimize(std::vector<strategy::AlgorithmNode>& algorithms,
                  CompilationContext& context);

private:
    /**
     * @brief Computes a semantic hash for an algorithm node.
     *
     * The hash is based on all semantically relevant fields:
     * - type (transform name)
     * - options (all parameters)
     * - inputs (input connections)
     * - timeframe (if specified)
     * - session (if specified)
     *
     * The node ID is explicitly excluded, as we want to hash based on
     * *what* the node computes, not *which* node it is.
     *
     * @param node The node to hash
     * @return A hash value representing the node's semantics
     */
    size_t ComputeSemanticHash(const strategy::AlgorithmNode& node) const;

    /**
     * @brief Checks if two nodes are semantically equivalent.
     *
     * Two nodes are semantically equivalent if they have identical:
     * - type
     * - options
     * - inputs
     * - timeframe
     * - session
     *
     * The node ID is explicitly excluded from comparison.
     *
     * This is used to verify hash collisions (hash equality doesn't
     * guarantee semantic equality).
     *
     * @param a First node
     * @param b Second node
     * @return true if nodes are semantically equivalent
     */
    bool SemanticEquals(const strategy::AlgorithmNode& a,
                       const strategy::AlgorithmNode& b) const;

    /**
     * @brief Checks if a node type should be excluded from CSE.
     *
     * Some node types have side effects and should never be deduplicated:
     * - Executors (trade_signal_executor, trade_manager_executor, etc.)
     * - Data sources with side effects
     *
     * @param type The node type to check
     * @return true if the node should never be deduplicated
     */
    bool ShouldExcludeFromCSE(const std::string& type) const;

    /**
     * @brief Checks if a node type is a scalar/literal type.
     *
     * Scalar types (text, number, bool_true, bool_false, null_number) are
     * timeframe and session agnostic. They should be deduplicated purely
     * based on type, options, and inputs, ignoring timeframe/session differences.
     *
     * @param type The node type to check
     * @return true if the node is a scalar/literal type
     */
    bool IsScalarType(const std::string& type) const;

    /**
     * @brief Extracts the node ID from a "node_id#handle" reference.
     *
     * Input references are stored in the format "node_id#handle".
     * This helper extracts just the node_id portion.
     *
     * @param ref The reference string (e.g., "ema_0#result")
     * @return The node ID portion (e.g., "ema_0")
     */
    std::string ExtractNodeId(const std::string& ref) const;

    /**
     * @brief Combines a hash value with a seed using a standard algorithm.
     *
     * This uses the same hash_combine formula used in MetaDataOptionDefinition:
     * seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2)
     *
     * @param seed The seed to combine with (modified in-place)
     * @param h The hash value to combine
     */
    void HashCombine(size_t& seed, size_t h) const;

    /**
     * @brief Hashes a SessionVariant value.
     *
     * Handles both SessionRange and SessionType variants.
     *
     * @param session The session variant to hash
     * @return The hash value
     */
    size_t HashSession(const strategy::SessionVariant& session) const;
};

} // namespace epoch_script
