//
// Created by Claude Code
// EpochScript Expression Compiler
//
// Compiles AST expressions into ValueHandles.
// Handles operators, function calls, literals, attributes, and subscripts.
//

#pragma once

#include "compilation_context.h"
#include "type_checker.h"
#include "constructor_parser.h"
#include "parser/ast_nodes.h"

namespace epoch_script
{

    // Forward declarations
    class ConstructorParser;
    class OptionValidator;
    class SpecialParameterHandler;

    class ExpressionCompiler
    {
    public:
        ExpressionCompiler(
            CompilationContext& context,
            TypeChecker& type_checker,
            OptionValidator& option_validator,
            SpecialParameterHandler& special_param_handler)
            : context_(context),
              type_checker_(type_checker),
              option_validator_(option_validator),
              special_param_handler_(special_param_handler),
              constructor_parser_(nullptr) {}

        // Set constructor parser (circular dependency resolved via setter)
        void SetConstructorParser(ConstructorParser* parser) { constructor_parser_ = parser; }

        // Visit expression and return value handle
        ValueHandle VisitExpr(const Expr& expr);

        // Visit specific expression types
        ValueHandle VisitCall(const Call& call);
        ValueHandle VisitAttribute(const Attribute& attr);
        ValueHandle VisitName(const Name& name);
        ValueHandle VisitConstant(const Constant& constant);
        ValueHandle VisitBinOp(const BinOp& binOp);
        ValueHandle VisitUnaryOp(const UnaryOp& unaryOp);
        ValueHandle VisitCompare(const Compare& compare);
        ValueHandle VisitBoolOp(const BoolOp& boolOp);
        ValueHandle VisitIfExp(const IfExp& ifExp);
        ValueHandle VisitSubscript(const Subscript& subscript);

        // Materialize literal nodes
        ValueHandle MaterializeNumber(double value);
        ValueHandle MaterializeBoolean(bool value);
        ValueHandle MaterializeText(const std::string& value);
        ValueHandle MaterializeNull();

    private:
        CompilationContext& context_;
        TypeChecker& type_checker_;
        OptionValidator& option_validator_;
        SpecialParameterHandler& special_param_handler_;
        ConstructorParser* constructor_parser_;

        // Attribute resolution helpers
        std::pair<std::string, std::string> AttributeToTuple(const Attribute& attr);
        ValueHandle ResolveHandle(const std::string& var, const std::string& handle);

        // Helper utilities
        std::string UniqueNodeId(const std::string& base);
        std::string JoinId(const std::string& node_id, const std::string& handle);
        void WireInputs(
            const std::string& target_node_id,
            const std::string& component_name,
            const std::vector<ValueHandle>& args,
            const std::unordered_map<std::string, ValueHandle>& kwargs);

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);
    };

} // namespace epoch_script
