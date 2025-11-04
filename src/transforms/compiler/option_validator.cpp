//
// Created by Claude Code
// EpochScript Option Validator Implementation
//

#include "option_validator.h"
#include <epoch_core/enum_wrapper.h>
#include <epoch_script/core/metadata_options.h>
#include <epoch_script/core/sql_statement.h>
#include <glaze/glaze.hpp>
#include <algorithm>
#include <format>
#include <regex>

namespace epoch_script
{

    // Helper function to trim leading/trailing whitespace from a string
    static std::string TrimWhitespace(const std::string& str)
    {
        const char* whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    void OptionValidator::ValidateAndApplyOptions(
        const std::string& node_id,
        const epoch_script::transforms::TransformsMetaData& comp_meta,
        std::unordered_map<std::string, epoch_script::MetaDataOptionDefinition::T>& kwargs,
        const Call& call)
    {
        // 1. Apply default options for missing required parameters
        for (const auto& meta_option : comp_meta.options)
        {
            if (meta_option.isRequired &&
                !kwargs.contains(meta_option.id) &&
                meta_option.defaultValue.has_value())
            {
                kwargs[meta_option.id] = meta_option.defaultValue->GetVariant();
            }
        }

        // 2. Validate required options are present
        for (const auto& meta_option : comp_meta.options)
        {
            if (meta_option.isRequired && !kwargs.contains(meta_option.id))
            {
                std::string suggestion = meta_option.defaultValue
                                             ? meta_option.defaultValue->ToString()
                                             : "required";

                ThrowError(
                    std::format("Node '{}' of type '{}' is missing required option '{}'. "
                                "Add option '{}' with type {}. Suggested value: {}",
                                node_id, comp_meta.name, meta_option.id,
                                meta_option.id,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(meta_option.type),
                                suggestion),
                    call.lineno, call.col_offset);
            }
        }

        // 3. Parse and validate all kwargs based on metadata types
        for (auto& [option_id, option_value] : kwargs)
        {
            // Skip special parameters (timeframe and session) - they're handled separately
            if (option_id == "timeframe" || option_id == "session")
            {
                continue;
            }

            // Find metadata for this option
            auto meta_option_it = std::find_if(
                comp_meta.options.begin(), comp_meta.options.end(),
                [&](const auto& opt) { return opt.id == option_id; });

            if (meta_option_it == comp_meta.options.end())
            {
                ThrowError(
                    std::format("Unknown option '{}' for node '{}' of type '{}'. "
                                "Remove option '{}' or check if you meant a different option name",
                                option_id, node_id, comp_meta.name, option_id),
                    call.lineno, call.col_offset);
            }

            const auto& meta_option = *meta_option_it;

            // Parse value based on metadata type
            option_value = ParseOptionByMetadata(option_value, meta_option, option_id, node_id, call, comp_meta);
        }

        // kwargs now contains validated, clamped values with defaults applied
    }

    epoch_script::MetaDataOptionDefinition::T OptionValidator::ParseOptionByMetadata(
        const epoch_script::MetaDataOptionDefinition::T& raw_value,
        const epoch_script::MetaDataOption& meta_option,
        const std::string& option_id,
        const std::string& node_id,
        const Call& call,
        const epoch_script::transforms::TransformsMetaData& comp_meta)
    {
        using MetaType = epoch_core::MetaDataOptionType;

        // Handle each metadata type
        switch (meta_option.type)
        {
        case MetaType::Integer:
        case MetaType::Decimal:
        {
            // Expect a numeric value
            if (!std::holds_alternative<double>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects type {} but got non-numeric value",
                                option_id, node_id,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(meta_option.type)),
                    call.lineno, call.col_offset);
            }

            double numeric_value = std::get<double>(raw_value);

            // Clamp to min/max bounds
            double clamped_value = std::max(meta_option.min, std::min(meta_option.max, numeric_value));

            return clamped_value;
        }

        case MetaType::Boolean:
        {
            // Expect a boolean value
            if (!std::holds_alternative<bool>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects Boolean but got non-boolean value",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }
            return raw_value;
        }

        case MetaType::String:
        case MetaType::Select:
        {
            // Expect a string value
            if (!std::holds_alternative<std::string>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects String but got non-string value",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }

            // For Select type, validate against allowed options
            if (meta_option.type == MetaType::Select && !meta_option.selectOption.empty())
            {
                const std::string& str_value = std::get<std::string>(raw_value);
                bool is_valid = false;
                for (const auto& option : meta_option.selectOption)
                {
                    if (option.value == str_value)
                    {
                        is_valid = true;
                        break;
                    }
                }

                if (!is_valid)
                {
                    std::string valid_options;
                    for (size_t i = 0; i < meta_option.selectOption.size(); ++i)
                    {
                        valid_options += meta_option.selectOption[i].value;
                        if (i < meta_option.selectOption.size() - 1)
                            valid_options += ", ";
                    }

                    ThrowError(
                        std::format("Option '{}' of node '{}' has invalid value '{}'. Valid options: {}",
                                    option_id, node_id, str_value, valid_options),
                        call.lineno, call.col_offset);
                }
            }

            return raw_value;
        }

        case MetaType::EventMarkerSchema:
        {
            // If already parsed as EventMarkerSchema, validate and return
            if (std::holds_alternative<epoch_script::EventMarkerSchema>(raw_value))
            {
                auto schema = std::get<epoch_script::EventMarkerSchema>(raw_value);

                // Validate SLOT syntax in select_key
                static const std::regex slot_pattern(R"(SLOT\d+)");
                if (!std::regex_match(schema.select_key, slot_pattern))
                {
                    ThrowError(
                        std::format("EventMarkerSchema field 'select_key' must use SLOT syntax (SLOT0, SLOT1, etc.). "
                                    "Found: '{}'. String references like 'signal#result' are not supported.",
                                    schema.select_key),
                        call.lineno, call.col_offset);
                }

                // Validate SLOT syntax in all column_id fields
                for (const auto& col_schema : schema.schemas)
                {
                    if (!std::regex_match(col_schema.column_id, slot_pattern))
                    {
                        ThrowError(
                            std::format("EventMarkerSchema CardColumnSchema 'column_id' must use SLOT syntax (SLOT0, SLOT1, etc.). "
                                        "Found: '{}'. String references like 'signal#result' are not supported.",
                                        col_schema.column_id),
                            call.lineno, call.col_offset);
                    }
                }

                return raw_value;
            }

            // Expect a JSON string to parse into EventMarkerSchema
            if (!std::holds_alternative<std::string>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects EventMarkerSchema (JSON string) but got non-string value",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }

            const std::string& json_str = std::get<std::string>(raw_value);
            // Trim leading/trailing whitespace (triple-quoted strings may have newlines)
            std::string trimmed_json = TrimWhitespace(json_str);

            // Parse as EventMarkerSchema
            auto filter_result = glz::read_json<epoch_script::EventMarkerSchema>(trimmed_json);
            if (!filter_result)
            {
                ThrowError(
                    std::format("Invalid EventMarkerSchema JSON for option '{}' of node '{}'. "
                                "EventMarkerSchema must contain 'select_key' field.",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }

            auto schema = filter_result.value();

            // Validate SLOT syntax in select_key
            static const std::regex slot_pattern(R"(SLOT\d+)");
            if (!std::regex_match(schema.select_key, slot_pattern))
            {
                ThrowError(
                    std::format("EventMarkerSchema field 'select_key' must use SLOT syntax (SLOT0, SLOT1, etc.). "
                                "Found: '{}'. String references like 'signal#result' are not supported.",
                                schema.select_key),
                    call.lineno, call.col_offset);
            }

            // Validate SLOT syntax in all column_id fields
            for (const auto& col_schema : schema.schemas)
            {
                if (!std::regex_match(col_schema.column_id, slot_pattern))
                {
                    ThrowError(
                        std::format("EventMarkerSchema CardColumnSchema 'column_id' must use SLOT syntax (SLOT0, SLOT1, etc.). "
                                    "Found: '{}'. String references like 'signal#result' are not supported.",
                                    col_schema.column_id),
                        call.lineno, call.col_offset);
                }
            }

            return epoch_script::MetaDataOptionDefinition::T{schema};
        }

        case MetaType::SqlStatement:
        {
            // If already parsed as SqlStatement, return as-is (already validated)
            if (std::holds_alternative<epoch_script::SqlStatement>(raw_value))
            {
                return raw_value;
            }

            // Expect a string (SQL query) that will be validated by SqlStatement constructor
            if (!std::holds_alternative<std::string>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects SqlStatement (string) but got non-string value",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }

            const std::string& sql_str = std::get<std::string>(raw_value);

            try
            {
                // Construct SqlStatement and validate with numOutputs
                epoch_script::SqlStatement sql_stmt{sql_str};
                int num_outputs = static_cast<int>(comp_meta.outputs.size());
                sql_stmt.Validate(num_outputs);
                return epoch_script::MetaDataOptionDefinition::T{sql_stmt};
            }
            catch (const std::exception& e)
            {
                ThrowError(
                    std::format("Option '{}' of node '{}': {}", option_id, node_id, e.what()),
                    call.lineno, call.col_offset);
            }
        }

        case MetaType::Time:
        {
            // If already a Time object (from constructor), return it
            if (std::holds_alternative<epoch_frame::Time>(raw_value))
            {
                return raw_value;
            }
            // Expect a string that can be parsed into Time
            else if (std::holds_alternative<std::string>(raw_value))
            {
                // Parse string into Time object
                const std::string& time_str = std::get<std::string>(raw_value);
                try
                {
                    epoch_frame::Time time = epoch_script::TimeFromString(time_str);
                    return epoch_script::MetaDataOptionDefinition::T{time};
                }
                catch (const std::exception& e)
                {
                    ThrowError(
                        std::format("Option '{}' of node '{}' has invalid Time format: {}. Error: {}",
                                    option_id, node_id, time_str, e.what()),
                        call.lineno, call.col_offset);
                }
            }
            else
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects Time (constructor or string) but got invalid type",
                                option_id, node_id),
                    call.lineno, call.col_offset);
            }
        }

        case MetaType::NumericList:
        case MetaType::StringList:
        {
            // Expect a Sequence
            if (!std::holds_alternative<epoch_script::Sequence>(raw_value))
            {
                ThrowError(
                    std::format("Option '{}' of node '{}' expects {} but got non-list value",
                                option_id, node_id,
                                epoch_core::MetaDataOptionTypeWrapper::ToString(meta_option.type)),
                    call.lineno, call.col_offset);
            }

            const auto& sequence = std::get<epoch_script::Sequence>(raw_value);

            // Validate sequence elements match expected type
            for (const auto& item : sequence)
            {
                if (meta_option.type == MetaType::NumericList && !std::holds_alternative<double>(item))
                {
                    ThrowError(
                        std::format("Option '{}' of node '{}' expects NumericList but contains non-numeric values",
                                    option_id, node_id),
                        call.lineno, call.col_offset);
                }
                else if (meta_option.type == MetaType::StringList && !std::holds_alternative<std::string>(item))
                {
                    ThrowError(
                        std::format("Option '{}' of node '{}' expects StringList but contains non-string values",
                                    option_id, node_id),
                        call.lineno, call.col_offset);
                }
            }

            return raw_value;
        }

        default:
            ThrowError(
                std::format("Unsupported metadata option type: {}",
                            epoch_core::MetaDataOptionTypeWrapper::ToString(meta_option.type)),
                call.lineno, call.col_offset);
        }

        return raw_value; // Unreachable, but keeps compiler happy
    }

    void OptionValidator::ThrowError(const std::string& msg, int line, int col)
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

} // namespace epoch_script
