//
// Created by Claude Code
// EpochScript Type Checker
//
// Handles type checking and type casting for node connections.
// Manages data type compatibility and automatic type conversions.
//

#pragma once

#include "compilation_context.h"
#include "parser/ast_nodes.h"
#include <optional>
#include <string>

namespace epoch_script
{

    class TypeChecker
    {
    public:
        explicit TypeChecker(CompilationContext& context) : context_(context) {}

        // Get the output type of a node's handle
        DataType GetNodeOutputType(const std::string& node_id, const std::string& handle);

        // Check if source type is compatible with target type
        bool IsTypeCompatible(DataType source, DataType target);

        // Determine if type cast is needed and which cast to use
        // Returns: std::nullopt if no cast needed, "bool_to_num", "num_to_bool", or "incompatible"
        std::optional<std::string> NeedsTypeCast(DataType source, DataType target);

        // Insert a type cast node and return the casted value handle
        ValueHandle InsertTypeCast(const ValueHandle& source, DataType source_type, DataType target_type);

        // Convert DataType enum to human-readable string
        static std::string DataTypeToString(DataType type);

    private:
        CompilationContext& context_;

        // Helper to create number literal nodes for casting
        ValueHandle MaterializeNumber(double value);

        // Helper to generate unique node ID
        std::string UniqueNodeId(const std::string& base);

        // Helper to create "node_id#handle" format
        std::string JoinId(const std::string& node_id, const std::string& handle);
    };

} // namespace epoch_script
