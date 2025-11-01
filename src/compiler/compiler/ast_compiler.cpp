//
// Created by Claude Code
// EpochFlow AST Compiler - Refactored Facade Implementation
//

#include "ast_compiler.h"
#include "../parser/python_parser.h"
#include <stdexcept>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace epoch_stratifyx::epochflow
{
    // Helper function to extract node_id from "node_id#handle" reference
    static std::string extractNodeId(const std::string& ref)
    {
        auto hash_pos = ref.find('#');
        if (hash_pos != std::string::npos)
        {
            return ref.substr(0, hash_pos);
        }
        return ref;
    }

    // Topological sort using Kahn's algorithm (BFS-based)
    // Returns nodes in dependency order: dependencies before dependents
    static std::vector<epoch_metadata::strategy::AlgorithmNode> TopologicalSort(
        std::vector<epoch_metadata::strategy::AlgorithmNode> nodes)
    {
        using AlgorithmNode = epoch_metadata::strategy::AlgorithmNode;

        // Build node index: node_id -> position in input vector
        std::unordered_map<std::string, size_t> node_index;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            node_index[nodes[i].id] = i;
        }

        // Build dependency graph
        std::unordered_map<std::string, int> in_degree;  // node_id -> count of dependencies
        std::unordered_map<std::string, std::vector<std::string>> dependents;  // node_id -> nodes that depend on it

        // Initialize in-degree to 0 for all nodes
        for (const auto& node : nodes)
        {
            in_degree[node.id] = 0;
        }

        // Calculate in-degrees and build adjacency list
        for (const auto& node : nodes)
        {
            const std::string& node_id = node.id;

            // Process all inputs to find dependencies
            for (const auto& [input_name, refs] : node.inputs)
            {
                for (const auto& ref : refs)
                {
                    std::string dep_id = extractNodeId(ref);

                    // Only count dependency if it's an internal node (not external like "src")
                    if (node_index.find(dep_id) != node_index.end())
                    {
                        in_degree[node_id]++;
                        dependents[dep_id].push_back(node_id);
                    }
                }
            }
        }

        // Kahn's algorithm: start with nodes that have no dependencies
        std::queue<std::string> queue;
        for (const auto& [node_id, degree] : in_degree)
        {
            if (degree == 0)
            {
                queue.push(node_id);
            }
        }

        // Process nodes in topological order
        std::vector<AlgorithmNode> sorted_nodes;
        sorted_nodes.reserve(nodes.size());

        while (!queue.empty())
        {
            std::string node_id = queue.front();
            queue.pop();

            // Add this node to sorted output
            sorted_nodes.push_back(std::move(nodes[node_index[node_id]]));

            // Decrease in-degree for all dependents
            for (const std::string& dependent_id : dependents[node_id])
            {
                in_degree[dependent_id]--;
                if (in_degree[dependent_id] == 0)
                {
                    queue.push(dependent_id);
                }
            }
        }

        // Check for cycles
        if (sorted_nodes.size() != nodes.size())
        {
            // Collect nodes that are part of the cycle
            std::vector<std::string> remaining_nodes;
            for (const auto& [node_id, degree] : in_degree)
            {
                if (degree > 0)
                {
                    remaining_nodes.push_back(node_id);
                }
            }

            std::string error_msg = "Circular dependency detected in algorithm graph! Remaining nodes: ";
            for (size_t i = 0; i < remaining_nodes.size(); ++i)
            {
                if (i > 0) error_msg += ", ";
                error_msg += remaining_nodes[i];
            }
            throw std::runtime_error(error_msg);
        }

        return sorted_nodes;
    }

    AlgorithmAstCompiler::AlgorithmAstCompiler()
    {
        initializeComponents();
    }

    void AlgorithmAstCompiler::initializeComponents()
    {
        // Initialize all components in dependency order
        type_checker_ = std::make_unique<TypeChecker>(context_);
        option_validator_ = std::make_unique<OptionValidator>(context_);
        special_param_handler_ = std::make_unique<SpecialParameterHandler>(context_);

        // ExpressionCompiler and ConstructorParser have circular dependency
        // Initialize ExpressionCompiler first, then ConstructorParser, then wire them together
        expr_compiler_ = std::make_unique<ExpressionCompiler>(
            context_,
            *type_checker_,
            *option_validator_,
            *special_param_handler_);
        constructor_parser_ = std::make_unique<ConstructorParser>(context_, *expr_compiler_);

        // Wire circular dependency
        expr_compiler_->SetConstructorParser(constructor_parser_.get());

        // Initialize remaining components
        node_builder_ = std::make_unique<NodeBuilder>(
            context_,
            *type_checker_,
            *option_validator_,
            *special_param_handler_,
            *constructor_parser_,
            *expr_compiler_);

        ast_visitor_ = std::make_unique<AstVisitor>(
            context_,
            *node_builder_,
            *expr_compiler_,
            *constructor_parser_);

        // Initialize constant folder for preprocessing
        constant_folder_ = std::make_unique<ConstantFolder>(context_);
    }

    CompilationResult AlgorithmAstCompiler::compile(const std::string& source)
    {
        // Parse Python source to AST
        PythonParser parser;
        auto module = parser.parse(source);

        // Compile AST directly to AlgorithmNode structures
        return compileAST(std::move(module));
    }

    CompilationResult AlgorithmAstCompiler::compileAST(ModulePtr module)
    {
        // Clear state for fresh compilation
        context_.algorithms.clear();
        context_.executor_count = 0;
        context_.node_lookup.clear();
        context_.var_to_binding.clear();
        context_.node_output_types.clear();
        context_.used_node_ids.clear();

        // Reserve capacity to prevent reallocations (typical algorithm has 50-500 nodes)
        context_.algorithms.reserve(500);

        // Preprocess module to fold constants (like C++ template metaprogramming)
        // This enables constant variables in subscripts: src.v[lookback_period]
        module = constant_folder_->PreprocessModule(std::move(module));

        // Visit the module - builds algorithms in AST order (source code order)
        ast_visitor_->VisitModule(*module);

        // Verify session dependencies and auto-create missing sessions nodes
        verifySessionDependencies();

        // Resolve timeframes for all nodes using TimeframeResolver utility
        resolveTimeframes(std::nullopt); // Pass nullopt as base timeframe for now

        // Sort algorithms in topological order: dependencies before dependents
        // This ensures handles are registered before they're referenced
        context_.algorithms = TopologicalSort(std::move(context_.algorithms));

        // Update node_lookup indices after reordering
        context_.node_lookup.clear();
        for (size_t i = 0; i < context_.algorithms.size(); ++i)
        {
            context_.node_lookup[context_.algorithms[i].id] = i;
        }

        // Return results - move semantics for zero-copy
        return std::move(context_.algorithms);
    }

    void AlgorithmAstCompiler::verifySessionDependencies()
    {
        special_param_handler_->VerifySessionDependencies();
    }

    void AlgorithmAstCompiler::resolveTimeframes(const std::optional<epoch_metadata::TimeFrame>& base_timeframe)
    {
        // Use TimeframeResolver utility to resolve timeframes for all nodes
        // Create fresh resolver instance to avoid stale cache from previous compilations
        TimeframeResolver resolver;

        for (auto& algo : context_.algorithms)
        {
            auto resolved_timeframe = resolver.ResolveNodeTimeframe(algo, base_timeframe);
            if (resolved_timeframe)
            {
                algo.timeframe = resolved_timeframe;
            }
        }
    }

    // Convenience function
    CompilationResult compileAlgorithm(const std::string& source)
    {
        AlgorithmAstCompiler compiler;
        return compiler.compile(source);
    }

} // namespace epoch_stratifyx::epochflow
