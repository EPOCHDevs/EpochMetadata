//
// Created by Claude Code
// EpochFlow Node Builder
//
// Handles AlgorithmNode creation, input wiring, and assignment statements.
// Coordinates with TypeChecker, OptionValidator, and SpecialParameterHandler.
//

#pragma once

#include "compilation_context.h"
#include "type_checker.h"
#include "option_validator.h"
#include "special_parameter_handler.h"
#include "constructor_parser.h"
#include "expression_compiler.h"
#include "../parser/ast_nodes.h"
#include <vector>

namespace epoch_stratifyx::epochflow
{

    class NodeBuilder
    {
    public:
        NodeBuilder(
            CompilationContext& context,
            TypeChecker& type_checker,
            OptionValidator& option_validator,
            SpecialParameterHandler& special_param_handler,
            ConstructorParser& constructor_parser,
            ExpressionCompiler& expr_compiler)
            : context_(context),
              type_checker_(type_checker),
              option_validator_(option_validator),
              special_param_handler_(special_param_handler),
              constructor_parser_(constructor_parser),
              expr_compiler_(expr_compiler) {}

        // Handle constructor-based assignment (e.g., x = ema(period=20)(src.c))
        void HandleConstructorAssignment(const Expr& target, const Expr& value, const Assign& assign);

        // Handle non-constructor assignment (e.g., x = src.c or x = a + b)
        void HandleNonConstructorAssignment(const Expr& target, const Expr& value, const Assign& assign);

        // Handle sink node creation (components with no outputs called as statements)
        void HandleSinkNode(const ConstructorParseResult& parse_result, const Call& call);

        // Wire inputs to a target node with type checking
        void WireInputs(
            const std::string& target_node_id,
            const std::string& component_name,
            const std::vector<ValueHandle>& args,
            const std::unordered_map<std::string, ValueHandle>& kwargs);

    private:
        CompilationContext& context_;
        TypeChecker& type_checker_;
        OptionValidator& option_validator_;
        SpecialParameterHandler& special_param_handler_;
        ConstructorParser& constructor_parser_;
        ExpressionCompiler& expr_compiler_;

        // Helper to generate unique node ID
        std::string UniqueNodeId(const std::string& base);

        // Helper to create "node_id#handle" format
        std::string JoinId(const std::string& node_id, const std::string& handle);

        // Helper to resolve SLOT references in node options
        void ResolveSlotReferencesInOptions(
            const std::string& target_node_id,
            const std::vector<ValueHandle>& args);

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);
    };

} // namespace epoch_stratifyx::epochflow
