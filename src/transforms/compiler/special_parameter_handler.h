//
// Created by Claude Code
// EpochFlow Special Parameter Handler
//
// Handles special parameters like 'timeframe' and 'session'.
// Validates, canonicalizes, and applies these parameters to algorithm nodes.
//

#pragma once

#include "compilation_context.h"
#include <epochflow/strategy/metadata.h>
#include <epochflow/core/metadata_options.h>
#include <unordered_map>

namespace epochflow
{

    class SpecialParameterHandler
    {
    public:
        explicit SpecialParameterHandler(CompilationContext& context) : context_(context) {}

        // Validate and canonicalize timeframe parameter
        void CanonicalizeTimeframe(std::unordered_map<std::string, epochflow::MetaDataOptionDefinition::T>& params);

        // Validate and canonicalize session parameter
        void CanonicalizeSession(std::unordered_map<std::string, epochflow::MetaDataOptionDefinition::T>& params);

        // Apply special fields (timeframe and session) to an AlgorithmNode
        void ApplySpecialFields(
            epochflow::strategy::AlgorithmNode& algo,
            const std::unordered_map<std::string, epochflow::MetaDataOptionDefinition::T>& params);

        // Verify that all nodes with session parameters have corresponding sessions nodes
        // Auto-creates missing sessions nodes if needed
        void VerifySessionDependencies();

        // Validate timeframe string format
        void ValidateTimeframe(const std::string& timeframe);

        // Validate session string against predefined session types
        void ValidateSession(const std::string& session);

    private:
        CompilationContext& context_;

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg);
    };

} // namespace epochflow
