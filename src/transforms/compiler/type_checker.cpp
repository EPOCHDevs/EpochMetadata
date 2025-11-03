//
// Created by Claude Code
// EpochScript Type Checker Implementation
//

#include "type_checker.h"
#include <epoch_core/enum_wrapper.h>
#include <stdexcept>
#include <algorithm>

namespace epoch_script
{

    DataType TypeChecker::GetNodeOutputType(const std::string& node_id, const std::string& handle)
    {
        // Check tracked output types first
        auto tracked_it = context_.node_output_types.find(node_id);
        if (tracked_it != context_.node_output_types.end())
        {
            auto handle_it = tracked_it->second.find(handle);
            if (handle_it != tracked_it->second.end())
            {
                return handle_it->second;
            }
        }

        // Check if it's a known node in our algorithms list
        auto node_it = context_.node_lookup.find(node_id);
        if (node_it != context_.node_lookup.end())
        {
            const auto& algo = context_.algorithms[node_it->second];
            // Copy the string to avoid reference invalidation after vector reallocation
            const std::string node_type = algo.type;

            // Check registry for output types
            const auto& all_metadata = context_.GetRegistry().GetMetaData();
            if (all_metadata.contains(node_type))
            {
                const auto& comp_meta = all_metadata.at(node_type);
                for (const auto& output : comp_meta.outputs)
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

    bool TypeChecker::IsTypeCompatible(DataType source, DataType target)
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

        // Numeric type compatibility: Number, Decimal, and Integer are mutually compatible
        // This allows arithmetic operations between different numeric types
        if ((target == DataType::Number || target == DataType::Decimal || target == DataType::Integer) &&
            (source == DataType::Number || source == DataType::Decimal || source == DataType::Integer))
        {
            return true;
        }

        // Otherwise incompatible
        return false;
    }

    std::optional<std::string> TypeChecker::NeedsTypeCast(DataType source, DataType target)
    {
        // If types are already compatible, no cast needed
        if (IsTypeCompatible(source, target))
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

    ValueHandle TypeChecker::InsertTypeCast(const ValueHandle& source, DataType source_type, DataType target_type)
    {
        // Check if casting is needed
        auto cast_method = NeedsTypeCast(source_type, target_type);

        if (!cast_method.has_value())
        {
            // No casting needed - types are compatible
            return source;
        }

        if (cast_method.value() == "bool_to_num")
        {
            // Use boolean_select to convert boolean to number
            // boolean_select(condition, true_value, false_value)
            std::string cast_node_id = UniqueNodeId("bool_to_num_cast");

            epoch_script::strategy::AlgorithmNode cast_algo;
            cast_algo.id = cast_node_id;
            cast_algo.type = "boolean_select";

            // Wire the boolean to condition
            cast_algo.inputs["condition"].push_back(JoinId(source.node_id, source.handle));

            // Create number nodes for true (1) and false (0)
            ValueHandle true_node = MaterializeNumber(1.0);
            ValueHandle false_node = MaterializeNumber(0.0);

            // Wire the numbers to true and false inputs
            cast_algo.inputs["true"].push_back(JoinId(true_node.node_id, true_node.handle));
            cast_algo.inputs["false"].push_back(JoinId(false_node.node_id, false_node.handle));

            // Add to algorithms list
            context_.algorithms.push_back(std::move(cast_algo));
            context_.node_lookup[cast_node_id] = context_.algorithms.size() - 1;

            // Track output type
            context_.node_output_types[cast_node_id]["result"] = DataType::Number;

            return {cast_node_id, "result"};
        }
        else if (cast_method.value() == "num_to_bool")
        {
            // Use neq (not equal) to convert number to boolean (num != 0)
            std::string cast_node_id = UniqueNodeId("num_to_bool_cast");

            epoch_script::strategy::AlgorithmNode cast_algo;
            cast_algo.id = cast_node_id;
            cast_algo.type = "neq";

            // Wire the number to SLOT0
            cast_algo.inputs["SLOT0"].push_back(JoinId(source.node_id, source.handle));

            // Create zero node
            ValueHandle zero_node = MaterializeNumber(0.0);

            // Wire zero to SLOT1
            cast_algo.inputs["SLOT1"].push_back(JoinId(zero_node.node_id, zero_node.handle));

            // Add to algorithms list
            context_.algorithms.push_back(std::move(cast_algo));
            context_.node_lookup[cast_node_id] = context_.algorithms.size() - 1;

            // Track output type
            context_.node_output_types[cast_node_id]["result"] = DataType::Boolean;

            return {cast_node_id, "result"};
        }
        else
        {
            // Incompatible types that can't be cast
            throw std::runtime_error("Type mismatch: Cannot convert " + DataTypeToString(source_type) +
                                     " to " + DataTypeToString(target_type));
        }
    }

    std::string TypeChecker::DataTypeToString(DataType type)
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

    // Private helpers

    ValueHandle TypeChecker::MaterializeNumber(double value)
    {
        std::string node_id = UniqueNodeId("number");

        epoch_script::strategy::AlgorithmNode algo;
        algo.id = node_id;
        algo.type = "number";
        algo.options["value"] = epoch_script::MetaDataOptionDefinition{value};

        context_.algorithms.push_back(std::move(algo));
        context_.node_lookup[node_id] = context_.algorithms.size() - 1;
        context_.var_to_binding[node_id] = "number";
        context_.node_output_types[node_id]["result"] = DataType::Decimal;

        return {node_id, "result"};
    }

    std::string TypeChecker::UniqueNodeId(const std::string& base)
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

    std::string TypeChecker::JoinId(const std::string& node_id, const std::string& handle)
    {
        return node_id + "#" + handle;
    }

} // namespace epoch_script
