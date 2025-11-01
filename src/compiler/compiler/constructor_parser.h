//
// Created by Claude Code
// EpochFlow Constructor Parser
//
// Parses constructor calls and feed chains from AST.
// Handles both named constructors with options and feed operator chains.
//

#pragma once

#include "compilation_context.h"
#include "../parser/ast_nodes.h"
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/transforms/metadata.h>
#include <unordered_map>
#include <vector>

namespace epoch_stratifyx::epochflow
{

    // Forward declaration
    class ExpressionCompiler;

    // Result of parsing a constructor call chain
    struct ConstructorParseResult
    {
        std::string ctor_name;
        std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> ctor_kwargs;
        std::vector<std::pair<std::vector<ValueHandle>, std::unordered_map<std::string, ValueHandle>>> feed_steps;
    };

    class ConstructorParser
    {
    public:
        ConstructorParser(CompilationContext& context, ExpressionCompiler& expr_compiler)
            : context_(context), expr_compiler_(expr_compiler) {}

        // Check if an expression is a constructor call
        bool IsConstructorCall(const Expr& expr);

        // Parse constructor and feed chain from a Call node
        ConstructorParseResult ParseConstructorAndFeeds(const Call& call);

        // Parse a literal or primitive value for use as an option
        epoch_metadata::MetaDataOptionDefinition::T ParseLiteralOrPrimitive(
            const Expr& expr,
            const epoch_metadata::MetaDataOption& meta_option,
            const epoch_metadata::transforms::TransformsMetaData& comp_meta);

    private:
        CompilationContext& context_;
        ExpressionCompiler& expr_compiler_;

        // Error reporting helper
        [[noreturn]] void ThrowError(const std::string& msg, int line = 0, int col = 0);

        // Custom type constructor parsers
        // These correspond to types in MetaDataOptionDefinition::T variant
        epoch_frame::Time ParseTimeConstructor(const Call& call);
        epoch_metadata::CardSchemaFilter ParseCardSchemaFilterConstructor(const Call& call);
        epoch_metadata::CardSchemaSQL ParseCardSchemaSQLConstructor(const Call& call);
        epoch_metadata::SqlStatement ParseSqlStatementConstructor(const Call& call);
        epoch_metadata::CardColumnSchema ParseCardColumnSchemaConstructor(const Call& call);
        // Note: SessionRange and TimeFrame are handled as special parameters, not regular options

        // Helper to convert Call kwargs to glz::generic for deserialization
        glz::generic CallKwargsToGeneric(const Call& call);
    };

} // namespace epoch_stratifyx::epochflow
