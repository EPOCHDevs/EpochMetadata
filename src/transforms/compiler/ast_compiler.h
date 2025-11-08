//
// Created by Claude Code
// EpochScript AST Compiler - Refactored Facade
//
// Compiles Python AST directly into AlgorithmNode structures.
// Coordinates specialized compiler components using facade pattern.
//

#pragma once

#include "compilation_context.h"
#include "type_checker.h"
#include "option_validator.h"
#include "special_parameter_handler.h"
#include "constructor_parser.h"
#include "expression_compiler.h"
#include "node_builder.h"
#include "ast_visitor.h"
#include "timeframe_resolver.h"
#include "constant_folder.h"
#include "cse_optimizer.h"
#include "parser/ast_nodes.h"
#include <epoch_script/strategy/metadata.h>
#include <epoch_script/core/time_frame.h>
#include <vector>
#include <optional>
#include <memory>

namespace epoch_script
{

    // Compilation result: topologically sorted algorithms (includes executor)
    using CompilationResult = std::vector<strategy::AlgorithmNode>;

    class AlgorithmAstCompiler
    {
    public:
        // Constructor - initializes all sub-components
        AlgorithmAstCompiler();

        // Main compilation entry point - returns topologically sorted algorithms
        CompilationResult compile(const std::string& source, bool skip_sink_validation = false);

        // Direct AST compilation (for testing)
        CompilationResult compileAST(ModulePtr module, bool skip_sink_validation = false);

        size_t getExecutorCount() const { return context_.executor_count; }

    private:
        // Compilation context (shared state)
        CompilationContext context_;

        // Specialized compiler components
        std::unique_ptr<TypeChecker> type_checker_;
        std::unique_ptr<OptionValidator> option_validator_;
        std::unique_ptr<SpecialParameterHandler> special_param_handler_;
        std::unique_ptr<ConstructorParser> constructor_parser_;
        std::unique_ptr<ExpressionCompiler> expr_compiler_;
        std::unique_ptr<NodeBuilder> node_builder_;
        std::unique_ptr<AstVisitor> ast_visitor_;
        std::unique_ptr<TimeframeResolver> timeframe_resolver_;
        std::unique_ptr<ConstantFolder> constant_folder_;
        std::unique_ptr<CSEOptimizer> cse_optimizer_;

        // Initialization helper
        void initializeComponents();

        // Post-compilation steps
        void verifySessionDependencies();
        void resolveTimeframes(const TimeFrame& base_timeframe, bool skip_sink_validation);
        void removeOrphanNodes(bool skip_sink_validation);
        bool isSinkNode(const std::string& type) const;
    };

    // Convenience function (mirrors Python's compile_algorithm)
    CompilationResult compileAlgorithm(const std::string& source);

} // namespace epoch_script
