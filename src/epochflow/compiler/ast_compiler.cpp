//
// Created by Claude Code
// EpochFlow AST Compiler - Refactored Facade Implementation
//

#include "ast_compiler.h"
#include "../parser/python_parser.h"
#include <stdexcept>

namespace epoch_stratifyx::epochflow
{

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

        // Visit the module - builds algorithms in topological order (Python guarantees this)
        ast_visitor_->VisitModule(*module);

        // Verify session dependencies and auto-create missing sessions nodes
        verifySessionDependencies();

        // Resolve timeframes for all nodes using TimeframeResolver utility
        resolveTimeframes(std::nullopt); // Pass nullopt as base timeframe for now

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
