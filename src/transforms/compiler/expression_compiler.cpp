//
// Created by Claude Code
// EpochFlow Expression Compiler Implementation
//

#include "expression_compiler.h"
#include "constructor_parser.h"
#include "option_validator.h"
#include "special_parameter_handler.h"
#include <epoch_core/enum_wrapper.h>
#include <algorithm>
#include <stdexcept>

namespace epochflow
{

    ValueHandle ExpressionCompiler::VisitExpr(const Expr& expr)
    {
        if (auto* call = dynamic_cast<const Call*>(&expr))
        {
            return VisitCall(*call);
        }
        else if (auto* attr = dynamic_cast<const Attribute*>(&expr))
        {
            return VisitAttribute(*attr);
        }
        else if (auto* name = dynamic_cast<const Name*>(&expr))
        {
            return VisitName(*name);
        }
        else if (auto* constant = dynamic_cast<const Constant*>(&expr))
        {
            return VisitConstant(*constant);
        }
        else if (auto* bin_op = dynamic_cast<const BinOp*>(&expr))
        {
            return VisitBinOp(*bin_op);
        }
        else if (auto* unary_op = dynamic_cast<const UnaryOp*>(&expr))
        {
            return VisitUnaryOp(*unary_op);
        }
        else if (auto* compare = dynamic_cast<const Compare*>(&expr))
        {
            return VisitCompare(*compare);
        }
        else if (auto* bool_op = dynamic_cast<const BoolOp*>(&expr))
        {
            return VisitBoolOp(*bool_op);
        }
        else if (auto* if_exp = dynamic_cast<const IfExp*>(&expr))
        {
            return VisitIfExp(*if_exp);
        }
        else if (auto* subscript = dynamic_cast<const Subscript*>(&expr))
        {
            return VisitSubscript(*subscript);
        }

        ThrowError("Unsupported expression type", expr.lineno, expr.col_offset);
        return {"", ""};
    }

    ValueHandle ExpressionCompiler::VisitCall(const Call& call)
    {
        // Phase 2: Handle inline constructor calls in expressions
        // Examples: gt(a, b), abs(value), ema(10)(src.c)

        // Parse as constructor call
        auto parse_result = constructor_parser_->ParseConstructorAndFeeds(call);

        // Validate component exists
        if (!context_.HasComponent(parse_result.ctor_name))
        {
            ThrowError("Unknown component '" + parse_result.ctor_name + "'", call.lineno, call.col_offset);
        }

        const auto& comp_meta = context_.GetComponentMetadata(parse_result.ctor_name);

        // Check if component has no outputs (is a sink/reporter)
        if (comp_meta.outputs.empty())
        {
            ThrowError("Direct call to component with outputs must be assigned to a variable", call.lineno, call.col_offset);
        }

        // Create synthetic node ID using component name (like: sma_0, ema_0, etc.)
        std::string synthetic_id = UniqueNodeId(parse_result.ctor_name);

        // Canonicalize special parameters
        auto params = parse_result.ctor_kwargs;
        special_param_handler_.CanonicalizeTimeframe(params);
        special_param_handler_.CanonicalizeSession(params);

        // Validate and apply option defaults/clamping
        option_validator_.ValidateAndApplyOptions(synthetic_id, comp_meta, params, call);

        // Create AlgorithmNode
        epochflow::strategy::AlgorithmNode algo;
        algo.id = synthetic_id;
        algo.type = parse_result.ctor_name;

        // Convert regular options (excluding timeframe and session)
        for (const auto& [key, value] : params)
        {
            if (key != "timeframe" && key != "session")
            {
                algo.options[key] = epochflow::MetaDataOptionDefinition{value};
            }
        }

        // Apply special fields (timeframe and session)
        special_param_handler_.ApplySpecialFields(algo, params);

        // Add to algorithms list
        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[synthetic_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[synthetic_id] = parse_result.ctor_name;

        // Track executor count
        if (parse_result.ctor_name == "trade_signal_executor")
        {
            context_.executor_count++;
        }

        // Wire inputs from feed steps
        for (const auto& [args, kwargs] : parse_result.feed_steps)
        {
            WireInputs(synthetic_id, parse_result.ctor_name, args, kwargs);
        }

        // Return the output handle
        // For single-output components, use the first (and only) output
        if (comp_meta.outputs.size() == 1)
        {
            std::string out_handle = comp_meta.outputs[0].id;
            return {synthetic_id, out_handle};
        }
        else
        {
            // Multi-output component used in expression context - error
            ThrowError("Component '" + parse_result.ctor_name + "' has " +
                       std::to_string(comp_meta.outputs.size()) +
                       " outputs; must be assigned to tuple");
        }
    }

    ValueHandle ExpressionCompiler::VisitAttribute(const Attribute& attr)
    {
        // Phase 2: Support attribute access on any expression, not just names
        // Examples: call().result, (ternary).result

        const Expr* base_expr = attr.value.get();

        // Check if base is a Name (simple case: var.handle)
        if (dynamic_cast<const Name*>(base_expr))
        {
            // Traditional attribute access: name.handle
            auto [var, handle] = AttributeToTuple(attr);
            return ResolveHandle(var, handle);
        }
        else
        {
            // Expression-based attribute access: expr.handle
            // Evaluate the base expression first
            ValueHandle base_handle = VisitExpr(*base_expr);

            // Now access the requested attribute (handle) on the result
            // The attr.attr field is the handle name we're accessing
            return {base_handle.node_id, attr.attr};
        }
    }

    ValueHandle ExpressionCompiler::VisitName(const Name& name)
    {
        // Look up variable in bindings
        auto it = context_.var_to_binding.find(name.id);
        if (it == context_.var_to_binding.end())
        {
            ThrowError("Unknown variable '" + name.id + "'", name.lineno, name.col_offset);
        }

        const std::string& ref = it->second;

        // Check if bound to a specific node.handle
        size_t dot_pos = ref.find('.');
        if (dot_pos != std::string::npos)
        {
            std::string node_id = ref.substr(0, dot_pos);
            std::string handle = ref.substr(dot_pos + 1);
            return {node_id, handle};
        }

        // Otherwise, ref is a component name - need to resolve single output
        const std::string& comp_name = ref;

        // Check if it's a synthetic literal node
        if (comp_name == "number" || comp_name == "bool_true" ||
            comp_name == "bool_false" || comp_name == "text" || comp_name == "null")
        {
            return {name.id, "result"};
        }

        // Look up component metadata
        if (!context_.HasComponent(comp_name))
        {
            ThrowError("Unknown component '" + comp_name + "'", name.lineno, name.col_offset);
        }

        const auto& comp_meta = context_.GetComponentMetadata(comp_name);
        const auto& outputs = comp_meta.outputs;

        if (outputs.empty())
        {
            ThrowError("Component '" + comp_name + "' has no outputs", name.lineno, name.col_offset);
        }

        // Must have exactly one output for unambiguous resolution
        if (outputs.size() != 1)
        {
            ThrowError("Ambiguous output for '" + name.id + "'", name.lineno, name.col_offset);
        }

        // Get the output handle
        std::string handle = outputs[0].id;

        return {name.id, handle};
    }

    ValueHandle ExpressionCompiler::VisitConstant(const Constant& constant)
    {
        // Materialize constants as nodes
        return std::visit([this, &constant](auto&& value) -> ValueHandle
        {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, int>) {
                return MaterializeNumber(static_cast<double>(value));
            } else if constexpr (std::is_same_v<T, double>) {
                return MaterializeNumber(value);
            } else if constexpr (std::is_same_v<T, bool>) {
                return MaterializeBoolean(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return MaterializeText(value);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                return MaterializeNull();
            } else {
                ThrowError("Unsupported constant type", constant.lineno, constant.col_offset);
                return {"", ""};
            }
        }, constant.value);
    }

    ValueHandle ExpressionCompiler::VisitBinOp(const BinOp& bin_op)
    {
        // Map operator type to component name
        std::string comp_name;
        switch (bin_op.op)
        {
        case BinOpType::Add:
            comp_name = "add";
            break;
        case BinOpType::Sub:
            comp_name = "sub";
            break;
        case BinOpType::Mult:
            comp_name = "mul";
            break;
        case BinOpType::Div:
            comp_name = "div";
            break;
        case BinOpType::Lt:
            comp_name = "lt";
            break;
        case BinOpType::Gt:
            comp_name = "gt";
            break;
        case BinOpType::LtE:
            comp_name = "lte";
            break;
        case BinOpType::GtE:
            comp_name = "gte";
            break;
        case BinOpType::Eq:
            comp_name = "eq";
            break;
        case BinOpType::NotEq:
            comp_name = "neq";
            break;
        case BinOpType::And:
            comp_name = "logical_and";
            break;
        case BinOpType::Or:
            comp_name = "logical_or";
            break;
        default:
            ThrowError("Unsupported binary operator", bin_op.lineno, bin_op.col_offset);
        }

        // Validate component exists
        if (!context_.HasComponent(comp_name))
        {
            ThrowError("Unknown operator component '" + comp_name + "'", bin_op.lineno, bin_op.col_offset);
        }

        const auto& comp_meta = context_.GetComponentMetadata(comp_name);

        // IMPORTANT: Create node ID and add placeholder to algorithms_ BEFORE recursing
        // This matches Python's behavior where parent nodes get lower IDs than children
        std::string node_id = UniqueNodeId(comp_name);
        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Add placeholder to algorithms_ to reserve the ID
        // Store the index since algorithms_ may reallocate during recursion
        size_t node_index = context_.algorithms.size();
        context_.algorithms.push_back(std::move(algo));

        // Now resolve left and right operands (may create child nodes with higher IDs)
        ValueHandle left = VisitExpr(*bin_op.left);
        ValueHandle right = VisitExpr(*bin_op.right);

        // Get input names and types from component metadata dynamically
        std::vector<std::string> input_names;
        std::unordered_map<std::string, DataType> input_types;
        const auto& inputs = comp_meta.inputs;

        for (const auto& input : inputs)
        {
            std::string input_id = input.id;

            // Handle SLOT naming convention (* -> SLOT, *0 -> SLOT0, etc.)
            if (!input_id.empty() && input_id[0] == '*')
            {
                std::string suffix = input_id.substr(1);
                input_id = suffix.empty() ? "SLOT" : "SLOT" + suffix;
            }

            input_names.push_back(input_id);

            // Map input type from IOMetaData to DataType enum
            std::string type_str = epoch_core::EnumWrapper<epoch_core::IODataType>::type::ToString(input.type);
            if (type_str == "Boolean")
                input_types[input_id] = DataType::Boolean;
            else if (type_str == "Integer")
                input_types[input_id] = DataType::Integer;
            else if (type_str == "Decimal")
                input_types[input_id] = DataType::Decimal;
            else if (type_str == "Number")
                input_types[input_id] = DataType::Number;
            else if (type_str == "String")
                input_types[input_id] = DataType::String;
            else
                input_types[input_id] = DataType::Any;
        }

        // Binary operators should have exactly 2 inputs
        if (input_names.size() != 2)
        {
            ThrowError("Binary operator '" + comp_name + "' must have exactly 2 inputs, got " +
                       std::to_string(input_names.size()),
                       bin_op.lineno, bin_op.col_offset);
        }

        // Extract first two input names dynamically (works for SLOT0/SLOT1, named inputs, etc.)
        std::string left_input_name = input_names[0];
        std::string right_input_name = input_names[1];

        // Type checking and casting for left operand
        DataType left_source_type = type_checker_.GetNodeOutputType(left.node_id, left.handle);
        DataType left_target_type = input_types[left_input_name];

        if (!type_checker_.IsTypeCompatible(left_source_type, left_target_type))
        {
            auto cast_result = type_checker_.NeedsTypeCast(left_source_type, left_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                left = type_checker_.InsertTypeCast(left, left_source_type, left_target_type);
            }
            else
            {
                ThrowError("Type mismatch for " + left_input_name + " of '" + node_id + "': expected " +
                           TypeChecker::DataTypeToString(left_target_type) + ", got " + TypeChecker::DataTypeToString(left_source_type),
                           bin_op.lineno, bin_op.col_offset);
            }
        }

        // Type checking and casting for right operand
        DataType right_source_type = type_checker_.GetNodeOutputType(right.node_id, right.handle);
        DataType right_target_type = input_types[right_input_name];

        if (!type_checker_.IsTypeCompatible(right_source_type, right_target_type))
        {
            auto cast_result = type_checker_.NeedsTypeCast(right_source_type, right_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                right = type_checker_.InsertTypeCast(right, right_source_type, right_target_type);
            }
            else
            {
                ThrowError("Type mismatch for " + right_input_name + " of '" + node_id + "': expected " +
                           TypeChecker::DataTypeToString(right_target_type) + ", got " + TypeChecker::DataTypeToString(right_source_type),
                           bin_op.lineno, bin_op.col_offset);
            }
        }

        // Wire inputs to the node we created earlier using dynamic input names
        context_.algorithms[node_index].inputs[left_input_name].push_back(JoinId(left.node_id, left.handle));
        context_.algorithms[node_index].inputs[right_input_name].push_back(JoinId(right.node_id, right.handle));

        // Update node_lookup_ AFTER recursion (index never invalidated)
        context_.node_lookup[node_id] = node_index;

        // Track output type for operators
        if (comp_name == "lt" || comp_name == "gt" || comp_name == "lte" ||
            comp_name == "gte" || comp_name == "eq" || comp_name == "neq" ||
            comp_name == "logical_and" || comp_name == "logical_or")
        {
            context_.node_output_types[node_id]["result"] = DataType::Boolean;
        }
        else if (comp_name == "add" || comp_name == "sub" ||
                 comp_name == "mul" || comp_name == "div")
        {
            context_.node_output_types[node_id]["result"] = DataType::Decimal;
        }

        // Get output handle from metadata
        std::string out_handle = "result";
        const auto& outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    ValueHandle ExpressionCompiler::VisitUnaryOp(const UnaryOp& unary_op)
    {
        // Handle unary plus (idempotent - just return the operand)
        if (unary_op.op == UnaryOpType::UAdd)
        {
            return VisitExpr(*unary_op.operand);
        }

        // Handle negation as multiplication by -1
        if (unary_op.op == UnaryOpType::USub)
        {
            // Create -1 number node
            ValueHandle minus_one = MaterializeNumber(-1.0);

            // Resolve operand
            ValueHandle operand = VisitExpr(*unary_op.operand);

            // Create mul AlgorithmNode
            std::string node_id = UniqueNodeId("mul");
            epochflow::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = "mul";

            // Wire inputs: (-1) * operand
            algo.inputs["SLOT0"].push_back(JoinId(minus_one.node_id, minus_one.handle));
            algo.inputs["SLOT1"].push_back(JoinId(operand.node_id, operand.handle));

            // Add to algorithms list
            context_.algorithms.push_back(std::move(algo));
            context_.node_lookup[node_id] = context_.algorithms.size() - 1;

            // Track output type
            context_.node_output_types[node_id]["result"] = DataType::Decimal;

            return {node_id, "result"};
        }

        // Handle logical not
        if (unary_op.op == UnaryOpType::Not)
        {
            std::string comp_name = "logical_not";

            // Validate component exists
            if (!context_.HasComponent(comp_name))
            {
                ThrowError("Unknown operator component '" + comp_name + "'", unary_op.lineno, unary_op.col_offset);
            }

            const auto& comp_meta = context_.GetComponentMetadata(comp_name);

            // Resolve operand FIRST (child-first/topological ordering required for timeframe resolution)
            ValueHandle operand = VisitExpr(*unary_op.operand);

            // Create node AFTER resolving operand
            std::string node_id = UniqueNodeId(comp_name);
            epochflow::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = comp_name;

            // Wire input (SLOT for unary operators)
            algo.inputs["SLOT"].push_back(JoinId(operand.node_id, operand.handle));

            // Add to algorithms list
            context_.algorithms.push_back(std::move(algo));
            context_.node_lookup[node_id] = context_.algorithms.size() - 1;

            // Track output type
            context_.node_output_types[node_id]["result"] = DataType::Boolean;

            // Get output handle
            std::string out_handle = "result";
            const auto& outputs = comp_meta.outputs;
            if (!outputs.empty())
            {
                out_handle = outputs[0].id;
            }

            return {node_id, out_handle};
        }

        ThrowError("Unsupported unary operator", unary_op.lineno, unary_op.col_offset);
        return {"", ""};
    }

    ValueHandle ExpressionCompiler::VisitCompare(const Compare& compare)
    {
        // Only single comparisons supported (a < b, not a < b < c)
        if (compare.ops.size() != 1 || compare.comparators.size() != 1)
        {
            ThrowError("Only single comparisons supported", compare.lineno, compare.col_offset);
        }

        // Map comparison operator to component name
        std::string comp_name;
        switch (compare.ops[0])
        {
        case BinOpType::Lt:
            comp_name = "lt";
            break;
        case BinOpType::Gt:
            comp_name = "gt";
            break;
        case BinOpType::LtE:
            comp_name = "lte";
            break;
        case BinOpType::GtE:
            comp_name = "gte";
            break;
        case BinOpType::Eq:
            comp_name = "eq";
            break;
        case BinOpType::NotEq:
            comp_name = "neq";
            break;
        default:
            ThrowError("Unsupported comparison operator", compare.lineno, compare.col_offset);
        }

        // Validate component exists
        if (!context_.HasComponent(comp_name))
        {
            ThrowError("Unknown operator component '" + comp_name + "'", compare.lineno, compare.col_offset);
        }

        const auto& comp_meta = context_.GetComponentMetadata(comp_name);

        // Resolve operands FIRST (child-first/topological ordering required for timeframe resolution)
        ValueHandle left = VisitExpr(*compare.left);
        ValueHandle right = VisitExpr(*compare.comparators[0]);

        // Create node AFTER resolving operands
        std::string node_id = UniqueNodeId(comp_name);
        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Get input names and types from component metadata dynamically
        std::vector<std::string> input_names;
        std::unordered_map<std::string, DataType> input_types;
        const auto& inputs = comp_meta.inputs;

        for (const auto& input : inputs)
        {
            std::string input_id = input.id;

            // Handle SLOT naming convention (* -> SLOT, *0 -> SLOT0, etc.)
            if (!input_id.empty() && input_id[0] == '*')
            {
                std::string suffix = input_id.substr(1);
                input_id = suffix.empty() ? "SLOT" : "SLOT" + suffix;
            }

            input_names.push_back(input_id);

            // Map input type from IOMetaData to DataType enum
            std::string type_str = epoch_core::EnumWrapper<epoch_core::IODataType>::type::ToString(input.type);
            if (type_str == "Boolean")
                input_types[input_id] = DataType::Boolean;
            else if (type_str == "Integer")
                input_types[input_id] = DataType::Integer;
            else if (type_str == "Decimal")
                input_types[input_id] = DataType::Decimal;
            else if (type_str == "Number")
                input_types[input_id] = DataType::Number;
            else if (type_str == "String")
                input_types[input_id] = DataType::String;
            else
                input_types[input_id] = DataType::Any;
        }

        // Comparison operators should have exactly 2 inputs
        if (input_names.size() != 2)
        {
            ThrowError("Comparison operator '" + comp_name + "' must have exactly 2 inputs, got " +
                       std::to_string(input_names.size()),
                       compare.lineno, compare.col_offset);
        }

        // Extract first two input names dynamically
        std::string left_input_name = input_names[0];
        std::string right_input_name = input_names[1];

        // Type checking and casting for left operand
        DataType left_source_type = type_checker_.GetNodeOutputType(left.node_id, left.handle);
        DataType left_target_type = input_types[left_input_name];

        if (!type_checker_.IsTypeCompatible(left_source_type, left_target_type))
        {
            auto cast_result = type_checker_.NeedsTypeCast(left_source_type, left_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                left = type_checker_.InsertTypeCast(left, left_source_type, left_target_type);
            }
            else
            {
                ThrowError("Type mismatch for " + left_input_name + " of '" + node_id + "': expected " +
                           TypeChecker::DataTypeToString(left_target_type) + ", got " + TypeChecker::DataTypeToString(left_source_type),
                           compare.lineno, compare.col_offset);
            }
        }

        // Type checking and casting for right operand
        DataType right_source_type = type_checker_.GetNodeOutputType(right.node_id, right.handle);
        DataType right_target_type = input_types[right_input_name];

        if (!type_checker_.IsTypeCompatible(right_source_type, right_target_type))
        {
            auto cast_result = type_checker_.NeedsTypeCast(right_source_type, right_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                right = type_checker_.InsertTypeCast(right, right_source_type, right_target_type);
            }
            else
            {
                ThrowError("Type mismatch for " + right_input_name + " of '" + node_id + "': expected " +
                           TypeChecker::DataTypeToString(right_target_type) + ", got " + TypeChecker::DataTypeToString(right_source_type),
                           compare.lineno, compare.col_offset);
            }
        }

        // Wire inputs
        algo.inputs[left_input_name].push_back(JoinId(left.node_id, left.handle));
        algo.inputs[right_input_name].push_back(JoinId(right.node_id, right.handle));

        // Add to algorithms list
        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;

        // Track output type (comparisons return Boolean)
        context_.node_output_types[node_id]["result"] = DataType::Boolean;

        // Get output handle
        std::string out_handle = "result";
        const auto& outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    ValueHandle ExpressionCompiler::VisitBoolOp(const BoolOp& bool_op)
    {
        // Boolean operations (and, or) with multiple operands
        // Convert to nested binary operations: (a and b and c) -> (a and (b and c))

        if (bool_op.values.size() < 2)
        {
            ThrowError("Boolean operation needs at least 2 operands", bool_op.lineno, bool_op.col_offset);
        }

        // Evaluate all operands
        std::vector<ValueHandle> handles;
        for (const auto& value : bool_op.values)
        {
            handles.push_back(VisitExpr(*value));
        }

        // Build nested structure: (a and b and c) -> logical_and_0(a, logical_and_1(b, c))
        std::string comp_name = (bool_op.op == BinOpType::And) ? "logical_and" : "logical_or";

        // Pre-create all logical_and/logical_or nodes needed (n-1 nodes for n operands)
        std::vector<size_t> node_indices;
        std::vector<std::string> node_ids;
        for (size_t i = 0; i < handles.size() - 1; ++i)
        {
            std::string node_id = UniqueNodeId(comp_name);
            epochflow::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = comp_name;

            size_t node_index = context_.algorithms.size();
            context_.algorithms.push_back(std::move(algo));

            node_indices.push_back(node_index);
            node_ids.push_back(node_id);
        }

        // Now wire them up: (a and b and c) -> logical_and_0(a, logical_and_1(b, c))
        if (handles.size() == 2)
        {
            // Simple case: just two operands
            context_.algorithms[node_indices[0]].inputs["SLOT0"].push_back(JoinId(handles[0].node_id, handles[0].handle));
            context_.algorithms[node_indices[0]].inputs["SLOT1"].push_back(JoinId(handles[1].node_id, handles[1].handle));
        }
        else
        {
            // Complex case: a and b and c ...
            context_.algorithms[node_indices[0]].inputs["SLOT0"].push_back(JoinId(handles[0].node_id, handles[0].handle));
            context_.algorithms[node_indices[0]].inputs["SLOT1"].push_back(JoinId(node_ids[1], "result"));

            // Middle nodes
            for (size_t i = 1; i < node_ids.size() - 1; ++i)
            {
                context_.algorithms[node_indices[i]].inputs["SLOT0"].push_back(JoinId(handles[i].node_id, handles[i].handle));
                context_.algorithms[node_indices[i]].inputs["SLOT1"].push_back(JoinId(node_ids[i + 1], "result"));
            }

            // Last node
            size_t last_idx = node_ids.size() - 1;
            context_.algorithms[node_indices[last_idx]].inputs["SLOT0"].push_back(JoinId(handles[last_idx].node_id, handles[last_idx].handle));
            context_.algorithms[node_indices[last_idx]].inputs["SLOT1"].push_back(JoinId(handles[last_idx + 1].node_id, handles[last_idx + 1].handle));
        }

        // Update node_lookup_ and track output types
        for (size_t i = 0; i < node_ids.size(); ++i)
        {
            context_.node_lookup[node_ids[i]] = node_indices[i];
            context_.node_output_types[node_ids[i]]["result"] = DataType::Boolean;
        }

        return {node_ids[0], "result"};
    }

    ValueHandle ExpressionCompiler::VisitIfExp(const IfExp& if_exp)
    {
        // Ternary expression: test ? body : orelse
        // Lower to boolean_select(condition, true, false)

        std::string comp_name = "boolean_select";

        // Validate component exists
        if (!context_.HasComponent(comp_name))
        {
            ThrowError("Unknown component '" + comp_name + "'", if_exp.lineno, if_exp.col_offset);
        }

        const auto& comp_meta = context_.GetComponentMetadata(comp_name);

        // Resolve inputs FIRST (child-first/topological ordering required for timeframe resolution)
        ValueHandle condition = VisitExpr(*if_exp.test);
        ValueHandle true_val = VisitExpr(*if_exp.body);
        ValueHandle false_val = VisitExpr(*if_exp.orelse);

        // Create node AFTER resolving inputs
        std::string node_id = UniqueNodeId("ifexp");
        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Wire inputs to named handles
        algo.inputs["condition"].push_back(JoinId(condition.node_id, condition.handle));
        algo.inputs["true"].push_back(JoinId(true_val.node_id, true_val.handle));
        algo.inputs["false"].push_back(JoinId(false_val.node_id, false_val.handle));

        // Add to algorithms list
        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;

        // Get output handle
        std::string out_handle = "result";
        const auto& outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    ValueHandle ExpressionCompiler::VisitSubscript(const Subscript& subscript)
    {
        // Subscript notation interpreted as lag operator
        // e.g., src.c[1] becomes lag(period=1)(src.c)

        // Extract lag period from slice
        int lag_period = 0;
        if (auto* constant = dynamic_cast<const Constant*>(subscript.slice.get()))
        {
            if (std::holds_alternative<int>(constant->value))
            {
                lag_period = std::get<int>(constant->value);
            }
            else
            {
                ThrowError("Subscript index must be an integer", subscript.lineno, subscript.col_offset);
            }
        }
        else if (auto* unary_op = dynamic_cast<const UnaryOp*>(subscript.slice.get()))
        {
            // Handle negative indices: -N is UnaryOp(USub, Constant(N))
            if (unary_op->op == UnaryOpType::USub)
            {
                if (auto* operand = dynamic_cast<const Constant*>(unary_op->operand.get()))
                {
                    if (std::holds_alternative<int>(operand->value))
                    {
                        lag_period = -std::get<int>(operand->value);
                    }
                    else
                    {
                        ThrowError("Subscript index must be an integer", subscript.lineno, subscript.col_offset);
                    }
                }
                else
                {
                    ThrowError("Subscript index must be a constant integer", subscript.lineno, subscript.col_offset);
                }
            }
            else
            {
                ThrowError("Unsupported unary operator in subscript", subscript.lineno, subscript.col_offset);
            }
        }
        else
        {
            ThrowError("Subscript index must be a constant integer", subscript.lineno, subscript.col_offset);
        }

        // Validate lag period is non-zero
        if (lag_period == 0)
        {
            ThrowError("Lag period must be a non-zero integer", subscript.lineno, subscript.col_offset);
        }

        // Resolve the value being lagged
        ValueHandle value = VisitExpr(*subscript.value);

        // Create AlgorithmNode for lag
        std::string node_id = UniqueNodeId("lag");
        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "lag";

        // Add period option
        algo.options["period"] = epochflow::MetaDataOptionDefinition{static_cast<double>(lag_period)};

        // Wire the value to the lag input
        algo.inputs["SLOT"].push_back(JoinId(value.node_id, value.handle));

        // Add to algorithms list
        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = "lag";

        // Track output type (lag always returns Decimal)
        context_.node_output_types[node_id]["result"] = DataType::Decimal;

        return {node_id, "result"};
    }

    // Materialize literal nodes

    ValueHandle ExpressionCompiler::MaterializeNumber(double value)
    {
        std::string node_id = UniqueNodeId("number");

        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "number";
        algo.options["value"] = epochflow::MetaDataOptionDefinition{value};

        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = "number";
        context_.node_output_types[node_id]["result"] = DataType::Decimal;

        return {node_id, "result"};
    }

    ValueHandle ExpressionCompiler::MaterializeBoolean(bool value)
    {
        std::string node_type = value ? "bool_true" : "bool_false";
        std::string node_id = UniqueNodeId(node_type);

        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = node_type;
        // No options needed for boolean nodes

        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = node_type;
        context_.node_output_types[node_id]["result"] = DataType::Boolean;

        return {node_id, "result"};
    }

    ValueHandle ExpressionCompiler::MaterializeText(const std::string& value)
    {
        std::string node_id = UniqueNodeId("text");

        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "text";
        algo.options["value"] = epochflow::MetaDataOptionDefinition{value};

        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = "text";
        context_.node_output_types[node_id]["result"] = DataType::String;

        return {node_id, "result"};
    }

    ValueHandle ExpressionCompiler::MaterializeNull()
    {
        std::string node_id = UniqueNodeId("null");

        epochflow::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "null";
        // No options needed for null node

        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = "null";
        context_.node_output_types[node_id]["result"] = DataType::Any;

        return {node_id, "result"};
    }

    // Private helper methods

    std::pair<std::string, std::string> ExpressionCompiler::AttributeToTuple(const Attribute& attr)
    {
        std::vector<std::string> parts;
        const Expr* current = &attr;

        // Walk backwards through the attribute chain
        while (auto* attr_node = dynamic_cast<const Attribute*>(current))
        {
            parts.push_back(attr_node->attr);
            current = attr_node->value.get();
        }

        // Base should be a Name
        if (auto* name_node = dynamic_cast<const Name*>(current))
        {
            parts.push_back(name_node->id);
        }
        else
        {
            ThrowError("Invalid attribute base - must be a name");
        }

        // Reverse to get correct order
        std::reverse(parts.begin(), parts.end());

        if (parts.size() < 2)
        {
            ThrowError("Attribute must have at least base.handle");
        }

        std::string var = parts[0];
        std::string handle = parts[1];
        for (size_t i = 2; i < parts.size(); ++i)
        {
            handle += "." + parts[i];
        }

        return {var, handle};
    }

    ValueHandle ExpressionCompiler::ResolveHandle(const std::string& var, const std::string& handle)
    {
        // Check if var is bound to a node.handle
        auto it = context_.var_to_binding.find(var);
        if (it != context_.var_to_binding.end())
        {
            const std::string& ref = it->second;
            if (ref.find('.') != std::string::npos)
            {
                ThrowError("Cannot access handle '" + handle + "' on '" + var +
                           "' which is already bound to '" + ref + "'");
            }
        }

        // Var should be a node name - look up its component type
        std::string comp_name;
        if (it != context_.var_to_binding.end())
        {
            comp_name = it->second;
        }
        else
        {
            // Check if it's a direct node reference (use node_lookup)
            if (context_.node_lookup.contains(var))
            {
                comp_name = context_.algorithms[context_.node_lookup[var]].type;
            }
            else
            {
                ThrowError("Unknown node '" + var + "'");
            }
        }

        // Validate component exists
        if (!context_.HasComponent(comp_name))
        {
            ThrowError("Unknown component '" + comp_name + "'");
        }

        const auto& comp_meta = context_.GetComponentMetadata(comp_name);

        // Extract valid handles from outputs and inputs
        std::set<std::string> valid_handles;

        const auto& outputs = comp_meta.outputs;
        for (const auto& output : outputs)
        {
            valid_handles.insert(output.id);
        }

        const auto& inputs = comp_meta.inputs;
        for (const auto& input : inputs)
        {
            std::string input_id = input.id;

            // Handle SLOT naming convention
            if (!input_id.empty() && input_id[0] == '*')
            {
                std::string suffix = input_id.substr(1);
                input_id = suffix.empty() ? "SLOT" : "SLOT" + suffix;
            }
            valid_handles.insert(input_id);
        }

        if (valid_handles.find(handle) == valid_handles.end())
        {
            ThrowError("Unknown handle '" + handle + "' on '" + var + "'");
        }

        return {var, handle};
    }

    std::string ExpressionCompiler::UniqueNodeId(const std::string& base)
    {
        // Use O(1) lookup instead of O(n) iteration
        int idx = 0;
        std::string candidate = base + "_" + std::to_string(idx);
        while (context_.used_node_ids.count(candidate))
        {
            ++idx;
            candidate = base + "_" + std::to_string(idx);
        }

        // Track this ID as used
        context_.used_node_ids.insert(candidate);
        return candidate;
    }

    std::string ExpressionCompiler::JoinId(const std::string& node_id, const std::string& handle)
    {
        return node_id + "#" + handle;
    }

    void ExpressionCompiler::WireInputs(
        const std::string& target_node_id,
        const std::string& component_name,
        const std::vector<ValueHandle>& args,
        const std::unordered_map<std::string, ValueHandle>& kwargs)
    {
        // Get component metadata
        if (!context_.HasComponent(component_name))
        {
            ThrowError("Unknown component '" + component_name + "'");
        }

        const auto& comp_meta = context_.GetComponentMetadata(component_name);

        // Extract input IDs and types from component metadata
        std::vector<std::string> input_ids;
        std::unordered_map<std::string, DataType> input_types;
        const auto& inputs = comp_meta.inputs;
        for (const auto& input : inputs)
        {
            std::string input_id = input.id;

            // Handle SLOT naming convention (* -> SLOT, *0 -> SLOT0, etc.)
            if (!input_id.empty() && input_id[0] == '*')
            {
                std::string suffix = input_id.substr(1);
                input_id = suffix.empty() ? "SLOT" : "SLOT" + suffix;
            }

            input_ids.push_back(input_id);

            // Map input type from IOMetaData to DataType enum
            std::string type_str = epoch_core::EnumWrapper<epoch_core::IODataType>::type::ToString(input.type);
            if (type_str == "Boolean")
                input_types[input_id] = DataType::Boolean;
            else if (type_str == "Integer")
                input_types[input_id] = DataType::Integer;
            else if (type_str == "Decimal")
                input_types[input_id] = DataType::Decimal;
            else if (type_str == "Number")
                input_types[input_id] = DataType::Number;
            else if (type_str == "String")
                input_types[input_id] = DataType::String;
            else
                input_types[input_id] = DataType::Any;
        }

        // Helper lambda to find the target node by ID
        auto find_target_node = [this](const std::string& node_id) -> epochflow::strategy::AlgorithmNode* {
            for (auto& algo : context_.algorithms)
            {
                if (algo.id == node_id)
                {
                    return &algo;
                }
            }
            return nullptr;
        };

        // Wire keyword arguments to inputs map
        for (const auto& [name, handle] : kwargs)
        {
            // Validate input name exists
            if (std::find(input_ids.begin(), input_ids.end(), name) == input_ids.end())
            {
                ThrowError("Unknown input handle '" + name + "' for '" + target_node_id + "'");
            }

            // Type checking: get source and target types
            DataType source_type = type_checker_.GetNodeOutputType(handle.node_id, handle.handle);
            DataType target_type = input_types[name];

            // Check if types are compatible
            if (!type_checker_.IsTypeCompatible(source_type, target_type))
            {
                // Try to insert type cast
                auto cast_result = type_checker_.NeedsTypeCast(source_type, target_type);
                if (cast_result.has_value() && cast_result.value() != "incompatible")
                {
                    // Insert cast node (this may reallocate algorithms_ vector)
                    ValueHandle casted = type_checker_.InsertTypeCast(handle, source_type, target_type);
                    // Find target node by ID after potential reallocation
                    auto* target_node = find_target_node(target_node_id);
                    if (target_node)
                    {
                        target_node->inputs[name].push_back(JoinId(casted.node_id, casted.handle));
                    }
                }
                else
                {
                    // Incompatible types - throw error
                    ThrowError("Type mismatch for input '" + name + "' of '" + target_node_id + "': expected " +
                               TypeChecker::DataTypeToString(target_type) + ", got " + TypeChecker::DataTypeToString(source_type));
                }
            }
            else
            {
                // Types are compatible - wire directly
                auto* target_node = find_target_node(target_node_id);
                if (target_node)
                {
                    target_node->inputs[name].push_back(JoinId(handle.node_id, handle.handle));
                }
            }
        }

        // Wire positional arguments to inputs map
        if (!args.empty())
        {
            if (input_ids.empty())
            {
                // Component with 0 inputs - ignore positional args (special case)
                return;
            }

            // Check if last input allows multiple connections (variadic inputs)
            bool last_input_allows_multi = false;
            if (!inputs.empty())
            {
                last_input_allows_multi = inputs.back().allowMultipleConnections;
            }

            // Validate positional args count (allow multiple args if last input is variadic)
            if (args.size() > input_ids.size() && !last_input_allows_multi)
            {
                ThrowError("Too many positional inputs for '" + target_node_id + "'");
            }

            for (size_t i = 0; i < args.size(); ++i)
            {
                const auto& handle = args[i];
                // For variadic inputs, all extra args go to the last input slot
                std::string dst_handle = (i < input_ids.size()) ? input_ids[i] : input_ids.back();

                // Type checking: get source and target types
                DataType source_type = type_checker_.GetNodeOutputType(handle.node_id, handle.handle);
                DataType target_type = input_types[dst_handle];

                // Check if types are compatible
                if (!type_checker_.IsTypeCompatible(source_type, target_type))
                {
                    // Try to insert type cast
                    auto cast_result = type_checker_.NeedsTypeCast(source_type, target_type);
                    if (cast_result.has_value() && cast_result.value() != "incompatible")
                    {
                        // Insert cast node (this may reallocate algorithms_ vector)
                        ValueHandle casted = type_checker_.InsertTypeCast(handle, source_type, target_type);
                        // Find target node by ID after potential reallocation
                        auto* target_node = find_target_node(target_node_id);
                        if (target_node)
                        {
                            target_node->inputs[dst_handle].push_back(JoinId(casted.node_id, casted.handle));
                        }
                    }
                    else
                    {
                        // Incompatible types - throw error
                        ThrowError("Type mismatch for positional input " + std::to_string(i) + " of '" + target_node_id + "': expected " +
                                   TypeChecker::DataTypeToString(target_type) + ", got " + TypeChecker::DataTypeToString(source_type));
                    }
                }
                else
                {
                    // Types are compatible - wire directly
                    auto* target_node = find_target_node(target_node_id);
                    if (target_node)
                    {
                        target_node->inputs[dst_handle].push_back(JoinId(handle.node_id, handle.handle));
                    }
                }
            }
        }
    }

    void ExpressionCompiler::ThrowError(const std::string& msg, int line, int col)
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

} // namespace epochflow
