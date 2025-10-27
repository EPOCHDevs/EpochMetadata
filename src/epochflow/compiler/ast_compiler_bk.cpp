//
// Created by Claude Code
// EpochFlow AST Compiler Implementation
//

#include "ast_compiler_bk.h"
#include "timeframe_resolver.h"
#include "../parser/python_parser.h"
#include <epoch_metadata/transforms/registry.h>
#include <stdexcept>
#include <iostream>

namespace epoch_stratifyx::epochflow
{


    CompilationResult AlgorithmAstCompiler::compile(const std::string &source)
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
        algorithms_.clear();
        executor_count_ = 0;
        node_lookup_.clear();
        var_to_binding_.clear();
        node_output_types_.clear();
        used_node_ids_.clear();

        // Reserve capacity to prevent reallocations (typical algorithm has 50-500 nodes)
        // Generous capacity to avoid any reallocations during compilation
        algorithms_.reserve(500);

        // Visit the module - builds algorithms in topological order (Python guarantees this)
        visitModule(*module);

        // Verify session dependencies and auto-create missing sessions nodes
        verifySessionDependencies();

        // Resolve timeframes for all nodes using TimeframeResolver utility
        resolveTimeframes(std::nullopt); // Pass nullopt as base timeframe for now

        // Return results - move semantics for zero-copy
        return std::move(algorithms_);
    }

    void AlgorithmAstCompiler::visitModule(const Module &module)
    {
        for (const auto &stmt : module.body)
        {
            visitStmt(*stmt);
        }
    }

    void AlgorithmAstCompiler::visitStmt(const Stmt &stmt)
    {
        if (auto *assign = dynamic_cast<const Assign *>(&stmt))
        {
            visitAssign(*assign);
        }
        else if (auto *exprStmt = dynamic_cast<const ExprStmt *>(&stmt))
        {
            visitExprStmt(*exprStmt);
        }
    }

    void AlgorithmAstCompiler::visitAssign(const Assign &assign)
    {
        // Validate: only single target supported
        if (assign.targets.size() != 1)
        {
            throwError("Only single assignment supported", assign.lineno, assign.col_offset);
        }

        const Expr *target = assign.targets[0].get();

        // Disallow attribute assignment (e.g., src.c = ...)
        if (dynamic_cast<const Attribute *>(target))
        {
            throwError("Assignment to attributes/handles is not allowed", assign.lineno, assign.col_offset);
        }

        // Check if value is a constructor call
        if (isConstructorCall(*assign.value))
        {
            handleConstructorAssignment(*target, *assign.value, assign);
        }
        else
        {
            // Handle non-constructor assignments (operators, references, etc.)
            handleNonConstructorAssignment(*target, *assign.value, assign);
        }
    }

    void AlgorithmAstCompiler::visitExprStmt(const ExprStmt &exprStmt)
    {
        // Allow direct calls to sink components (components with no outputs)
        const Expr *value = exprStmt.value.get();

        if (isConstructorCall(*value))
        {
            auto parseResult = parseConstructorAndFeeds(dynamic_cast<const Call &>(*value));

            // Validate component exists in singleton registry
            const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
            const auto &all_metadata = registry.GetMetaData();
            if (!all_metadata.contains(parseResult.ctor_name))
            {
                throwError("Unknown component '" + parseResult.ctor_name + "'", exprStmt.lineno, exprStmt.col_offset);
            }

            const auto &comp_meta = all_metadata.at(parseResult.ctor_name);

            // Check if component has no outputs (is a sink)
            if (comp_meta.outputs.empty())
            {
                // Create sink node with synthetic ID
                std::string synthetic_id = uniqueNodeId("node");

                // Canonicalize special parameters
                auto params = parseResult.ctor_kwargs;
                canonicalizeTimeframe(params);
                canonicalizeSession(params);

                // Validate and apply option defaults/clamping
                validateAndApplyOptions(synthetic_id, comp_meta, params, dynamic_cast<const Call &>(*value));

                // Create AlgorithmNode
                epoch_metadata::strategy::AlgorithmNode algo;
                algo.id = synthetic_id;
                algo.type = parseResult.ctor_name;

                // Convert regular options (excluding timeframe and session)
                for (const auto &[key, value] : params)
                {
                    if (key != "timeframe" && key != "session")
                    {
                        algo.options[key] = epoch_metadata::MetaDataOptionDefinition{value};
                    }
                }

                // Apply special fields (timeframe and session)
                applySpecialFields(algo, params);

                // Add to algorithms list
                algorithms_.push_back(std::move(algo));
                node_lookup_[synthetic_id] = algorithms_.size() - 1;

                // Track executor count
                if (parseResult.ctor_name == "trade_signal_executor")
                {
                    executor_count_++;
                }

                // Wire inputs from feed steps
                for (const auto &[args, kwargs] : parseResult.feed_steps)
                {
                    wireInputs(synthetic_id, parseResult.ctor_name, args, kwargs);
                }

                return;
            }
            else
            {
                throwError("Direct call to component with outputs must be assigned to a variable",
                           exprStmt.lineno, exprStmt.col_offset);
            }
        }

        throwError("Unsupported expression statement", exprStmt.lineno, exprStmt.col_offset);
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitExpr(const Expr &expr)
    {
        if (auto *call = dynamic_cast<const Call *>(&expr))
        {
            return visitCall(*call);
        }
        else if (auto *attr = dynamic_cast<const Attribute *>(&expr))
        {
            return visitAttribute(*attr);
        }
        else if (auto *name = dynamic_cast<const Name *>(&expr))
        {
            return visitName(*name);
        }
        else if (auto *constant = dynamic_cast<const Constant *>(&expr))
        {
            return visitConstant(*constant);
        }
        else if (auto *binOp = dynamic_cast<const BinOp *>(&expr))
        {
            return visitBinOp(*binOp);
        }
        else if (auto *unaryOp = dynamic_cast<const UnaryOp *>(&expr))
        {
            return visitUnaryOp(*unaryOp);
        }
        else if (auto *compare = dynamic_cast<const Compare *>(&expr))
        {
            return visitCompare(*compare);
        }
        else if (auto *boolOp = dynamic_cast<const BoolOp *>(&expr))
        {
            return visitBoolOp(*boolOp);
        }
        else if (auto *ifExp = dynamic_cast<const IfExp *>(&expr))
        {
            return visitIfExp(*ifExp);
        }
        else if (auto *subscript = dynamic_cast<const Subscript *>(&expr))
        {
            return visitSubscript(*subscript);
        }

        throwError("Unsupported expression type", expr.lineno, expr.col_offset);
        return {"", ""};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitCall(const Call &call)
    {
        // Phase 2: Handle inline constructor calls in expressions
        // Examples: gt(a, b), abs(value), ema(10)(src.c)

        // Parse as constructor call
        auto parseResult = parseConstructorAndFeeds(call);

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(parseResult.ctor_name))
        {
            throwError("Unknown component '" + parseResult.ctor_name + "'", call.lineno, call.col_offset);
        }

        const auto &comp_meta = all_metadata.at(parseResult.ctor_name);

        // Check if component has no outputs (is a sink/reporter)
        if (comp_meta.outputs.empty())
        {
            throwError("Direct call to component with outputs must be assigned to a variable", call.lineno, call.col_offset);
        }

        // Create synthetic node ID using component name (like: sma_0, ema_0, etc.)
        std::string synthetic_id = uniqueNodeId(parseResult.ctor_name);

        // Canonicalize special parameters
        auto params = parseResult.ctor_kwargs;
        canonicalizeTimeframe(params);
        canonicalizeSession(params);

        // Validate and apply option defaults/clamping
        validateAndApplyOptions(synthetic_id, comp_meta, params, call);

        // Create AlgorithmNode
        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = synthetic_id;
        algo.type = parseResult.ctor_name;

        // Convert regular options (excluding timeframe and session)
        for (const auto &[key, value] : params)
        {
            if (key != "timeframe" && key != "session")
            {
                algo.options[key] = epoch_metadata::MetaDataOptionDefinition{value};
            }
        }

        // Apply special fields (timeframe and session)
        applySpecialFields(algo, params);

        // Add to algorithms list
        algorithms_.push_back(std::move(algo));
        node_lookup_[synthetic_id] = algorithms_.size() - 1;
        var_to_binding_[synthetic_id] = parseResult.ctor_name;

        // Track executor count
        if (parseResult.ctor_name == "trade_signal_executor")
        {
            executor_count_++;
        }

        // Wire inputs from feed steps
        for (const auto &[args, kwargs] : parseResult.feed_steps)
        {
            wireInputs(synthetic_id, parseResult.ctor_name, args, kwargs);
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
            throwError("Component '" + parseResult.ctor_name + "' has " +
                       std::to_string(comp_meta.outputs.size()) +
                       " outputs; must be assigned to tuple");
        }
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitAttribute(const Attribute &attr)
    {
        // Phase 2: Support attribute access on any expression, not just names
        // Examples: call().result, (ternary).result

        const Expr *base_expr = attr.value.get();

        // Check if base is a Name (simple case: var.handle)
        if (dynamic_cast<const Name *>(base_expr))
        {
            // Traditional attribute access: name.handle
            auto [var, handle] = attributeToTuple(attr);
            return resolveHandle(var, handle);
        }
        else
        {
            // Expression-based attribute access: expr.handle
            // Evaluate the base expression first
            ValueHandle base_handle = visitExpr(*base_expr);

            // Now access the requested attribute (handle) on the result
            // The attr.attr field is the handle name we're accessing
            return {base_handle.node_id, attr.attr};
        }
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitName(const Name &name)
    {
        // Look up variable in bindings
        auto it = var_to_binding_.find(name.id);
        if (it == var_to_binding_.end())
        {
            throwError("Unknown variable '" + name.id + "'", name.lineno, name.col_offset);
        }

        const std::string &ref = it->second;

        // Check if bound to a specific node.handle
        size_t dotPos = ref.find('.');
        if (dotPos != std::string::npos)
        {
            std::string node_id = ref.substr(0, dotPos);
            std::string handle = ref.substr(dotPos + 1);
            return {node_id, handle};
        }

        // Otherwise, ref is a component name - need to resolve single output
        const std::string &comp_name = ref;

        // Check if it's a synthetic literal node
        if (comp_name == "number" || comp_name == "bool_true" ||
            comp_name == "bool_false" || comp_name == "text" || comp_name == "null")
        {
            return {name.id, "result"};
        }

        // Look up component metadata from singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();

        if (!all_metadata.contains(comp_name))
        {
            throwError("Unknown component '" + comp_name + "'", name.lineno, name.col_offset);
        }

        const auto &comp_meta = all_metadata.at(comp_name);
        const auto &outputs = comp_meta.outputs;

        if (outputs.empty())
        {
            throwError("Component '" + comp_name + "' has no outputs", name.lineno, name.col_offset);
        }

        // Must have exactly one output for unambiguous resolution
        if (outputs.size() != 1)
        {
            throwError("Ambiguous output for '" + name.id + "'", name.lineno, name.col_offset);
        }

        // Get the output handle
        std::string handle = outputs[0].id;

        return {name.id, handle};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitConstant(const Constant &constant)
    {
        // Materialize constants as nodes
        return std::visit([this, &constant](auto &&value) -> ValueHandle
                          {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, int>) {
            return materializeNumber(static_cast<double>(value));
        } else if constexpr (std::is_same_v<T, double>) {
            return materializeNumber(value);
        } else if constexpr (std::is_same_v<T, bool>) {
            return materializeBoolean(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return materializeText(value);
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return materializeNull();
        } else {
            throwError("Unsupported constant type", constant.lineno, constant.col_offset);
            return {"", ""};
        } }, constant.value);
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitBinOp(const BinOp &binOp)
    {
        // Map operator type to component name
        std::string comp_name;
        switch (binOp.op)
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
            throwError("Unsupported binary operator", binOp.lineno, binOp.col_offset);
        }

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(comp_name))
        {
            throwError("Unknown operator component '" + comp_name + "'", binOp.lineno, binOp.col_offset);
        }

        const auto &comp_meta = all_metadata.at(comp_name);

        // IMPORTANT: Create node ID and add placeholder to algorithms_ BEFORE recursing
        // This matches Python's behavior where parent nodes get lower IDs than children
        std::string node_id = uniqueNodeId(comp_name);
        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Add placeholder to algorithms_ to reserve the ID
        // Store the index since algorithms_ may reallocate during recursion
        size_t node_index = algorithms_.size();
        algorithms_.push_back(std::move(algo));

        // Now resolve left and right operands (may create child nodes with higher IDs)
        ValueHandle left = visitExpr(*binOp.left);
        ValueHandle right = visitExpr(*binOp.right);

        // Get input names and types from component metadata dynamically
        std::vector<std::string> input_names;
        std::unordered_map<std::string, DataType> input_types;
        const auto &inputs = comp_meta.inputs;

        for (const auto &input : inputs)
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
            throwError("Binary operator '" + comp_name + "' must have exactly 2 inputs, got " +
                       std::to_string(input_names.size()),
                       binOp.lineno, binOp.col_offset);
        }

        // Extract first two input names dynamically (works for SLOT0/SLOT1, named inputs, etc.)
        std::string left_input_name = input_names[0];
        std::string right_input_name = input_names[1];

        // Type checking and casting for left operand
        DataType left_source_type = getNodeOutputType(left.node_id, left.handle);
        DataType left_target_type = input_types[left_input_name];

        if (!isTypeCompatible(left_source_type, left_target_type))
        {
            auto cast_result = needsTypeCast(left_source_type, left_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                left = insertTypeCast(left, left_source_type, left_target_type);
            }
            else
            {
                throwError("Type mismatch for " + left_input_name + " of '" + node_id + "': expected " +
                           dataTypeToString(left_target_type) + ", got " + dataTypeToString(left_source_type),
                           binOp.lineno, binOp.col_offset);
            }
        }

        // Type checking and casting for right operand
        DataType right_source_type = getNodeOutputType(right.node_id, right.handle);
        DataType right_target_type = input_types[right_input_name];

        if (!isTypeCompatible(right_source_type, right_target_type))
        {
            auto cast_result = needsTypeCast(right_source_type, right_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                right = insertTypeCast(right, right_source_type, right_target_type);
            }
            else
            {
                throwError("Type mismatch for " + right_input_name + " of '" + node_id + "': expected " +
                           dataTypeToString(right_target_type) + ", got " + dataTypeToString(right_source_type),
                           binOp.lineno, binOp.col_offset);
            }
        }

        // Wire inputs to the node we created earlier using dynamic input names
        algorithms_[node_index].inputs[left_input_name].push_back(joinId(left.node_id, left.handle));
        algorithms_[node_index].inputs[right_input_name].push_back(joinId(right.node_id, right.handle));

        // Update node_lookup_ AFTER recursion (index never invalidated)
        node_lookup_[node_id] = node_index;

        // Track output type for operators
        if (comp_name == "lt" || comp_name == "gt" || comp_name == "lte" ||
            comp_name == "gte" || comp_name == "eq" || comp_name == "neq" ||
            comp_name == "logical_and" || comp_name == "logical_or")
        {
            node_output_types_[node_id]["result"] = DataType::Boolean;
        }
        else if (comp_name == "add" || comp_name == "sub" ||
                 comp_name == "mul" || comp_name == "div")
        {
            node_output_types_[node_id]["result"] = DataType::Decimal;
        }

        // Get output handle from metadata
        std::string out_handle = "result";
        const auto &outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitUnaryOp(const UnaryOp &unaryOp)
    {
        // Handle unary plus (idempotent - just return the operand)
        if (unaryOp.op == UnaryOpType::UAdd)
        {
            return visitExpr(*unaryOp.operand);
        }

        // Handle negation as multiplication by -1
        if (unaryOp.op == UnaryOpType::USub)
        {
            // Create -1 number node
            ValueHandle minus_one = materializeNumber(-1.0);

            // Resolve operand
            ValueHandle operand = visitExpr(*unaryOp.operand);

            // Create mul AlgorithmNode
            std::string node_id = uniqueNodeId("mul");
            epoch_metadata::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = "mul";

            // Wire inputs: (-1) * operand
            algo.inputs["SLOT0"].push_back(joinId(minus_one.node_id, minus_one.handle));
            algo.inputs["SLOT1"].push_back(joinId(operand.node_id, operand.handle));

            // Add to algorithms list
            algorithms_.push_back(std::move(algo));
            node_lookup_[node_id] = algorithms_.size() - 1;

            // Track output type
            node_output_types_[node_id]["result"] = DataType::Decimal;

            return {node_id, "result"};
        }

        // Handle logical not
        if (unaryOp.op == UnaryOpType::Not)
        {
            std::string comp_name = "logical_not";

            // Validate component exists in singleton registry
            const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
            const auto &all_metadata = registry.GetMetaData();
            if (!all_metadata.contains(comp_name))
            {
                throwError("Unknown operator component '" + comp_name + "'", unaryOp.lineno, unaryOp.col_offset);
            }

            const auto &comp_meta = all_metadata.at(comp_name);

            // Resolve operand
            ValueHandle operand = visitExpr(*unaryOp.operand);

            // Create AlgorithmNode
            std::string node_id = uniqueNodeId(comp_name);
            epoch_metadata::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = comp_name;

            // Wire input (SLOT for unary operators)
            algo.inputs["SLOT"].push_back(joinId(operand.node_id, operand.handle));

            // Add to algorithms list
            algorithms_.push_back(std::move(algo));
            node_lookup_[node_id] = algorithms_.size() - 1;

            // Track output type
            node_output_types_[node_id]["result"] = DataType::Boolean;

            // Get output handle
            std::string out_handle = "result";
            const auto &outputs = comp_meta.outputs;
            if (!outputs.empty())
            {
                out_handle = outputs[0].id;
            }

            return {node_id, out_handle};
        }

        throwError("Unsupported unary operator", unaryOp.lineno, unaryOp.col_offset);
        return {"", ""};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitCompare(const Compare &compare)
    {
        // Only single comparisons supported (a < b, not a < b < c)
        if (compare.ops.size() != 1 || compare.comparators.size() != 1)
        {
            throwError("Only single comparisons supported", compare.lineno, compare.col_offset);
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
            throwError("Unsupported comparison operator", compare.lineno, compare.col_offset);
        }

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(comp_name))
        {
            throwError("Unknown operator component '" + comp_name + "'", compare.lineno, compare.col_offset);
        }

        const auto &comp_meta = all_metadata.at(comp_name);

        // IMPORTANT: Create node ID and add placeholder BEFORE recursing
        std::string node_id = uniqueNodeId(comp_name);
        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Add placeholder to reserve the ID
        // Store the index since algorithms_ may reallocate during recursion
        size_t node_index = algorithms_.size();
        algorithms_.push_back(std::move(algo));

        // Now resolve left and right operands
        ValueHandle left = visitExpr(*compare.left);
        ValueHandle right = visitExpr(*compare.comparators[0]);

        // Get input names and types from component metadata dynamically
        std::vector<std::string> input_names;
        std::unordered_map<std::string, DataType> input_types;
        const auto &inputs = comp_meta.inputs;

        for (const auto &input : inputs)
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
            throwError("Comparison operator '" + comp_name + "' must have exactly 2 inputs, got " +
                       std::to_string(input_names.size()),
                       compare.lineno, compare.col_offset);
        }

        // Extract first two input names dynamically (works for SLOT0/SLOT1, named inputs, etc.)
        std::string left_input_name = input_names[0];
        std::string right_input_name = input_names[1];

        // Type checking and casting for left operand
        DataType left_source_type = getNodeOutputType(left.node_id, left.handle);
        DataType left_target_type = input_types[left_input_name];

        if (!isTypeCompatible(left_source_type, left_target_type))
        {
            auto cast_result = needsTypeCast(left_source_type, left_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                left = insertTypeCast(left, left_source_type, left_target_type);
            }
            else
            {
                throwError("Type mismatch for " + left_input_name + " of '" + node_id + "': expected " +
                           dataTypeToString(left_target_type) + ", got " + dataTypeToString(left_source_type),
                           compare.lineno, compare.col_offset);
            }
        }

        // Type checking and casting for right operand
        DataType right_source_type = getNodeOutputType(right.node_id, right.handle);
        DataType right_target_type = input_types[right_input_name];

        if (!isTypeCompatible(right_source_type, right_target_type))
        {
            auto cast_result = needsTypeCast(right_source_type, right_target_type);
            if (cast_result.has_value() && cast_result.value() != "incompatible")
            {
                right = insertTypeCast(right, right_source_type, right_target_type);
            }
            else
            {
                throwError("Type mismatch for " + right_input_name + " of '" + node_id + "': expected " +
                           dataTypeToString(right_target_type) + ", got " + dataTypeToString(right_source_type),
                           compare.lineno, compare.col_offset);
            }
        }

        // Wire inputs to the node we created earlier using dynamic input names
        algorithms_[node_index].inputs[left_input_name].push_back(joinId(left.node_id, left.handle));
        algorithms_[node_index].inputs[right_input_name].push_back(joinId(right.node_id, right.handle));

        // Update node_lookup_ AFTER recursion (index never invalidated)
        node_lookup_[node_id] = node_index;

        // Track output type (comparisons return Boolean)
        node_output_types_[node_id]["result"] = DataType::Boolean;

        // Get output handle
        std::string out_handle = "result";
        const auto &outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitBoolOp(const BoolOp &boolOp)
    {
        // Boolean operations (and, or) with multiple operands
        // Convert to nested binary operations: (a and b and c) -> (a and (b and c))

        if (boolOp.values.size() < 2)
        {
            throwError("Boolean operation needs at least 2 operands", boolOp.lineno, boolOp.col_offset);
        }

        // Evaluate all operands
        std::vector<ValueHandle> handles;
        for (const auto &value : boolOp.values)
        {
            handles.push_back(visitExpr(*value));
        }

        // Build nested structure: (a and b and c) -> logical_and_0(a, logical_and_1(b, c))
        // IMPORTANT: Create placeholder nodes in parent-first order to match Python's node ID assignment
        std::string comp_name = (boolOp.op == BinOpType::And) ? "logical_and" : "logical_or";

        // Pre-create all logical_and/logical_or nodes needed (n-1 nodes for n operands)
        std::vector<size_t> node_indices;
        std::vector<std::string> node_ids;
        for (size_t i = 0; i < handles.size() - 1; ++i)
        {
            std::string node_id = uniqueNodeId(comp_name);
            epoch_metadata::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = comp_name;

            size_t node_index = algorithms_.size();
            algorithms_.push_back(std::move(algo));

            node_indices.push_back(node_index);
            node_ids.push_back(node_id);
        }

        // Now wire them up: (a and b and c) -> logical_and_0(a, logical_and_1(b, c))
        // First node: SLOT0=handles[0], SLOT1=node_ids[1] (if exists) OR handles[1]
        if (handles.size() == 2)
        {
            // Simple case: just two operands
            algorithms_[node_indices[0]].inputs["SLOT0"].push_back(joinId(handles[0].node_id, handles[0].handle));
            algorithms_[node_indices[0]].inputs["SLOT1"].push_back(joinId(handles[1].node_id, handles[1].handle));
        }
        else
        {
            // Complex case: a and b and c ...
            // logical_and_0: (handles[0], logical_and_1)
            algorithms_[node_indices[0]].inputs["SLOT0"].push_back(joinId(handles[0].node_id, handles[0].handle));
            algorithms_[node_indices[0]].inputs["SLOT1"].push_back(joinId(node_ids[1], "result"));

            // Middle nodes: logical_and_i: (handles[i], logical_and_{i+1})
            for (size_t i = 1; i < node_ids.size() - 1; ++i)
            {
                algorithms_[node_indices[i]].inputs["SLOT0"].push_back(joinId(handles[i].node_id, handles[i].handle));
                algorithms_[node_indices[i]].inputs["SLOT1"].push_back(joinId(node_ids[i + 1], "result"));
            }

            // Last node: logical_and_{n-2}: (handles[n-2], handles[n-1])
            size_t last_idx = node_ids.size() - 1;
            algorithms_[node_indices[last_idx]].inputs["SLOT0"].push_back(joinId(handles[last_idx].node_id, handles[last_idx].handle));
            algorithms_[node_indices[last_idx]].inputs["SLOT1"].push_back(joinId(handles[last_idx + 1].node_id, handles[last_idx + 1].handle));
        }

        // Update node_lookup_ and track output types
        for (size_t i = 0; i < node_ids.size(); ++i)
        {
            node_lookup_[node_ids[i]] = node_indices[i];
            node_output_types_[node_ids[i]]["result"] = DataType::Boolean;
        }

        return {node_ids[0], "result"};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitIfExp(const IfExp &ifExp)
    {
        // Ternary expression: test ? body : orelse
        // Lower to boolean_select(condition, true, false)

        std::string comp_name = "boolean_select";

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(comp_name))
        {
            throwError("Unknown component '" + comp_name + "'", ifExp.lineno, ifExp.col_offset);
        }

        const auto &comp_meta = all_metadata.at(comp_name);

        // Resolve inputs
        ValueHandle condition = visitExpr(*ifExp.test);
        ValueHandle true_val = visitExpr(*ifExp.body);
        ValueHandle false_val = visitExpr(*ifExp.orelse);

        // Create AlgorithmNode
        std::string node_id = uniqueNodeId("ifexp");
        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = comp_name;

        // Wire inputs to named handles
        algo.inputs["condition"].push_back(joinId(condition.node_id, condition.handle));
        algo.inputs["true"].push_back(joinId(true_val.node_id, true_val.handle));
        algo.inputs["false"].push_back(joinId(false_val.node_id, false_val.handle));

        // Add to algorithms list
        algorithms_.push_back(std::move(algo));
        node_lookup_[node_id] = algorithms_.size() - 1;

        // Get output handle
        std::string out_handle = "result";
        const auto &outputs = comp_meta.outputs;
        if (!outputs.empty())
        {
            out_handle = outputs[0].id;
        }

        return {node_id, out_handle};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::visitSubscript(const Subscript &subscript)
    {
        // Subscript notation interpreted as lag operator
        // e.g., src.c[1] becomes lag(period=1)(src.c)

        // Extract lag period from slice
        int lag_period = 0;
        if (auto *constant = dynamic_cast<const Constant *>(subscript.slice.get()))
        {
            if (std::holds_alternative<int>(constant->value))
            {
                lag_period = std::get<int>(constant->value);
            }
            else
            {
                throwError("Subscript index must be an integer", subscript.lineno, subscript.col_offset);
            }
        }
        else if (auto *unaryOp = dynamic_cast<const UnaryOp *>(subscript.slice.get()))
        {
            // Handle negative indices: -N is UnaryOp(USub, Constant(N))
            if (unaryOp->op == UnaryOpType::USub)
            {
                if (auto *operand = dynamic_cast<const Constant *>(unaryOp->operand.get()))
                {
                    if (std::holds_alternative<int>(operand->value))
                    {
                        lag_period = -std::get<int>(operand->value);
                    }
                    else
                    {
                        throwError("Subscript index must be an integer", subscript.lineno, subscript.col_offset);
                    }
                }
                else
                {
                    throwError("Subscript index must be a constant integer", subscript.lineno, subscript.col_offset);
                }
            }
            else
            {
                throwError("Unsupported unary operator in subscript", subscript.lineno, subscript.col_offset);
            }
        }
        else
        {
            throwError("Subscript index must be a constant integer", subscript.lineno, subscript.col_offset);
        }

        // Validate lag period is non-zero
        if (lag_period == 0)
        {
            throwError("Lag period must be a non-zero integer", subscript.lineno, subscript.col_offset);
        }

        // Resolve the value being lagged
        ValueHandle value = visitExpr(*subscript.value);

        // Create AlgorithmNode for lag
        std::string node_id = uniqueNodeId("lag");
        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "lag";

        // Add period option
        algo.options["period"] = epoch_metadata::MetaDataOptionDefinition{static_cast<double>(lag_period)};

        // Wire the value to the lag input
        algo.inputs["SLOT"].push_back(joinId(value.node_id, value.handle));

        // Add to algorithms list
        algorithms_.push_back(std::move(algo));
        node_lookup_[node_id] = algorithms_.size() - 1;
        var_to_binding_[node_id] = "lag";

        // Track output type (lag always returns Decimal)
        node_output_types_[node_id]["result"] = DataType::Decimal;

        return {node_id, "result"};
    }

    std::string AlgorithmAstCompiler::uniqueNodeId(const std::string &base)
    {
        // Use O(1) lookup instead of O(n) iteration
        int idx = 0;
        std::string candidate = base + "_" + std::to_string(idx);
        while (used_node_ids_.count(candidate))
        {
            ++idx;
            candidate = base + "_" + std::to_string(idx);
        }

        // Track this ID as used
        used_node_ids_.insert(candidate);
        return candidate;
    }


    std::string AlgorithmAstCompiler::joinId(const std::string &node_id, const std::string &handle)
    {
        return node_id + "#" + handle;
    }

    void AlgorithmAstCompiler::validateComponent(const std::string &componentName)
    {
        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(componentName))
        {
            throwError("Unknown component '" + componentName + "'");
        }
    }

    void AlgorithmAstCompiler::validateTimeframe(const std::string &timeframe)
    {
        // Validate timeframe is a non-empty string (pandas offset)
        // Without pandas, we accept any non-empty string (similar to Python fallback)
        if (timeframe.empty())
        {
            throwError("Parameter 'timeframe' must be a non-empty string (pandas offset)");
        }
    }

    void AlgorithmAstCompiler::validateSession(const std::string &session)
    {
        // Validate session against predefined session types
        static const std::unordered_set<std::string> valid_sessions = {
            "Sydney", "Tokyo", "London", "NewYork",
            "AsianKillZone", "LondonOpenKillZone", "NewYorkKillZone", "LondonCloseKillZone"
        };

        if (session.empty())
        {
            throwError("Parameter 'session' must be a non-empty string");
        }

        if (valid_sessions.find(session) == valid_sessions.end())
        {
            throwError("Invalid session '" + session + "'. Must be one of: " +
                       "AsianKillZone, London, LondonCloseKillZone, LondonOpenKillZone, " +
                       "NewYork, NewYorkKillZone, Sydney, Tokyo");
        }
    }

    // Helper function to trim leading/trailing whitespace from a string
    static std::string trimWhitespace(const std::string &str)
    {
        const char *whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    epoch_metadata::MetaDataOptionDefinition::T AlgorithmAstCompiler::parseOptionByMetadata(
        const epoch_metadata::MetaDataOptionDefinition::T &rawValue,
        const epoch_metadata::MetaDataOption &metaOption,
        const std::string &optionId,
        const std::string &nodeId,
        const Call &call,
        const epoch_metadata::transforms::TransformsMetaData &comp_meta)
    {
        using MetaType = epoch_core::MetaDataOptionType;

        // Handle each metadata type
        switch (metaOption.type)
        {
        case MetaType::Integer:
        case MetaType::Decimal:
        {
            // Expect a numeric value
            if (!std::holds_alternative<double>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects type {} but got non-numeric value",
                                optionId, nodeId,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(metaOption.type)),
                    call.lineno, call.col_offset);
            }

            double numericValue = std::get<double>(rawValue);

            // Clamp to min/max bounds
            double clampedValue = std::max(metaOption.min, std::min(metaOption.max, numericValue));

            return clampedValue;
        }

        case MetaType::Boolean:
        {
            // Expect a boolean value
            if (!std::holds_alternative<bool>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects Boolean but got non-boolean value",
                                optionId, nodeId),
                    call.lineno, call.col_offset);
            }
            return rawValue;
        }

        case MetaType::String:
        case MetaType::Select:
        {
            // Expect a string value
            if (!std::holds_alternative<std::string>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects String but got non-string value",
                                optionId, nodeId),
                    call.lineno, call.col_offset);
            }

            // For Select type, validate against allowed options
            if (metaOption.type == MetaType::Select && !metaOption.selectOption.empty())
            {
                const std::string &strValue = std::get<std::string>(rawValue);
                bool isValid = false;
                for (const auto &option : metaOption.selectOption)
                {
                    if (option.value == strValue)
                    {
                        isValid = true;
                        break;
                    }
                }

                if (!isValid)
                {
                    std::string validOptions;
                    for (size_t i = 0; i < metaOption.selectOption.size(); ++i)
                    {
                        validOptions += metaOption.selectOption[i].value;
                        if (i < metaOption.selectOption.size() - 1)
                            validOptions += ", ";
                    }

                    throwError(
                        std::format("Option '{}' of node '{}' has invalid value '{}'. Valid options: {}",
                                    optionId, nodeId, strValue, validOptions),
                        call.lineno, call.col_offset);
                }
            }

            return rawValue;
        }

        case MetaType::CardSchema:
        {
            // If already parsed as CardSchemaFilter or CardSchemaSQL, return as-is (already validated)
            if (std::holds_alternative<epoch_metadata::CardSchemaFilter>(rawValue) ||
                std::holds_alternative<epoch_metadata::CardSchemaSQL>(rawValue))
            {
                return rawValue;
            }

            // Expect a JSON string to parse into CardSchemaFilter or CardSchemaSQL
            if (!std::holds_alternative<std::string>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects CardSchema (JSON string) but got non-string value",
                                optionId, nodeId),
                    call.lineno, call.col_offset);
            }

            const std::string &json_str = std::get<std::string>(rawValue);
            // Trim leading/trailing whitespace (triple-quoted strings may have newlines)
            std::string trimmed_json = trimWhitespace(json_str);

            // Try parsing as CardSchemaFilter first (uses select_key)
            auto filter_result = glz::read_json<epoch_metadata::CardSchemaFilter>(trimmed_json);
            if (filter_result)
            {
                return epoch_metadata::MetaDataOptionDefinition::T{filter_result.value()};
            }

            // Try parsing as CardSchemaSQL (uses sql)
            auto sql_result = glz::read_json<epoch_metadata::CardSchemaSQL>(trimmed_json);
            if (sql_result)
            {
                auto cardSchemaSQL = sql_result.value();

                // Validate SqlStatement with correct numOutputs
                int numOutputs = static_cast<int>(comp_meta.outputs.size());
                try
                {
                    cardSchemaSQL.sql.Validate(numOutputs);
                }
                catch (const std::exception &e)
                {
                    throwError(
                        std::format("Invalid SQL in CardSchema for option '{}' of node '{}': {}",
                                    optionId, nodeId, e.what()),
                        call.lineno, call.col_offset);
                }

                return epoch_metadata::MetaDataOptionDefinition::T{cardSchemaSQL};
            }

            // Both CardSchemaFilter and CardSchemaSQL failed
            throwError(
                std::format("Invalid CardSchema JSON for option '{}' of node '{}'. "
                            "CardSchema must contain either 'select_key' (for filter mode) or 'sql' (for SQL mode).",
                            optionId, nodeId),
                call.lineno, call.col_offset);
        }

        case MetaType::SqlStatement:
        {
            // If already parsed as SqlStatement, return as-is (already validated)
            if (std::holds_alternative<epoch_metadata::SqlStatement>(rawValue))
            {
                return rawValue;
            }

            // Expect a string (SQL query) that will be validated by SqlStatement constructor
            if (!std::holds_alternative<std::string>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects SqlStatement (string) but got non-string value",
                                optionId, nodeId),
                    call.lineno, call.col_offset);
            }

            const std::string &sql_str = std::get<std::string>(rawValue);

            try
            {
                // Construct SqlStatement and validate with numOutputs
                epoch_metadata::SqlStatement sqlStmt{sql_str};
                int numOutputs = static_cast<int>(comp_meta.outputs.size());
                sqlStmt.Validate(numOutputs);
                return epoch_metadata::MetaDataOptionDefinition::T{sqlStmt};
            }
            catch (const std::exception &e)
            {
                throwError(
                    std::format("Option '{}' of node '{}': {}", optionId, nodeId, e.what()),
                    call.lineno, call.col_offset);
            }
        }

        case MetaType::Time:
        {
            // Expect a string that can be parsed into Time
            if (std::holds_alternative<std::string>(rawValue))
            {
                // Parse string into Time object
                const std::string &timeStr = std::get<std::string>(rawValue);
                try
                {
                    epoch_frame::Time time = epoch_metadata::TimeFromString(timeStr);
                    return epoch_metadata::MetaDataOptionDefinition::T{time};
                }
                catch (const std::exception &e)
                {
                    throwError(
                        std::format("Option '{}' of node '{}' has invalid Time format: {}. Error: {}",
                                    optionId, nodeId, timeStr, e.what()),
                        call.lineno, call.col_offset);
                }
            }
            else if (std::holds_alternative<epoch_frame::Time>(rawValue))
            {
                // Already a Time object
                return rawValue;
            }
            else
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects Time (string) but got invalid type",
                                optionId, nodeId),
                    call.lineno, call.col_offset);
            }
        }

        case MetaType::NumericList:
        case MetaType::StringList:
        {
            // Expect a Sequence
            if (!std::holds_alternative<epoch_metadata::Sequence>(rawValue))
            {
                throwError(
                    std::format("Option '{}' of node '{}' expects {} but got non-list value",
                                optionId, nodeId,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(metaOption.type)),
                    call.lineno, call.col_offset);
            }

            const auto &sequence = std::get<epoch_metadata::Sequence>(rawValue);

            // Validate sequence elements match expected type
            for (const auto &item : sequence)
            {
                if (metaOption.type == MetaType::NumericList && !std::holds_alternative<double>(item))
                {
                    throwError(
                        std::format("Option '{}' of node '{}' expects NumericList but contains non-numeric values",
                                    optionId, nodeId),
                        call.lineno, call.col_offset);
                }
                else if (metaOption.type == MetaType::StringList && !std::holds_alternative<std::string>(item))
                {
                    throwError(
                        std::format("Option '{}' of node '{}' expects StringList but contains non-string values",
                                    optionId, nodeId),
                        call.lineno, call.col_offset);
                }
            }

            return rawValue;
        }

        default:
            throwError(
                std::format("Unsupported metadata option type: {}",
                            epoch_core::MetaDataOptionTypeWrapper::ToString(metaOption.type)),
                call.lineno, call.col_offset);
        }

        return rawValue; // Unreachable, but keeps compiler happy
    }

    void AlgorithmAstCompiler::validateAndApplyOptions(
        const std::string &node_id,
        const epoch_metadata::transforms::TransformsMetaData &comp_meta,
        std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &kwargs,
        const Call &call)
    {
        //  1. Apply default options for missing required parameters
        for (const auto &metaOption : comp_meta.options)
        {
            if (metaOption.isRequired &&
                !kwargs.contains(metaOption.id) &&
                metaOption.defaultValue.has_value())
            {
                kwargs[metaOption.id] = metaOption.defaultValue->GetVariant();
            }
        }

        // 2. Validate required options are present
        for (const auto &metaOption : comp_meta.options)
        {
            if (metaOption.isRequired && !kwargs.contains(metaOption.id))
            {
                std::string suggestion = metaOption.defaultValue
                                             ? metaOption.defaultValue->ToString()
                                             : "required";

                throwError(
                    std::format("Node '{}' of type '{}' is missing required option '{}'. "
                                "Add option '{}' with type {}. Suggested value: {}",
                                node_id, comp_meta.name, metaOption.id,
                                metaOption.id,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(metaOption.type),
                                suggestion),
                    call.lineno, call.col_offset);
            }
        }

        // 3. Parse and validate all kwargs based on metadata types
        for (auto &[optionId, optionValue] : kwargs)
        {
            // Skip special parameters (timeframe and session) - they're handled separately
            if (optionId == "timeframe" || optionId == "session")
            {
                continue;
            }

            // Find metadata for this option
            auto metaOptionIt = std::find_if(
                comp_meta.options.begin(), comp_meta.options.end(),
                [&](const auto &opt)
                { return opt.id == optionId; });

            if (metaOptionIt == comp_meta.options.end())
            {
                throwError(
                    std::format("Unknown option '{}' for node '{}' of type '{}'. "
                                "Remove option '{}' or check if you meant a different option name",
                                optionId, node_id, comp_meta.name, optionId),
                    call.lineno, call.col_offset);
            }

            const auto &metaOption = *metaOptionIt;

            // Parse value based on metadata type
            optionValue = parseOptionByMetadata(optionValue, metaOption, optionId, node_id, call, comp_meta);
        }

        // kwargs now contains validated, clamped values with defaults applied
        // Caller will use these to populate algo.options
    }

    void AlgorithmAstCompiler::canonicalizeTimeframe(std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params)
    {
        // Validate timeframe is a pandas offset string if provided; do not mutate unless empty
        if (params.contains("timeframe"))
        {
            const auto &tf_val = params.at("timeframe");

            // Check if it's a string
            if (std::holds_alternative<std::string>(tf_val))
            {
                const std::string &tf_str = std::get<std::string>(tf_val);

                // Skip empty strings - treat as "not specified"
                if (tf_str.empty())
                {
                    params.erase("timeframe");
                    return;
                }

                // Validate it's a string (already confirmed above)
                // Could add pandas offset validation here similar to Python
                // For now, accept any non-empty string
            }
            else
            {
                throwError("Parameter 'timeframe' must be a string (pandas offset)");
            }
        }
    }

    void AlgorithmAstCompiler::canonicalizeSession(std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params)
    {
        // Validate session parameter if provided - must be a string literal
        if (params.contains("session"))
        {
            const auto &session_val = params.at("session");

            // Check if it's a string
            if (std::holds_alternative<std::string>(session_val))
            {
                const std::string &session_str = std::get<std::string>(session_val);

                // Skip empty strings - treat as "not specified"
                if (session_str.empty())
                {
                    params.erase("session");
                    return;
                }

                // Validate against predefined sessions
                validateSession(session_str);
            }
            else
            {
                throwError("Parameter 'session' must be a string literal");
            }
        }
    }

    void AlgorithmAstCompiler::applySpecialFields(
        epoch_metadata::strategy::AlgorithmNode &algo,
        const std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params)
    {
        // Extract timeframe and session from params and set as separate fields
        // These should NOT be added to the options map
        // NOTE: For now, we store as strings. Later we'll extend to support variable references.
        // TODO: See TODO.md items 5-7 for future enhancements

        // Handle timeframe as special field (store as string literal for now)
        if (params.contains("timeframe"))
        {
            const auto &tf_value = params.at("timeframe");
            if (std::holds_alternative<std::string>(tf_value))
            {
                const std::string &tf_str = std::get<std::string>(tf_value);
                // Validate timeframe (should already be validated by canonicalizeTimeframe, but double-check)
                validateTimeframe(tf_str);
                // Convert string to TimeFrame object
                algo.timeframe = epoch_metadata::TimeFrame(tf_str);
            }
            else
            {
                throwError("Parameter 'timeframe' must be a string (pandas offset)");
            }
        }

        // Handle session as special field (store as SessionType enum for now)
        if (params.contains("session"))
        {
            const auto &session_value = params.at("session");
            if (std::holds_alternative<std::string>(session_value))
            {
                const std::string &session_str = std::get<std::string>(session_value);
                // Convert string to SessionType using wrapper
                algo.session = epoch_core::SessionTypeWrapper::FromString(session_str);
            }
            else
            {
                throwError("Parameter 'session' must be a string literal");
            }
        }
    }

    void AlgorithmAstCompiler::verifySessionDependencies()
    {
        // Verify that all nodes with session parameters have corresponding sessions nodes
        // Track required sessions: {(session_val, timeframe_str): [node_ids]}
        std::map<std::pair<std::string, std::optional<std::string>>, std::vector<std::string>> required_sessions;

        // Scan all nodes for session fields
        for (const auto &node : algorithms_)
        {
            if (!node.session || node.type == "sessions")
            {
                continue;
            }

            // Extract session string from SessionVariant
            std::string session_str;
            if (std::holds_alternative<epoch_core::SessionType>(*node.session))
            {
                session_str = epoch_core::SessionTypeWrapper::ToString(std::get<epoch_core::SessionType>(*node.session));
            }
            else
            {
                // SessionRange - skip for now as we don't handle these in auto-creation
                continue;
            }

            // Extract timeframe string
            std::optional<std::string> timeframe_str;
            if (node.timeframe.has_value())
            {
                timeframe_str = node.timeframe->ToString();
            }

            std::pair<std::string, std::optional<std::string>> key = {session_str, timeframe_str};
            required_sessions[key].push_back(node.id);
        }

        // For each required session, ensure a sessions node exists
        // Use a separate counter for session nodes to match Python's numbering (starts at 0)
        int session_counter = 0;
        for (const auto &[key, node_ids] : required_sessions)
        {
            const auto &[session_val, timeframe_str] = key;

            // Check if a matching sessions node exists
            bool has_sessions_node = false;
            for (const auto &node : algorithms_)
            {
                if (node.type == "sessions")
                {
                    // Check if this sessions node matches
                    bool session_matches = false;
                    if (node.options.contains("session"))
                    {
                        const auto &opt_val = node.options.at("session").GetVariant();
                        if (std::holds_alternative<std::string>(opt_val))
                        {
                            session_matches = (std::get<std::string>(opt_val) == session_val);
                        }
                    }
                    else if (node.options.contains("session_type"))
                    {
                        const auto &opt_val = node.options.at("session_type").GetVariant();
                        if (std::holds_alternative<std::string>(opt_val))
                        {
                            session_matches = (std::get<std::string>(opt_val) == session_val);
                        }
                    }

                    // Compare timeframe strings
                    bool timeframe_matches = false;
                    if (node.timeframe.has_value() && timeframe_str.has_value())
                    {
                        timeframe_matches = (node.timeframe->ToString() == *timeframe_str);
                    }
                    else if (!node.timeframe.has_value() && !timeframe_str.has_value())
                    {
                        timeframe_matches = true;
                    }

                    if (session_matches && timeframe_matches)
                    {
                        has_sessions_node = true;
                        break;
                    }
                }
            }

            // If no matching sessions node exists, create one
            if (!has_sessions_node)
            {
                std::string synthetic_id = "sessions_" + std::to_string(session_counter++);

                epoch_metadata::strategy::AlgorithmNode sessions_algo;
                sessions_algo.id = synthetic_id;
                sessions_algo.type = "sessions";
                sessions_algo.options["session"] = epoch_metadata::MetaDataOptionDefinition{session_val};
                if (timeframe_str.has_value())
                {
                    sessions_algo.timeframe = epoch_metadata::TimeFrame(*timeframe_str);
                }

                algorithms_.push_back(std::move(sessions_algo));
                node_lookup_[synthetic_id] = algorithms_.size() - 1;
            }
        }
    }

    void AlgorithmAstCompiler::resolveTimeframes(const std::optional<epoch_metadata::TimeFrame> &baseTimeframe)
    {
        // Use TimeframeResolver utility to resolve timeframes for all nodes
        TimeframeResolver resolver;

        // Process all algorithms and resolve their timeframes
        for (auto &algo : algorithms_)
        {
            auto resolvedTimeframe = resolver.ResolveNodeTimeframe(algo, baseTimeframe);
            if (resolvedTimeframe)
            {
                algo.timeframe = resolvedTimeframe;
            }
        }
    }


    DataType AlgorithmAstCompiler::getNodeOutputType(const std::string &nodeId, const std::string &handle)
    {
        // Check tracked output types first
        auto tracked_it = node_output_types_.find(nodeId);
        if (tracked_it != node_output_types_.end())
        {
            auto handle_it = tracked_it->second.find(handle);
            if (handle_it != tracked_it->second.end())
            {
                return handle_it->second;
            }
        }

        // Check if it's a known node in our algorithms list
        auto node_it = node_lookup_.find(nodeId);
        if (node_it != node_lookup_.end())
        {
            const auto &algo = algorithms_[node_it->second];
            // Copy the string to avoid reference invalidation after vector reallocation
            const std::string node_type = algo.type;

            // Check registry for output types
            const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
            const auto &all_metadata = registry.GetMetaData();
            if (all_metadata.contains(node_type))
            {
                const auto &comp_meta = all_metadata.at(node_type);
                for (const auto &output : comp_meta.outputs)
                {
                    if (output.id == handle)
                    {
                        // Map IOMetaData type to DataType enum using EnumWrapper
                        std::string type_str = epoch_core::EnumWrapper<epoch_core::IODataType>::type::ToString(output.type);
                        if (type_str == "Boolean")
                            return DataType::Boolean;
                        else if (type_str == "Integer")
                            return DataType::Integer;
                        else if (type_str == "Decimal")
                            return DataType::Decimal;
                        else if (type_str == "Number")
                            return DataType::Number;
                        else if (type_str == "String")
                            return DataType::String;
                        else
                            return DataType::Any;
                    }
                }
            }

            // Special cases for operators and literals
            if (node_type == "lt" || node_type == "gt" || node_type == "lte" ||
                node_type == "gte" || node_type == "eq" || node_type == "neq" ||
                node_type == "logical_and" || node_type == "logical_or" || node_type == "logical_not")
            {
                return DataType::Boolean;
            }
            else if (node_type == "add" || node_type == "sub" ||
                     node_type == "mul" || node_type == "div")
            {
                return DataType::Decimal;
            }
            else if (node_type == "number")
            {
                return DataType::Decimal;
            }
            else if (node_type == "bool_true" || node_type == "bool_false")
            {
                return DataType::Boolean;
            }
            else if (node_type == "text")
            {
                return DataType::String;
            }
            else if (node_type == "null")
            {
                return DataType::Any;
            }
        }

        // Default to Any if unknown
        return DataType::Any;
    }

    bool AlgorithmAstCompiler::isTypeCompatible(DataType source, DataType target)
    {
        // Any type accepts all
        if (target == DataType::Any || source == DataType::Any)
        {
            return true;
        }

        // Exact match
        if (source == target)
        {
            return true;
        }

        // Number accepts Integer and Decimal
        if (target == DataType::Number && (source == DataType::Integer || source == DataType::Decimal))
        {
            return true;
        }

        // Otherwise incompatible
        return false;
    }

    std::optional<std::string> AlgorithmAstCompiler::needsTypeCast(DataType source, DataType target)
    {
        // If types are already compatible, no cast needed
        if (isTypeCompatible(source, target))
        {
            return std::nullopt;
        }

        // Boolean to Number/Decimal/Integer
        if (source == DataType::Boolean &&
            (target == DataType::Number || target == DataType::Decimal || target == DataType::Integer))
        {
            return "bool_to_num";
        }

        // Number/Decimal/Integer to Boolean
        if ((source == DataType::Number || source == DataType::Decimal || source == DataType::Integer) &&
            target == DataType::Boolean)
        {
            return "num_to_bool";
        }

        // Incompatible types that can't be cast
        return "incompatible";
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::insertTypeCast(
        const ValueHandle &source,
        DataType sourceType,
        DataType targetType)
    {
        // Check if casting is needed
        auto cast_method = needsTypeCast(sourceType, targetType);

        if (!cast_method.has_value())
        {
            // No casting needed - types are compatible
            return source;
        }

        if (cast_method.value() == "bool_to_num")
        {
            // Use boolean_select to convert boolean to number
            // boolean_select(condition, true_value, false_value)
            std::string cast_node_id = uniqueNodeId("bool_to_num_cast");

            epoch_metadata::strategy::AlgorithmNode cast_algo;
            cast_algo.id = cast_node_id;
            cast_algo.type = "boolean_select";

            // Wire the boolean to condition
            cast_algo.inputs["condition"].push_back(joinId(source.node_id, source.handle));

            // Create number nodes for true (1) and false (0)
            ValueHandle true_node = materializeNumber(1.0);
            ValueHandle false_node = materializeNumber(0.0);

            // Wire the numbers to true and false inputs
            cast_algo.inputs["true"].push_back(joinId(true_node.node_id, true_node.handle));
            cast_algo.inputs["false"].push_back(joinId(false_node.node_id, false_node.handle));

            // Add to algorithms list
            algorithms_.push_back(std::move(cast_algo));
            node_lookup_[cast_node_id] = algorithms_.size() - 1;

            // Track output type
            node_output_types_[cast_node_id]["result"] = DataType::Number;

            return {cast_node_id, "result"};
        }
        else if (cast_method.value() == "num_to_bool")
        {
            // Use neq (not equal) to convert number to boolean (num != 0)
            std::string cast_node_id = uniqueNodeId("num_to_bool_cast");

            epoch_metadata::strategy::AlgorithmNode cast_algo;
            cast_algo.id = cast_node_id;
            cast_algo.type = "neq";

            // Wire the number to SLOT0
            cast_algo.inputs["SLOT0"].push_back(joinId(source.node_id, source.handle));

            // Create zero node
            ValueHandle zero_node = materializeNumber(0.0);

            // Wire zero to SLOT1
            cast_algo.inputs["SLOT1"].push_back(joinId(zero_node.node_id, zero_node.handle));

            // Add to algorithms list
            algorithms_.push_back(std::move(cast_algo));
            node_lookup_[cast_node_id] = algorithms_.size() - 1;

            // Track output type
            node_output_types_[cast_node_id]["result"] = DataType::Boolean;

            return {cast_node_id, "result"};
        }
        else
        {
            // Incompatible types that can't be cast
            throwError("Type mismatch: Cannot convert " + dataTypeToString(sourceType) +
                       " to " + dataTypeToString(targetType));
            return source;
        }
    }

    std::string AlgorithmAstCompiler::dataTypeToString(DataType type)
    {
        switch (type)
        {
        case DataType::Boolean:
            return "Boolean";
        case DataType::Integer:
            return "Integer";
        case DataType::Decimal:
            return "Decimal";
        case DataType::Number:
            return "Number";
        case DataType::String:
            return "String";
        case DataType::Any:
            return "Any";
        default:
            return "Unknown";
        }
    }

    void AlgorithmAstCompiler::throwError(const std::string &msg, int line, int col)
    {
        std::string fullMsg = msg;
        // Tree-sitter provides 1-based line numbers (line 1 is the first line)
        // Only add location info if line > 0
        if (line > 0)
        {
            fullMsg += " (line " + std::to_string(line) + ", col " + std::to_string(col) + ")";
        }
        throw std::runtime_error(fullMsg);
    }

    // Attribute resolution helpers
    std::pair<std::string, std::string> AlgorithmAstCompiler::attributeToTuple(const Attribute &attr)
    {
        std::vector<std::string> parts;
        const Expr *current = &attr;

        // Walk backwards through the attribute chain
        while (auto *attrNode = dynamic_cast<const Attribute *>(current))
        {
            parts.push_back(attrNode->attr);
            current = attrNode->value.get();
        }

        // Base should be a Name
        if (auto *nameNode = dynamic_cast<const Name *>(current))
        {
            parts.push_back(nameNode->id);
        }
        else
        {
            throwError("Invalid attribute base - must be a name");
        }

        // Reverse to get correct order
        std::reverse(parts.begin(), parts.end());

        if (parts.size() < 2)
        {
            throwError("Attribute must have at least base.handle");
        }

        std::string var = parts[0];
        std::string handle = parts[1];
        for (size_t i = 2; i < parts.size(); ++i)
        {
            handle += "." + parts[i];
        }

        return {var, handle};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::resolveHandle(
        const std::string &var,
        const std::string &handle)
    {
        // Check if var is bound to a node.handle
        auto it = var_to_binding_.find(var);
        if (it != var_to_binding_.end())
        {
            const std::string &ref = it->second;
            if (ref.find('.') != std::string::npos)
            {
                throwError("Cannot access handle '" + handle + "' on '" + var +
                           "' which is already bound to '" + ref + "'");
            }
        }

        // Var should be a node name - look up its component type
        std::string comp_name;
        if (it != var_to_binding_.end())
        {
            comp_name = it->second;
        }
        else
        {
            // Check if it's a direct node reference (use node_lookup_)
            if (node_lookup_.contains(var))
            {
                comp_name = algorithms_[node_lookup_[var]].type;
            }
            else
            {
                throwError("Unknown node '" + var + "'");
            }
        }

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(comp_name))
        {
            throwError("Unknown component '" + comp_name + "'");
        }

        const auto &comp_meta = all_metadata.at(comp_name);

        // Extract valid handles from outputs and inputs
        std::set<std::string> valid_handles;

        const auto &outputs = comp_meta.outputs;
        for (const auto &output : outputs)
        {
            valid_handles.insert(output.id);
        }

        const auto &inputs = comp_meta.inputs;
        for (const auto &input : inputs)
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
            throwError("Unknown handle '" + handle + "' on '" + var + "'");
        }

        return {var, handle};
    }

    // Materialize literal nodes - build AlgorithmNode directly
    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::materializeNumber(double value)
    {
        std::string node_id = uniqueNodeId("number");

        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "number";
        algo.options["value"] = epoch_metadata::MetaDataOptionDefinition{value};

        algorithms_.push_back(std::move(algo)); // Move for zero-copy
        node_lookup_[node_id] = algorithms_.size() - 1;
        var_to_binding_[node_id] = "number";
        node_output_types_[node_id]["result"] = DataType::Decimal;

        return {node_id, "result"};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::materializeBoolean(bool value)
    {
        std::string node_type = value ? "bool_true" : "bool_false";
        std::string node_id = uniqueNodeId(node_type);

        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = node_type;
        // No options needed for boolean nodes

        algorithms_.push_back(std::move(algo));
        node_lookup_[node_id] = algorithms_.size() - 1;
        var_to_binding_[node_id] = node_type;
        node_output_types_[node_id]["result"] = DataType::Boolean;

        return {node_id, "result"};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::materializeText(const std::string &value)
    {
        std::string node_id = uniqueNodeId("text");

        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "text";
        algo.options["value"] = epoch_metadata::MetaDataOptionDefinition{value};

        algorithms_.push_back(std::move(algo));
        node_lookup_[node_id] = algorithms_.size() - 1;
        var_to_binding_[node_id] = "text";
        node_output_types_[node_id]["result"] = DataType::String;

        return {node_id, "result"};
    }

    AlgorithmAstCompiler::ValueHandle AlgorithmAstCompiler::materializeNull()
    {
        std::string node_id = uniqueNodeId("null");

        epoch_metadata::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "null";
        // No options needed for null node

        algorithms_.push_back(std::move(algo));
        node_lookup_[node_id] = algorithms_.size() - 1;
        var_to_binding_[node_id] = "null";
        node_output_types_[node_id]["result"] = DataType::Any;

        return {node_id, "result"};
    }

    // Constructor call parsing helpers
    bool AlgorithmAstCompiler::isConstructorCall(const Expr &expr)
    {
        // Must have at least one Call in the chain
        if (auto *call = dynamic_cast<const Call *>(&expr))
        {
            const Expr *cur = call;
            while (auto *callNode = dynamic_cast<const Call *>(cur))
            {
                cur = callNode->func.get();
            }
            // Base must be a Name (component name)
            return dynamic_cast<const Name *>(cur) != nullptr;
        }
        return false;
    }

    AlgorithmAstCompiler::ConstructorParseResult AlgorithmAstCompiler::parseConstructorAndFeeds(const Call &call)
    {
        // Collect all calls in the chain
        std::vector<const Call *> calls;
        const Expr *cur = &call;
        while (auto *callNode = dynamic_cast<const Call *>(cur))
        {
            calls.push_back(callNode);
            cur = callNode->func.get();
        }

        // Base must be a Name
        auto *nameNode = dynamic_cast<const Name *>(cur);
        if (!nameNode)
        {
            throwError("Right-hand side must be a constructor call (e.g., ema(...)(...))", call.lineno, call.col_offset);
        }

        std::string ctor_name = nameNode->id;
        std::reverse(calls.begin(), calls.end());

        // Get component metadata for option parsing
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(ctor_name))
        {
            throwError("Unknown component '" + ctor_name + "'", call.lineno, call.col_offset);
        }
        const auto &comp_meta = all_metadata.at(ctor_name);

        // Build metadata lookup map for O(1) lookups
        std::unordered_map<std::string, epoch_metadata::MetaDataOption> option_metadata;
        for (const auto &opt : comp_meta.options)
        {
            option_metadata[opt.id] = opt;
        }

        ConstructorParseResult result;
        result.ctor_name = ctor_name;

        // Parse constructor kwargs from first call
        for (const auto &[key, valueExpr] : calls[0]->keywords)
        {
            // Skip special parameters (timeframe and session) - validated separately
            if (key == "timeframe" || key == "session")
            {
                // Extract raw string value (validated later by canonicalizeTimeframe/Session)
                if (auto *constant = dynamic_cast<const Constant *>(valueExpr.get()))
                {
                    if (std::holds_alternative<std::string>(constant->value))
                    {
                        result.ctor_kwargs[key] = std::get<std::string>(constant->value);
                    }
                    else
                    {
                        throwError(
                            std::format("Parameter '{}' must be a string", key),
                            calls[0]->lineno, calls[0]->col_offset);
                    }
                }
                else if (auto *name = dynamic_cast<const Name *>(valueExpr.get()))
                {
                    // Bare identifier like sessions(session=London)
                    result.ctor_kwargs[key] = name->id;
                }
                else
                {
                    throwError(
                        std::format("Parameter '{}' must be a string literal", key),
                        calls[0]->lineno, calls[0]->col_offset);
                }
                continue;
            }

            // Look up metadata for this option (throw error if not found - invalid option)
            auto it = option_metadata.find(key);
            if (it == option_metadata.end())
            {
                throwError(
                    std::format("Unknown option '{}' for component '{}'", key, ctor_name),
                    calls[0]->lineno, calls[0]->col_offset);
            }

            result.ctor_kwargs[key] = parseLiteralOrPrimitive(*valueExpr, it->second, comp_meta);
        }

        // Handle shorthand syntax: component(inputs) instead of component()(inputs)
        // If first call has args and component has no options, treat args as feed inputs
        std::vector<ValueHandle> feed_step_args;
        std::unordered_map<std::string, ValueHandle> feed_step_kwargs;

        if (!calls[0]->args.empty())
        {
            bool has_options = !comp_meta.options.empty();

            if (!has_options && calls.size() == 1)
            {
                // Shorthand: treat positional args as feed inputs
                for (const auto &argExpr : calls[0]->args)
                {
                    feed_step_args.push_back(visitExpr(*argExpr));
                }
                result.feed_steps.push_back({feed_step_args, feed_step_kwargs});
            }
            else
            {
                throwError("Positional constructor arguments not supported; use keyword args", calls[0]->lineno, calls[0]->col_offset);
            }
        }

        // Parse subsequent feed steps
        for (size_t i = 1; i < calls.size(); ++i)
        {
            std::vector<ValueHandle> args;
            std::unordered_map<std::string, ValueHandle> kwargs;

            for (const auto &argExpr : calls[i]->args)
            {
                args.push_back(visitExpr(*argExpr));
            }

            for (const auto &[key, valueExpr] : calls[i]->keywords)
            {
                kwargs[key] = visitExpr(*valueExpr);
            }

            result.feed_steps.push_back({args, kwargs});
        }

        return result;
    }

    epoch_metadata::MetaDataOptionDefinition::T AlgorithmAstCompiler::parseLiteralOrPrimitive(
        const Expr &expr,
        const epoch_metadata::MetaDataOption &metaOption,
        const epoch_metadata::transforms::TransformsMetaData &comp_meta)
    {
        // Extract raw value from AST expression
        epoch_metadata::MetaDataOptionDefinition::T rawValue;

        if (auto *constant = dynamic_cast<const Constant *>(&expr))
        {
            rawValue = std::visit([](auto &&value) -> epoch_metadata::MetaDataOptionDefinition::T
                              {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>) {
                return static_cast<double>(value);
            } else if constexpr (std::is_same_v<T, double>) {
                return value;
            } else if constexpr (std::is_same_v<T, bool>) {
                return value;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return value;
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                return std::string{""};  // Return empty string for null/None
            }
            throw std::runtime_error("Unsupported constant type"); }, constant->value);
        }
        else if (auto *name = dynamic_cast<const Name *>(&expr))
        {
            // Check if this name is bound to a constant value
            auto it = var_to_binding_.find(name->id);
            if (it != var_to_binding_.end() && it->second.find('.') != std::string::npos)
            {
                std::string node_id = it->second.substr(0, it->second.find('.'));

                // Check if it's a literal node (use node_lookup_ and algorithms_)
                if (node_lookup_.contains(node_id))
                {
                    const auto &algo = algorithms_[node_lookup_[node_id]];
                    if (algo.type == "number")
                    {
                        if (algo.options.contains("value"))
                        {
                            rawValue = algo.options.at("value").GetVariant();
                        }
                        else
                        {
                            throwError("Number node missing value option");
                        }
                    }
                    else if (algo.type == "bool_true")
                    {
                        rawValue = true;
                    }
                    else if (algo.type == "bool_false")
                    {
                        rawValue = false;
                    }
                    else
                    {
                        throwError("Only literal values supported for options");
                    }
                }
                else
                {
                    throwError("Only literal values supported for options");
                }
            }
            else
            {
                // Fallback: accept bare identifiers as strings (e.g., sessions(session=London))
                rawValue = name->id;
            }
        }
        else
        {
            throwError("Only literal keyword values supported");
        }

        // Delegate to parseOptionByMetadata for type-aware parsing
        // Note: We don't have access to the original Call node here, but parseOptionByMetadata
        // will use the Call passed from parseConstructorAndFeeds which has the correct location
        // So we create a dummy call for now - the real error locations come from the caller
        auto dummyFunc = std::make_unique<Name>("dummy");
        Call dummyCall{std::move(dummyFunc)};
        dummyCall.lineno = 0;
        dummyCall.col_offset = 0;

        return parseOptionByMetadata(rawValue, metaOption, metaOption.id, comp_meta.id, dummyCall, comp_meta);
    }

    void AlgorithmAstCompiler::handleConstructorAssignment(const Expr &target, const Expr &value, const Assign &assign)
    {
        auto parseResult = parseConstructorAndFeeds(dynamic_cast<const Call &>(value));

        // Validate component exists in singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(parseResult.ctor_name))
        {
            throwError("Unknown component '" + parseResult.ctor_name + "'", assign.lineno, assign.col_offset);
        }

        const auto &comp_meta = all_metadata.at(parseResult.ctor_name);

        // Case 1: Single name target (e.g., x = ema(period=20)(src.c))
        if (auto *nameTarget = dynamic_cast<const Name *>(&target))
        {
            std::string node_id = nameTarget->id;

            // Ensure variable is not already bound
            if (node_id != "_" && var_to_binding_.count(node_id))
            {
                throwError("Variable '" + node_id + "' already bound", assign.lineno, assign.col_offset);
            }

            // Canonicalize special parameters
            auto params = parseResult.ctor_kwargs;
            canonicalizeTimeframe(params);
            canonicalizeSession(params);

            // Validate and apply option defaults/clamping
            validateAndApplyOptions(node_id, comp_meta, params, dynamic_cast<const Call &>(value));

            // Create AlgorithmNode
            epoch_metadata::strategy::AlgorithmNode algo;
            algo.id = node_id;
            algo.type = parseResult.ctor_name;

            // Convert regular options (excluding timeframe and session)
            for (const auto &[key, value] : params)
            {
                if (key != "timeframe" && key != "session")
                {
                    algo.options[key] = epoch_metadata::MetaDataOptionDefinition{value};
                }
            }

            // Apply special fields (timeframe and session)
            applySpecialFields(algo, params);

            // Add to algorithms list first (so wireInputs can access it)
            algorithms_.push_back(std::move(algo));
            node_lookup_[node_id] = algorithms_.size() - 1;
            var_to_binding_[node_id] = parseResult.ctor_name;

            // Track executor count
            if (parseResult.ctor_name == "trade_signal_executor")
            {
                executor_count_++;
            }

            // Wire inputs from feed steps
            for (const auto &[args, kwargs] : parseResult.feed_steps)
            {
                wireInputs(node_id, parseResult.ctor_name, args, kwargs);
            }

            return;
        }

        // Case 2: Tuple target (e.g., a, b = macd()(src.c))
        if (auto *tupleTarget = dynamic_cast<const Tuple *>(&target))
        {
            std::vector<std::string> names;
            for (const auto &elt : tupleTarget->elts)
            {
                if (auto *nameElt = dynamic_cast<const Name *>(elt.get()))
                {
                    names.push_back(nameElt->id);
                }
                else
                {
                    throwError("Tuple targets must be simple names", assign.lineno, assign.col_offset);
                }
            }

            // Ensure all names are not already bound
            for (const auto &name : names)
            {
                if (name != "_" && var_to_binding_.count(name))
                {
                    throwError("Variable '" + name + "' already bound", assign.lineno, assign.col_offset);
                }
            }

            // Create synthetic node ID
            std::string synthetic_id = uniqueNodeId("node");

            // Canonicalize special parameters
            auto params = parseResult.ctor_kwargs;
            canonicalizeTimeframe(params);
            canonicalizeSession(params);

            // Validate and apply option defaults/clamping
            validateAndApplyOptions(synthetic_id, comp_meta, params, dynamic_cast<const Call &>(value));

            // Create AlgorithmNode
            epoch_metadata::strategy::AlgorithmNode algo;
            algo.id = synthetic_id;
            algo.type = parseResult.ctor_name;

            // Convert regular options (excluding timeframe and session)
            for (const auto &[key, value] : params)
            {
                if (key != "timeframe" && key != "session")
                {
                    algo.options[key] = epoch_metadata::MetaDataOptionDefinition{value};
                }
            }

            // Apply special fields (timeframe and session)
            applySpecialFields(algo, params);

            // Add to algorithms list
            algorithms_.push_back(std::move(algo));
            node_lookup_[synthetic_id] = algorithms_.size() - 1;
            var_to_binding_[synthetic_id] = parseResult.ctor_name;

            // Track executor count
            if (parseResult.ctor_name == "trade_signal_executor")
            {
                executor_count_++;
            }

            // Wire inputs from feed steps
            for (const auto &[args, kwargs] : parseResult.feed_steps)
            {
                wireInputs(synthetic_id, parseResult.ctor_name, args, kwargs);
            }

            // Extract output handles and bind to tuple variables
            const auto &outputs = comp_meta.outputs;
            if (outputs.size() != names.size())
            {
                throwError("Expected " + std::to_string(outputs.size()) + " outputs, got " + std::to_string(names.size()),
                           assign.lineno, assign.col_offset);
            }

            for (size_t i = 0; i < names.size(); ++i)
            {
                std::string handle = outputs[i].id;
                // Don't bind underscore variables - they're throwaway
                if (names[i] != "_")
                {
                    var_to_binding_[names[i]] = synthetic_id + "." + handle;
                }
            }

            return;
        }

        throwError("Unsupported assignment target", assign.lineno, assign.col_offset);
    }

    void AlgorithmAstCompiler::handleNonConstructorAssignment(const Expr &target, const Expr &value, const Assign &assign)
    {
        // Handle non-constructor assignments (operators, name references, etc.)
        if (auto *nameTarget = dynamic_cast<const Name *>(&target))
        {
            std::string node_id = nameTarget->id;

            // Ensure variable is not already bound
            if (node_id != "_" && var_to_binding_.count(node_id))
            {
                throwError("Variable '" + node_id + "' already bound", assign.lineno, assign.col_offset);
            }

            // Parse the value and resolve to a handle
            ValueHandle handle = visitExpr(value);

            // Bind the variable to the source node.handle
            var_to_binding_[node_id] = handle.node_id + "." + handle.handle;
            return;
        }

        throwError("Unsupported non-constructor assignment target", assign.lineno, assign.col_offset);
    }

    void AlgorithmAstCompiler::wireInputs(
        const std::string &target_node_id,
        const std::string &component_name,
        const std::vector<ValueHandle> &args,
        const std::unordered_map<std::string, ValueHandle> &kwargs)
    {
        // Get component metadata from singleton registry
        const auto &registry = epoch_metadata::transforms::ITransformRegistry::GetInstance();
        const auto &all_metadata = registry.GetMetaData();
        if (!all_metadata.contains(component_name))
        {
            throwError("Unknown component '" + component_name + "'");
        }

        const auto &comp_meta = all_metadata.at(component_name);

        // Extract input IDs and types from component metadata
        std::vector<std::string> input_ids;
        std::unordered_map<std::string, DataType> input_types;
        const auto &inputs = comp_meta.inputs;
        for (const auto &input : inputs)
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
        auto findTargetNode = [this](const std::string &node_id) -> epoch_metadata::strategy::AlgorithmNode * {
            for (auto &algo : algorithms_)
            {
                if (algo.id == node_id)
                {
                    return &algo;
                }
            }
            return nullptr;
        };

        // Wire keyword arguments to inputs map
        for (const auto &[name, handle] : kwargs)
        {
            // Validate input name exists
            if (std::find(input_ids.begin(), input_ids.end(), name) == input_ids.end())
            {
                throwError("Unknown input handle '" + name + "' for '" + target_node_id + "'");
            }

            // Type checking: get source and target types
            DataType source_type = getNodeOutputType(handle.node_id, handle.handle);
            DataType target_type = input_types[name];

            // Check if types are compatible
            if (!isTypeCompatible(source_type, target_type))
            {
                // Try to insert type cast
                auto cast_result = needsTypeCast(source_type, target_type);
                if (cast_result.has_value() && cast_result.value() != "incompatible")
                {
                    // Insert cast node (this may reallocate algorithms_ vector)
                    ValueHandle casted = insertTypeCast(handle, source_type, target_type);
                    // Find target node by ID after potential reallocation
                    auto *target_node = findTargetNode(target_node_id);
                    if (target_node)
                    {
                        target_node->inputs[name].push_back(joinId(casted.node_id, casted.handle));
                    }
                }
                else
                {
                    // Incompatible types - throw error
                    throwError("Type mismatch for input '" + name + "' of '" + target_node_id + "': expected " +
                               dataTypeToString(target_type) + ", got " + dataTypeToString(source_type));
                }
            }
            else
            {
                // Types are compatible - wire directly
                auto *target_node = findTargetNode(target_node_id);
                if (target_node)
                {
                    target_node->inputs[name].push_back(joinId(handle.node_id, handle.handle));
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
                throwError("Too many positional inputs for '" + target_node_id + "'");
            }

            for (size_t i = 0; i < args.size(); ++i)
            {
                const auto &handle = args[i];
                // For variadic inputs, all extra args go to the last input slot
                std::string dst_handle = (i < input_ids.size()) ? input_ids[i] : input_ids.back();

                // Type checking: get source and target types
                DataType source_type = getNodeOutputType(handle.node_id, handle.handle);
                DataType target_type = input_types[dst_handle];

                // Check if types are compatible
                if (!isTypeCompatible(source_type, target_type))
                {
                    // Try to insert type cast
                    auto cast_result = needsTypeCast(source_type, target_type);
                    if (cast_result.has_value() && cast_result.value() != "incompatible")
                    {
                        // Insert cast node (this may reallocate algorithms_ vector)
                        ValueHandle casted = insertTypeCast(handle, source_type, target_type);
                        // Find target node by ID after potential reallocation
                        auto *target_node = findTargetNode(target_node_id);
                        if (target_node)
                        {
                            target_node->inputs[dst_handle].push_back(joinId(casted.node_id, casted.handle));
                        }
                    }
                    else
                    {
                        // Incompatible types - throw error
                        throwError("Type mismatch for positional input " + std::to_string(i) + " of '" + target_node_id + "': expected " +
                                   dataTypeToString(target_type) + ", got " + dataTypeToString(source_type));
                    }
                }
                else
                {
                    // Types are compatible - wire directly
                    auto *target_node = findTargetNode(target_node_id);
                    if (target_node)
                    {
                        target_node->inputs[dst_handle].push_back(joinId(handle.node_id, handle.handle));
                    }
                }
            }
        }
    }

    // Convenience function
    CompilationResult compileAlgorithm(const std::string &source)
    {
        AlgorithmAstCompiler compiler;
        return compiler.compile(source);
    }

} // namespace epoch_stratifyx::epochflow
