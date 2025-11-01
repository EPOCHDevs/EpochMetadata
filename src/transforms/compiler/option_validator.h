//
// Created by Claude Code
// EpochFlow Option Validator
//
// Validates and parses component options based on metadata.
// Handles type-specific parsing, defaults, clamping, and validation.
//

#pragma once

#include "compilation_context.h"
#include "parser/ast_nodes.h"
#include <epochflow/core/metadata_options.h>
#include <epochflow/transforms/core/metadata.h>
#include <unordered_map>

namespace epochflow
{

    class OptionValidator
    {
    public:
        explicit OptionValidator(CompilationContext& context) : context_(context) {}

        // Validate all options for a node and apply defaults/clamping
        void ValidateAndApplyOptions(
            const std::string& node_id,
            const epochflow::transforms::TransformsMetaData& comp_meta,
            std::unordered_map<std::string, epochflow::MetaDataOptionDefinition::T>& kwargs,
            const Call& call);

        // Parse a single option value based on metadata type
        epochflow::MetaDataOptionDefinition::T ParseOptionByMetadata(
            const epochflow::MetaDataOptionDefinition::T& raw_value,
            const epochflow::MetaDataOption& meta_option,
            const std::string& option_id,
            const std::string& node_id,
            const Call& call,
            const epochflow::transforms::TransformsMetaData& comp_meta);

    private:
        CompilationContext& context_;

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);
    };

} // namespace epochflow
