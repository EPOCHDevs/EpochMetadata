//
// Created by Claude Code
// EpochFlow Constructor Parser Implementation
//

#include "constructor_parser.h"
#include "expression_compiler.h"
#include "option_validator.h"
#include <algorithm>
#include <format>

namespace epoch_stratifyx::epochflow
{

    bool ConstructorParser::IsConstructorCall(const Expr& expr)
    {
        // Must have at least one Call in the chain
        if (auto* call = dynamic_cast<const Call*>(&expr))
        {
            const Expr* cur = call;
            while (auto* call_node = dynamic_cast<const Call*>(cur))
            {
                cur = call_node->func.get();
            }
            // Base must be a Name (component name)
            return dynamic_cast<const Name*>(cur) != nullptr;
        }
        return false;
    }

    ConstructorParseResult ConstructorParser::ParseConstructorAndFeeds(const Call& call)
    {
        // Collect all calls in the chain
        std::vector<const Call*> calls;
        const Expr* cur = &call;
        while (auto* call_node = dynamic_cast<const Call*>(cur))
        {
            calls.push_back(call_node);
            cur = call_node->func.get();
        }

        // Base must be a Name
        auto* name_node = dynamic_cast<const Name*>(cur);
        if (!name_node)
        {
            ThrowError("Right-hand side must be a constructor call (e.g., ema(...)(...))", call.lineno, call.col_offset);
        }

        std::string ctor_name = name_node->id;
        std::reverse(calls.begin(), calls.end());

        // Get component metadata for option parsing
        if (!context_.HasComponent(ctor_name))
        {
            ThrowError("Unknown component '" + ctor_name + "'", call.lineno, call.col_offset);
        }
        const auto& comp_meta = context_.GetComponentMetadata(ctor_name);

        // Build metadata lookup map for O(1) lookups
        std::unordered_map<std::string, epoch_metadata::MetaDataOption> option_metadata;
        for (const auto& opt : comp_meta.options)
        {
            option_metadata[opt.id] = opt;
        }

        ConstructorParseResult result;
        result.ctor_name = ctor_name;

        // Parse constructor kwargs from first call
        for (const auto& [key, value_expr] : calls[0]->keywords)
        {
            // Skip special parameters (timeframe and session) - validated separately
            if (key == "timeframe" || key == "session")
            {
                // Extract raw string value (validated later by canonicalizeTimeframe/Session)
                if (auto* constant = dynamic_cast<const Constant*>(value_expr.get()))
                {
                    if (std::holds_alternative<std::string>(constant->value))
                    {
                        result.ctor_kwargs[key] = std::get<std::string>(constant->value);
                    }
                    else
                    {
                        ThrowError(
                            std::format("Parameter '{}' must be a string", key),
                            calls[0]->lineno, calls[0]->col_offset);
                    }
                }
                else if (auto* name = dynamic_cast<const Name*>(value_expr.get()))
                {
                    // Bare identifier like sessions(session=London)
                    result.ctor_kwargs[key] = name->id;
                }
                else
                {
                    ThrowError(
                        std::format("Parameter '{}' must be a string literal", key),
                        calls[0]->lineno, calls[0]->col_offset);
                }
                continue;
            }

            // Look up metadata for this option (throw error if not found - invalid option)
            auto it = option_metadata.find(key);
            if (it == option_metadata.end())
            {
                ThrowError(
                    std::format("Unknown option '{}' for component '{}'", key, ctor_name),
                    calls[0]->lineno, calls[0]->col_offset);
            }

            result.ctor_kwargs[key] = ParseLiteralOrPrimitive(*value_expr, it->second, comp_meta);
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
                for (const auto& arg_expr : calls[0]->args)
                {
                    feed_step_args.push_back(expr_compiler_.VisitExpr(*arg_expr));
                }
                result.feed_steps.push_back({feed_step_args, feed_step_kwargs});
            }
            else
            {
                ThrowError("Positional constructor arguments not supported; use keyword args", calls[0]->lineno, calls[0]->col_offset);
            }
        }

        // Parse subsequent feed steps
        for (size_t i = 1; i < calls.size(); ++i)
        {
            std::vector<ValueHandle> args;
            std::unordered_map<std::string, ValueHandle> kwargs;

            for (const auto& arg_expr : calls[i]->args)
            {
                args.push_back(expr_compiler_.VisitExpr(*arg_expr));
            }

            for (const auto& [key, value_expr] : calls[i]->keywords)
            {
                kwargs[key] = expr_compiler_.VisitExpr(*value_expr);
            }

            result.feed_steps.push_back({args, kwargs});
        }

        return result;
    }

    epoch_metadata::MetaDataOptionDefinition::T ConstructorParser::ParseLiteralOrPrimitive(
        const Expr& expr,
        const epoch_metadata::MetaDataOption& meta_option,
        const epoch_metadata::transforms::TransformsMetaData& comp_meta)
    {
        // Extract raw value from AST expression
        epoch_metadata::MetaDataOptionDefinition::T raw_value;

        if (auto* constant = dynamic_cast<const Constant*>(&expr))
        {
            raw_value = std::visit([](auto&& value) -> epoch_metadata::MetaDataOptionDefinition::T
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
                throw std::runtime_error("Unsupported constant type");
            }, constant->value);
        }
        else if (auto* name = dynamic_cast<const Name*>(&expr))
        {
            // Check if this name is bound to a constant value
            auto it = context_.var_to_binding.find(name->id);
            if (it != context_.var_to_binding.end() && it->second.find('.') != std::string::npos)
            {
                std::string node_id = it->second.substr(0, it->second.find('.'));

                // Check if it's a literal node (use node_lookup and algorithms)
                if (context_.node_lookup.contains(node_id))
                {
                    const auto& algo = context_.algorithms[context_.node_lookup[node_id]];
                    if (algo.type == "number")
                    {
                        if (algo.options.contains("value"))
                        {
                            raw_value = algo.options.at("value").GetVariant();
                        }
                        else
                        {
                            ThrowError("Number node missing value option");
                        }
                    }
                    else if (algo.type == "bool_true")
                    {
                        raw_value = true;
                    }
                    else if (algo.type == "bool_false")
                    {
                        raw_value = false;
                    }
                    else
                    {
                        ThrowError("Only literal values supported for options");
                    }
                }
                else
                {
                    ThrowError("Only literal values supported for options");
                }
            }
            else
            {
                // Fallback: accept bare identifiers as strings (e.g., sessions(session=London))
                raw_value = name->id;
            }
        }
        else
        {
            ThrowError("Only literal keyword values supported");
        }

        // Delegate to OptionValidator for type-aware parsing
        // Create a dummy call for error reporting (real error locations come from the caller)
        auto dummy_func = std::make_unique<Name>("dummy");
        Call dummy_call{std::move(dummy_func)};
        dummy_call.lineno = 0;
        dummy_call.col_offset = 0;

        OptionValidator validator(context_);
        return validator.ParseOptionByMetadata(raw_value, meta_option, meta_option.id, comp_meta.id, dummy_call, comp_meta);
    }

    void ConstructorParser::ThrowError(const std::string& msg, int line, int col)
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

} // namespace epoch_stratifyx::epochflow
