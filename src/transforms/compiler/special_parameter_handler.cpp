//
// Created by Claude Code
// EpochScript Special Parameter Handler Implementation
//

#include "special_parameter_handler.h"
#include <epoch_core/enum_wrapper.h>
#include <epoch_script/strategy/session_variant.h>
#include <unordered_set>
#include <map>

namespace epoch_script
{

    void SpecialParameterHandler::CanonicalizeTimeframe(
        std::unordered_map<std::string, epoch_script::MetaDataOptionDefinition::T>& params)
    {
        // Validate timeframe is a pandas offset string if provided; do not mutate unless empty
        if (params.contains("timeframe"))
        {
            const auto& tf_val = params.at("timeframe");

            // Check if it's a string
            if (std::holds_alternative<std::string>(tf_val))
            {
                const std::string& tf_str = std::get<std::string>(tf_val);

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
                ThrowError("Parameter 'timeframe' must be a string (pandas offset)");
            }
        }
    }

    void SpecialParameterHandler::CanonicalizeSession(
        std::unordered_map<std::string, epoch_script::MetaDataOptionDefinition::T>& params)
    {
        // Validate session parameter if provided - must be a string literal
        if (params.contains("session"))
        {
            const auto& session_val = params.at("session");

            // Check if it's a string
            if (std::holds_alternative<std::string>(session_val))
            {
                const std::string& session_str = std::get<std::string>(session_val);

                // Skip empty strings - treat as "not specified"
                if (session_str.empty())
                {
                    params.erase("session");
                    return;
                }

                // Validate against predefined sessions
                ValidateSession(session_str);
            }
            else
            {
                ThrowError("Parameter 'session' must be a string literal");
            }
        }
    }

    void SpecialParameterHandler::ApplySpecialFields(
        epoch_script::strategy::AlgorithmNode& algo,
        const std::unordered_map<std::string, epoch_script::MetaDataOptionDefinition::T>& params)
    {
        // Extract timeframe and session from params and set as separate fields
        // These should NOT be added to the options map
        // NOTE: For now, we store as strings. Later we'll extend to support variable references.

        // Handle timeframe as special field (store as string literal for now)
        if (params.contains("timeframe"))
        {
            const auto& tf_value = params.at("timeframe");
            if (std::holds_alternative<std::string>(tf_value))
            {
                const std::string& tf_str = std::get<std::string>(tf_value);
                // Validate timeframe (should already be validated by CanonicalizeTimeframe, but double-check)
                ValidateTimeframe(tf_str);
                // Convert string to TimeFrame object
                algo.timeframe = epoch_script::TimeFrame(tf_str);
            }
            else
            {
                ThrowError("Parameter 'timeframe' must be a string (pandas offset)");
            }
        }

        // Handle session as special field (store as SessionType enum for now)
        if (params.contains("session"))
        {
            const auto& session_value = params.at("session");
            if (std::holds_alternative<std::string>(session_value))
            {
                const std::string& session_str = std::get<std::string>(session_value);
                // Convert string to SessionType using wrapper
                algo.session = epoch_core::SessionTypeWrapper::FromString(session_str);
            }
            else
            {
                ThrowError("Parameter 'session' must be a string literal");
            }
        }
    }

    void SpecialParameterHandler::VerifySessionDependencies()
    {
        // Verify that all nodes with session parameters have corresponding sessions nodes
        // Track required sessions: {(session_val, timeframe_str): [node_ids]}
        std::map<std::pair<std::string, std::optional<std::string>>, std::vector<std::string>> required_sessions;

        // Scan all nodes for session fields
        for (const auto& node : context_.algorithms)
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
        for (const auto& [key, node_ids] : required_sessions)
        {
            const auto& [session_val, timeframe_str] = key;

            // Check if a matching sessions node exists
            bool has_sessions_node = false;
            for (const auto& node : context_.algorithms)
            {
                if (node.type == "sessions")
                {
                    // Check if this sessions node matches
                    bool session_matches = false;
                    if (node.options.contains("session_type"))
                    {
                        const auto& opt_val = node.options.at("session_type").GetVariant();
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

                epoch_script::strategy::AlgorithmNode sessions_algo;
                sessions_algo.id = synthetic_id;
                sessions_algo.type = "sessions";
                sessions_algo.options["session_type"] = epoch_script::MetaDataOptionDefinition{session_val};
                if (timeframe_str.has_value())
                {
                    sessions_algo.timeframe = epoch_script::TimeFrame(*timeframe_str);
                }

                context_.algorithms.push_back(std::move(sessions_algo));
                context_.node_lookup[synthetic_id] = context_.algorithms.size() - 1;
            }
        }
    }

    void SpecialParameterHandler::ValidateTimeframe(const std::string& timeframe)
    {
        // Validate timeframe is a non-empty string (pandas offset)
        // Without pandas, we accept any non-empty string (similar to Python fallback)
        if (timeframe.empty())
        {
            ThrowError("Parameter 'timeframe' must be a non-empty string (pandas offset)");
        }
    }

    void SpecialParameterHandler::ValidateSession(const std::string& session)
    {
        // Validate session against predefined session types
        static const std::unordered_set<std::string> valid_sessions = {
            "Sydney", "Tokyo", "London", "NewYork",
            "AsianKillZone", "LondonOpenKillZone", "NewYorkKillZone", "LondonCloseKillZone"
        };

        if (session.empty())
        {
            ThrowError("Parameter 'session' must be a non-empty string");
        }

        if (valid_sessions.find(session) == valid_sessions.end())
        {
            ThrowError("Invalid session '" + session + "'. Must be one of: " +
                       "AsianKillZone, London, LondonCloseKillZone, LondonOpenKillZone, " +
                       "NewYork, NewYorkKillZone, Sydney, Tokyo");
        }
    }

    void SpecialParameterHandler::ThrowError(const std::string& msg)
    {
        throw std::runtime_error(msg);
    }

} // namespace epoch_script
