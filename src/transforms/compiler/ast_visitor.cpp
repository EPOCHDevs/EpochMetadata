//
// Created by Claude Code
// EpochScript AST Visitor Implementation
//

#include "ast_visitor.h"
#include <stdexcept>

namespace epoch_script
{

    void AstVisitor::VisitModule(const Module& module)
    {
        for (const auto& stmt : module.body)
        {
            VisitStmt(*stmt);
        }
    }

    void AstVisitor::VisitStmt(const Stmt& stmt)
    {
        if (auto* assign = dynamic_cast<const Assign*>(&stmt))
        {
            VisitAssign(*assign);
        }
        else if (auto* expr_stmt = dynamic_cast<const ExprStmt*>(&stmt))
        {
            VisitExprStmt(*expr_stmt);
        }
    }

    void AstVisitor::VisitAssign(const Assign& assign)
    {
        // Validate: only single target supported
        if (assign.targets.size() != 1)
        {
            ThrowError("Only single assignment supported", assign.lineno, assign.col_offset);
        }

        const Expr* target = assign.targets[0].get();

        // Disallow attribute assignment (e.g., src.c = ...)
        if (dynamic_cast<const Attribute*>(target))
        {
            ThrowError("Assignment to attributes/handles is not allowed", assign.lineno, assign.col_offset);
        }

        // Check if value is a constructor call
        if (constructor_parser_.IsConstructorCall(*assign.value))
        {
            node_builder_.HandleConstructorAssignment(*target, *assign.value, assign);
        }
        else
        {
            // Handle non-constructor assignments (operators, references, etc.)
            node_builder_.HandleNonConstructorAssignment(*target, *assign.value, assign);
        }
    }

    void AstVisitor::VisitExprStmt(const ExprStmt& expr_stmt)
    {
        // Allow direct calls to sink components (components with no outputs)
        const Expr* value = expr_stmt.value.get();

        if (constructor_parser_.IsConstructorCall(*value))
        {
            auto parse_result = constructor_parser_.ParseConstructorAndFeeds(dynamic_cast<const Call&>(*value));

            // Validate component exists
            if (!context_.HasComponent(parse_result.ctor_name))
            {
                ThrowError("Unknown component '" + parse_result.ctor_name + "'", expr_stmt.lineno, expr_stmt.col_offset);
            }

            const auto& comp_meta = context_.GetComponentMetadata(parse_result.ctor_name);

            // Check if component has no outputs (is a sink)
            if (comp_meta.outputs.empty())
            {
                // Delegate sink node creation to NodeBuilder
                node_builder_.HandleSinkNode(parse_result, dynamic_cast<const Call&>(*value));
                return;
            }
            else
            {
                ThrowError("Direct call to component with outputs must be assigned to a variable",
                           expr_stmt.lineno, expr_stmt.col_offset);
            }
        }

        ThrowError("Unsupported expression statement", expr_stmt.lineno, expr_stmt.col_offset);
    }

    void AstVisitor::ThrowError(const std::string& msg, int line, int col)
    {
        std::string full_msg = msg;
        // Tree-sitter provides 1-based line numbers (line 1 is the first line)
        // Only add location info if line > 0
        if (line > 0)
        {
            full_msg += " (line " + std::to_string(line) + ", col " + std::to_string(col) + ")";
        }
        throw std::runtime_error(full_msg);
    }

} // namespace epoch_script
