#pragma once

#include "parser/ast_nodes.h"
#include "compilation_context.h"
#include <memory>
#include <unordered_map>
#include <optional>

namespace epoch_script {

/**
 * @brief Constant folding preprocessing pass for the EpochScript compiler.
 *
 * This class implements a two-pass algorithm to resolve constant expressions
 * at compile-time, enabling features like:
 * - Using constant variables in subscript operations: src.v[lookback_period]
 * - Evaluating constant arithmetic: period = 10 + 5
 * - Propagating constants through expressions
 *
 * The preprocessing pass transforms the AST before compilation by:
 * 1. Identifying constant assignments (e.g., lookback_period = 20)
 * 2. Replacing Name references with their Constant values
 *
 * This is analogous to C++ template metaprogramming, where constants are
 * resolved at compile-time rather than runtime.
 */
class ConstantFolder
{
public:
    /**
     * @brief Constructs a ConstantFolder with a compilation context.
     * @param context The compilation context for tracking constants
     */
    explicit ConstantFolder(CompilationContext& context);

    /**
     * @brief Preprocesses a module to fold all constant expressions.
     *
     * This is the main entry point. It performs two passes:
     * 1. Identify constants: Scan assignments to find compile-time constants
     * 2. Fold constants: Replace Name nodes with their Constant values
     *
     * @param module The AST module to preprocess
     * @return The transformed module with constants folded
     */
    ModulePtr PreprocessModule(ModulePtr module);

private:
    CompilationContext& context_;

    /**
     * @brief Table of constant values discovered during preprocessing.
     * Maps variable names to their compile-time constant values.
     */
    std::unordered_map<std::string, Constant::Value> constant_table_;

    /**
     * @brief First pass: Identifies constant assignments in the module.
     *
     * Scans through all statements and identifies assignments where the
     * right-hand side is a compile-time constant expression.
     *
     * @param module The module to scan for constants
     */
    void IdentifyConstants(Module& module);

    /**
     * @brief Second pass: Folds constants by replacing Names with Constants.
     *
     * Walks through all statements and recursively transforms expressions,
     * replacing Name nodes that reference constants with Constant nodes.
     *
     * @param module The module to transform
     */
    void FoldConstants(Module& module);

    /**
     * @brief Checks if an expression is a compile-time constant.
     *
     * An expression is constant if:
     * - It's a Constant literal (20, 3.14, "text", true)
     * - It's a Name that references a known constant
     * - It's a UnaryOp with a constant operand (-20, not true)
     * - It's a BinOp with constant operands (10 + 20)
     *
     * @param expr The expression to check
     * @return true if the expression is a compile-time constant
     */
    bool IsConstantExpression(const Expr& expr);

    /**
     * @brief Evaluates a constant expression at compile-time.
     *
     * Recursively evaluates an expression tree to produce a constant value.
     * Returns std::nullopt if the expression is not a constant.
     *
     * @param expr The expression to evaluate
     * @return The constant value, or nullopt if not constant
     */
    std::optional<Constant::Value> EvaluateConstant(const Expr& expr);

    /**
     * @brief Transforms an expression by replacing Names with Constants.
     *
     * Recursively walks the expression tree and replaces Name nodes that
     * reference constants with Constant nodes containing their values.
     *
     * @param expr The expression to transform
     * @return The transformed expression
     */
    ExprPtr TransformExpression(ExprPtr expr);

    /**
     * @brief Evaluates a unary operation at compile-time.
     *
     * Handles operators: +, -, not
     *
     * @param op The unary operator
     * @param operand The operand value
     * @return The result of the operation
     */
    std::optional<Constant::Value> EvaluateUnaryOp(
        UnaryOpType op,
        const Constant::Value& operand
    );

    /**
     * @brief Evaluates a binary operation at compile-time.
     *
     * Handles operators: +, -, *, /, %, ==, !=, <, >, <=, >=, and, or
     *
     * @param left The left operand value
     * @param op The binary operator
     * @param right The right operand value
     * @return The result of the operation
     */
    std::optional<Constant::Value> EvaluateBinOp(
        const Constant::Value& left,
        BinOpType op,
        const Constant::Value& right
    );

    /**
     * @brief Converts a Constant::Value to a numeric type for arithmetic.
     *
     * @param value The value to convert
     * @return The numeric value (int or double), or nullopt if not numeric
     */
    std::optional<std::variant<int, double>> ToNumeric(const Constant::Value& value);

    /**
     * @brief Converts a Constant::Value to a boolean for logical operations.
     *
     * @param value The value to convert
     * @return The boolean value, or nullopt if not convertible
     */
    std::optional<bool> ToBool(const Constant::Value& value);
};

} // namespace epoch_script
