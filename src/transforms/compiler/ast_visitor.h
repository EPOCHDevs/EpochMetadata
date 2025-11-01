//
// Created by Claude Code
// EpochFlow AST Visitor
//
// Coordinates AST traversal and dispatches to appropriate handlers.
// Entry point for compilation from Python AST to AlgorithmNodes.
//

#pragma once

#include "compilation_context.h"
#include "node_builder.h"
#include "expression_compiler.h"
#include "constructor_parser.h"
#include "parser/ast_nodes.h"

namespace epochflow
{

    class AstVisitor
    {
    public:
        AstVisitor(
            CompilationContext& context,
            NodeBuilder& node_builder,
            ExpressionCompiler& expr_compiler,
            ConstructorParser& constructor_parser)
            : context_(context),
              node_builder_(node_builder),
              expr_compiler_(expr_compiler),
              constructor_parser_(constructor_parser) {}

        // Visit the module (top-level entry point)
        void VisitModule(const Module& module);

        // Visit a statement
        void VisitStmt(const Stmt& stmt);

        // Visit specific statement types
        void VisitAssign(const Assign& assign);
        void VisitExprStmt(const ExprStmt& expr_stmt);

    private:
        CompilationContext& context_;
        NodeBuilder& node_builder_;
        ExpressionCompiler& expr_compiler_;
        ConstructorParser& constructor_parser_;

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);
    };

} // namespace epochflow
