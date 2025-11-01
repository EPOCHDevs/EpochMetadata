#include "constant_folder.h"
#include <cmath>

namespace epoch_stratifyx::epochflow {

ConstantFolder::ConstantFolder(CompilationContext& context)
    : context_(context)
{
}

ModulePtr ConstantFolder::PreprocessModule(ModulePtr module)
{
    if (!module) {
        return module;
    }

    // First pass: Identify constant assignments
    IdentifyConstants(*module);

    // Second pass: Fold constants by replacing Names with Constants
    FoldConstants(*module);

    return module;
}

void ConstantFolder::IdentifyConstants(Module& module)
{
    for (const auto& stmt : module.body) {
        // Check if statement is an assignment
        if (auto* assign = dynamic_cast<Assign*>(stmt.get())) {
            // Check if the right-hand side is a constant expression
            if (assign->value && IsConstantExpression(*assign->value)) {
                auto constant_value = EvaluateConstant(*assign->value);
                if (constant_value.has_value()) {
                    // For each target in the assignment, record the constant value
                    for (const auto& target : assign->targets) {
                        if (auto* name = dynamic_cast<Name*>(target.get())) {
                            constant_table_[name->id] = constant_value.value();
                        }
                    }
                }
            }
        }
    }
}

void ConstantFolder::FoldConstants(Module& module)
{
    for (auto& stmt : module.body) {
        if (auto* assign = dynamic_cast<Assign*>(stmt.get())) {
            // Transform the right-hand side expression
            if (assign->value) {
                assign->value = TransformExpression(std::move(assign->value));
            }
        }
        else if (auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get())) {
            // Transform the expression
            if (expr_stmt->value) {
                expr_stmt->value = TransformExpression(std::move(expr_stmt->value));
            }
        }
    }
}

bool ConstantFolder::IsConstantExpression(const Expr& expr)
{
    // Check different expression types
    if (dynamic_cast<const Constant*>(&expr)) {
        return true;
    }

    if (auto* name = dynamic_cast<const Name*>(&expr)) {
        // Check if this name references a known constant
        return constant_table_.find(name->id) != constant_table_.end();
    }

    if (auto* unary_op = dynamic_cast<const UnaryOp*>(&expr)) {
        return IsConstantExpression(*unary_op->operand);
    }

    if (auto* bin_op = dynamic_cast<const BinOp*>(&expr)) {
        return IsConstantExpression(*bin_op->left) && IsConstantExpression(*bin_op->right);
    }

    // Other expression types are not constant
    return false;
}

std::optional<Constant::Value> ConstantFolder::EvaluateConstant(const Expr& expr)
{
    // Handle Constant literals
    if (auto* constant = dynamic_cast<const Constant*>(&expr)) {
        return constant->value;
    }

    // Handle Name references to constants
    if (auto* name = dynamic_cast<const Name*>(&expr)) {
        auto it = constant_table_.find(name->id);
        if (it != constant_table_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Handle UnaryOp
    if (auto* unary_op = dynamic_cast<const UnaryOp*>(&expr)) {
        auto operand_value = EvaluateConstant(*unary_op->operand);
        if (operand_value.has_value()) {
            return EvaluateUnaryOp(unary_op->op, operand_value.value());
        }
        return std::nullopt;
    }

    // Handle BinOp
    if (auto* bin_op = dynamic_cast<const BinOp*>(&expr)) {
        auto left_value = EvaluateConstant(*bin_op->left);
        auto right_value = EvaluateConstant(*bin_op->right);
        if (left_value.has_value() && right_value.has_value()) {
            return EvaluateBinOp(left_value.value(), bin_op->op, right_value.value());
        }
        return std::nullopt;
    }

    // Handle Compare (e.g., a < b < c)
    if (auto* compare = dynamic_cast<const Compare*>(&expr)) {
        // For simplicity, only handle single comparisons
        if (compare->ops.size() == 1 && compare->comparators.size() == 1) {
            auto left_value = EvaluateConstant(*compare->left);
            auto right_value = EvaluateConstant(*compare->comparators[0]);
            if (left_value.has_value() && right_value.has_value()) {
                return EvaluateBinOp(left_value.value(), compare->ops[0], right_value.value());
            }
        }
        return std::nullopt;
    }

    // Handle BoolOp (e.g., a and b and c)
    if (auto* bool_op = dynamic_cast<const BoolOp*>(&expr)) {
        if (bool_op->values.empty()) {
            return std::nullopt;
        }

        // Evaluate first value
        auto result = EvaluateConstant(*bool_op->values[0]);
        if (!result.has_value()) {
            return std::nullopt;
        }

        // Apply operator with remaining values
        for (size_t i = 1; i < bool_op->values.size(); ++i) {
            auto next_value = EvaluateConstant(*bool_op->values[i]);
            if (!next_value.has_value()) {
                return std::nullopt;
            }
            result = EvaluateBinOp(result.value(), bool_op->op, next_value.value());
            if (!result.has_value()) {
                return std::nullopt;
            }
        }

        return result;
    }

    // Not a constant expression
    return std::nullopt;
}

ExprPtr ConstantFolder::TransformExpression(ExprPtr expr)
{
    if (!expr) {
        return expr;
    }

    // Note: We do NOT transform Name nodes here to avoid creating duplicate number nodes
    // Constants are only transformed inside subscript slice expressions (see below)

    // Recursively transform nested expressions
    if (auto* attribute = dynamic_cast<Attribute*>(expr.get())) {
        attribute->value = TransformExpression(std::move(attribute->value));
        return expr;
    }

    if (auto* call = dynamic_cast<Call*>(expr.get())) {
        call->func = TransformExpression(std::move(call->func));
        for (auto& arg : call->args) {
            arg = TransformExpression(std::move(arg));
        }
        for (auto& [key, value] : call->keywords) {
            value = TransformExpression(std::move(value));
        }
        return expr;
    }

    if (auto* bin_op = dynamic_cast<BinOp*>(expr.get())) {
        bin_op->left = TransformExpression(std::move(bin_op->left));
        bin_op->right = TransformExpression(std::move(bin_op->right));
        return expr;
    }

    if (auto* compare = dynamic_cast<Compare*>(expr.get())) {
        compare->left = TransformExpression(std::move(compare->left));
        for (auto& comparator : compare->comparators) {
            comparator = TransformExpression(std::move(comparator));
        }
        return expr;
    }

    if (auto* bool_op = dynamic_cast<BoolOp*>(expr.get())) {
        for (auto& value : bool_op->values) {
            value = TransformExpression(std::move(value));
        }
        return expr;
    }

    if (auto* unary_op = dynamic_cast<UnaryOp*>(expr.get())) {
        unary_op->operand = TransformExpression(std::move(unary_op->operand));
        return expr;
    }

    if (auto* if_exp = dynamic_cast<IfExp*>(expr.get())) {
        if_exp->test = TransformExpression(std::move(if_exp->test));
        if_exp->body = TransformExpression(std::move(if_exp->body));
        if_exp->orelse = TransformExpression(std::move(if_exp->orelse));
        return expr;
    }

    if (auto* list = dynamic_cast<List*>(expr.get())) {
        for (auto& elt : list->elts) {
            elt = TransformExpression(std::move(elt));
        }
        return expr;
    }

    if (auto* tuple = dynamic_cast<Tuple*>(expr.get())) {
        for (auto& elt : tuple->elts) {
            elt = TransformExpression(std::move(elt));
        }
        return expr;
    }

    if (auto* dict = dynamic_cast<Dict*>(expr.get())) {
        for (auto& key : dict->keys) {
            key = TransformExpression(std::move(key));
        }
        for (auto& value : dict->values) {
            value = TransformExpression(std::move(value));
        }
        return expr;
    }

    if (auto* subscript = dynamic_cast<Subscript*>(expr.get())) {
        subscript->value = TransformExpression(std::move(subscript->value));

        // SPECIAL CASE: Transform constants in subscript slice for lag operations
        // This is the key feature - allowing src.v[lookback_period] or src.v[10 + 5]

        // Check if the slice is a constant expression and evaluate it
        if (IsConstantExpression(*subscript->slice)) {
            auto constant_value = EvaluateConstant(*subscript->slice);
            if (constant_value.has_value()) {
                // Replace with evaluated Constant
                subscript->slice = std::make_unique<Constant>(constant_value.value());
            } else {
                // Not evaluable, transform recursively
                subscript->slice = TransformExpression(std::move(subscript->slice));
            }
        } else {
            // For non-constant slices, recursively transform
            subscript->slice = TransformExpression(std::move(subscript->slice));
        }

        return expr;
    }

    // Default: return expression unchanged
    return expr;
}

std::optional<Constant::Value> ConstantFolder::EvaluateUnaryOp(
    UnaryOpType op,
    const Constant::Value& operand)
{
    switch (op) {
        case UnaryOpType::UAdd: {
            // Unary plus: +x
            auto numeric = ToNumeric(operand);
            if (!numeric.has_value()) {
                return std::nullopt;
            }
            if (std::holds_alternative<int>(numeric.value())) {
                return std::get<int>(numeric.value());
            } else {
                return std::get<double>(numeric.value());
            }
        }

        case UnaryOpType::USub: {
            // Unary minus: -x
            auto numeric = ToNumeric(operand);
            if (!numeric.has_value()) {
                return std::nullopt;
            }
            if (std::holds_alternative<int>(numeric.value())) {
                return -std::get<int>(numeric.value());
            } else {
                return -std::get<double>(numeric.value());
            }
        }

        case UnaryOpType::Not: {
            // Logical not: not x
            auto bool_val = ToBool(operand);
            if (!bool_val.has_value()) {
                return std::nullopt;
            }
            return !bool_val.value();
        }

        default:
            return std::nullopt;
    }
}

std::optional<Constant::Value> ConstantFolder::EvaluateBinOp(
    const Constant::Value& left,
    BinOpType op,
    const Constant::Value& right)
{
    // Handle arithmetic and comparison operators
    switch (op) {
        case BinOpType::Add: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            // If both are ints, return int; otherwise return double
            if (std::holds_alternative<int>(left_num.value()) &&
                std::holds_alternative<int>(right_num.value())) {
                return std::get<int>(left_num.value()) + std::get<int>(right_num.value());
            } else {
                double left_d = std::holds_alternative<int>(left_num.value()) ?
                    static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
                double right_d = std::holds_alternative<int>(right_num.value()) ?
                    static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());
                return left_d + right_d;
            }
        }

        case BinOpType::Sub: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            if (std::holds_alternative<int>(left_num.value()) &&
                std::holds_alternative<int>(right_num.value())) {
                return std::get<int>(left_num.value()) - std::get<int>(right_num.value());
            } else {
                double left_d = std::holds_alternative<int>(left_num.value()) ?
                    static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
                double right_d = std::holds_alternative<int>(right_num.value()) ?
                    static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());
                return left_d - right_d;
            }
        }

        case BinOpType::Mult: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            if (std::holds_alternative<int>(left_num.value()) &&
                std::holds_alternative<int>(right_num.value())) {
                return std::get<int>(left_num.value()) * std::get<int>(right_num.value());
            } else {
                double left_d = std::holds_alternative<int>(left_num.value()) ?
                    static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
                double right_d = std::holds_alternative<int>(right_num.value()) ?
                    static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());
                return left_d * right_d;
            }
        }

        case BinOpType::Div: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            if (right_d == 0.0) {
                return std::nullopt;  // Division by zero
            }
            return left_d / right_d;
        }

        case BinOpType::Mod: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            // For modulo, try to keep as int if both are ints
            if (std::holds_alternative<int>(left_num.value()) &&
                std::holds_alternative<int>(right_num.value())) {
                int right_i = std::get<int>(right_num.value());
                if (right_i == 0) {
                    return std::nullopt;  // Modulo by zero
                }
                return std::get<int>(left_num.value()) % right_i;
            } else {
                double left_d = std::holds_alternative<int>(left_num.value()) ?
                    static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
                double right_d = std::holds_alternative<int>(right_num.value()) ?
                    static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

                if (right_d == 0.0) {
                    return std::nullopt;  // Modulo by zero
                }
                return std::fmod(left_d, right_d);
            }
        }

        case BinOpType::Pow: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            return std::pow(left_d, right_d);
        }

        case BinOpType::Lt: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            return left_d < right_d;
        }

        case BinOpType::Gt: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            return left_d > right_d;
        }

        case BinOpType::LtE: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            return left_d <= right_d;
        }

        case BinOpType::GtE: {
            auto left_num = ToNumeric(left);
            auto right_num = ToNumeric(right);
            if (!left_num.has_value() || !right_num.has_value()) {
                return std::nullopt;
            }

            double left_d = std::holds_alternative<int>(left_num.value()) ?
                static_cast<double>(std::get<int>(left_num.value())) : std::get<double>(left_num.value());
            double right_d = std::holds_alternative<int>(right_num.value()) ?
                static_cast<double>(std::get<int>(right_num.value())) : std::get<double>(right_num.value());

            return left_d >= right_d;
        }

        case BinOpType::Eq: {
            // Equality comparison - handle different types
            if (left.index() != right.index()) {
                return false;
            }

            if (std::holds_alternative<int>(left)) {
                return std::get<int>(left) == std::get<int>(right);
            } else if (std::holds_alternative<double>(left)) {
                return std::get<double>(left) == std::get<double>(right);
            } else if (std::holds_alternative<bool>(left)) {
                return std::get<bool>(left) == std::get<bool>(right);
            } else if (std::holds_alternative<std::string>(left)) {
                return std::get<std::string>(left) == std::get<std::string>(right);
            }

            return std::nullopt;
        }

        case BinOpType::NotEq: {
            // Not equal comparison
            if (left.index() != right.index()) {
                return true;
            }

            if (std::holds_alternative<int>(left)) {
                return std::get<int>(left) != std::get<int>(right);
            } else if (std::holds_alternative<double>(left)) {
                return std::get<double>(left) != std::get<double>(right);
            } else if (std::holds_alternative<bool>(left)) {
                return std::get<bool>(left) != std::get<bool>(right);
            } else if (std::holds_alternative<std::string>(left)) {
                return std::get<std::string>(left) != std::get<std::string>(right);
            }

            return std::nullopt;
        }

        case BinOpType::And: {
            auto left_bool = ToBool(left);
            auto right_bool = ToBool(right);
            if (!left_bool.has_value() || !right_bool.has_value()) {
                return std::nullopt;
            }
            return left_bool.value() && right_bool.value();
        }

        case BinOpType::Or: {
            auto left_bool = ToBool(left);
            auto right_bool = ToBool(right);
            if (!left_bool.has_value() || !right_bool.has_value()) {
                return std::nullopt;
            }
            return left_bool.value() || right_bool.value();
        }

        default:
            return std::nullopt;
    }
}

std::optional<std::variant<int, double>> ConstantFolder::ToNumeric(const Constant::Value& value)
{
    if (std::holds_alternative<int>(value)) {
        return std::get<int>(value);
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    } else if (std::holds_alternative<bool>(value)) {
        // Convert bool to int (Python-like behavior)
        return std::get<bool>(value) ? 1 : 0;
    }
    return std::nullopt;
}

std::optional<bool> ConstantFolder::ToBool(const Constant::Value& value)
{
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    } else if (std::holds_alternative<int>(value)) {
        return std::get<int>(value) != 0;
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0.0;
    } else if (std::holds_alternative<std::string>(value)) {
        return !std::get<std::string>(value).empty();
    }
    return std::nullopt;
}

} // namespace epoch_stratifyx::epochflow
