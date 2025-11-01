//
// Created by Claude Code
// EpochFlow Option Validator
//
// Validates and parses component options based on metadata.
// Handles type-specific parsing, defaults, clamping, and validation.
//

#pragma once

#include "compilation_context.h"
#include "../parser/ast_nodes.h"
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/transforms/metadata.h>
#include <unordered_map>

namespace epoch_stratifyx::epochflow
{

    class OptionValidator
    {
    public:
        explicit OptionValidator(CompilationContext& context) : context_(context) {}

        // Validate all options for a node and apply defaults/clamping
        void ValidateAndApplyOptions(
            const std::string& node_id,
            const epoch_metadata::transforms::TransformsMetaData& comp_meta,
            std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T>& kwargs,
            const Call& call);

        // Parse a single option value based on metadata type
        epoch_metadata::MetaDataOptionDefinition::T ParseOptionByMetadata(
            const epoch_metadata::MetaDataOptionDefinition::T& raw_value,
            const epoch_metadata::MetaDataOption& meta_option,
            const std::string& option_id,
            const std::string& node_id,
            const Call& call,
            const epoch_metadata::transforms::TransformsMetaData& comp_meta);

    private:
        CompilationContext& context_;

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);
    };

} // namespace epoch_stratifyx::epochflow
