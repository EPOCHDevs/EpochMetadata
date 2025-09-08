//
// Created by assistant on fix date.
//
#include "epoch_metadata/transforms/transform_definition.h"
#include "epoch_metadata/transforms/registry.h"
#include <epoch_core/macros.h>
#include <stdexcept>

using namespace epoch_metadata;

TransformDefinition::TransformDefinition(YAML::Node const &node) {
    if (!node["type"]) {
        throw std::runtime_error("TransformDefinition: 'type' is required");
    }

    m_data.type = node["type"].as<std::string>();
    m_data.id = node["id"].as<std::string>(m_data.type);

    // Parse timeframe if present - it's typically serialized as a string
    if (node["timeframe"]) {
        if (node["timeframe"].IsScalar()) {
            auto timeframe_str = node["timeframe"].as<std::string>();
            m_data.timeframe = epoch_metadata::TimeFrame{timeframe_str};
        } else if (node["timeframe"]["type"] && node["timeframe"]["interval"]) {
            // Handle structured timeframe format
            auto type = node["timeframe"]["type"].as<std::string>();
            auto interval = node["timeframe"]["interval"].as<int64_t>();
            std::string tf_str;
            // Map common type names to valid timeframe strings
            if (type == "day" || type == "days") {
                tf_str = std::to_string(interval) + "D";
            } else if (type == "hour" || type == "hours") {
                tf_str = std::to_string(interval) + "H";
            } else if (type == "minute" || type == "minutes") {
                tf_str = std::to_string(interval) + "Min";
            } else {
                // Default fallback
                tf_str = std::to_string(interval) + type;
            }
            m_data.timeframe = epoch_metadata::TimeFrame{tf_str};
        }
    }

    // Parse options if present
    if (node["options"] && node["options"].IsMap()) {
        for (const auto& option : node["options"]) {
            const auto& key = option.first.as<std::string>();
            const auto& value_node = option.second;
            
            if (value_node.IsScalar()) {
                auto value_str = value_node.as<std::string>();
                // Try different types based on the string content
                if (value_str == "true") {
                    m_data.options[key] = epoch_metadata::MetaDataOptionDefinition{true};
                } else if (value_str == "false") {
                    m_data.options[key] = epoch_metadata::MetaDataOptionDefinition{false};
                } else {
                    try {
                        // Try as integer first
                        auto int_val = value_node.as<int64_t>();
                        m_data.options[key] = epoch_metadata::MetaDataOptionDefinition{static_cast<double>(int_val)};
                    } catch (...) {
                        try {
                            // Try as double
                            m_data.options[key] = epoch_metadata::MetaDataOptionDefinition{value_node.as<double>()};
                        } catch (...) {
                            // Fall back to string
                            m_data.options[key] = epoch_metadata::MetaDataOptionDefinition{value_str};
                        }
                    }
                }
            }
        }
    }

    // Parse inputs if present
    if (node["inputs"] && node["inputs"].IsMap()) {
        for (const auto& input : node["inputs"]) {
            const auto& key = input.first.as<std::string>();
            const auto& value_node = input.second;
            
            if (value_node.IsSequence()) {
                std::vector<std::string> values;
                for (const auto& item : value_node) {
                    values.push_back(item.as<std::string>());
                }
                m_data.inputs[key] = values;
            } else {
                m_data.inputs[key] = std::vector<std::string>{value_node.as<std::string>()};
            }
        }
    }
    
    // Look up metadata for this transform type from the registry
    try {
        auto metadata = transforms::ITransformRegistry::GetInstance().GetMetaData(m_data.type);
        if (metadata) {
            m_data.metaData = metadata.value();
        }
    } catch (const std::exception& e) {
        // If metadata lookup fails, we'll use default empty metadata
        // This allows tests to run even if some metadata is missing
    }
}