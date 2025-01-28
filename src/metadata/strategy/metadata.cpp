//
// Created by dewe on 9/10/24.
//
#include "metadata.h"
#include "../doc_deserialization_helper.h"


using namespace metadata;
using namespace metadata::strategy;

namespace YAML {
    bool convert<AlgorithmNode>::decode(YAML::Node const &node,
                                        AlgorithmNode &metadata) {

        metadata.type = node["type"].as<std::string>();
        metadata.id = node["id"].as<std::string>(metadata.type);

        auto transform = metadata::transforms::ITransformRegistry::GetInstance().GetMetaData(metadata.type);
        if (!transform) {
            throw std::runtime_error("Unknown transform type: " + metadata.type);
        }

        auto options = node["options"];
        if (!options && transform->options.size() > 0) {
            throw std::runtime_error(fmt::format("Missing options for transform {}", metadata.type));
        }

        for (auto const &option: transform->options) {
            auto arg = options[option.id];
            if (option.isRequired && !arg) {
                throw std::runtime_error("Missing required option: " + option.id + " for transform " + metadata.type);
            }

            auto serialized = YAML::Dump(arg);
            if (serialized.starts_with(".")) {
                metadata.options.emplace(option.id, MetaDataOptionDefinition{MetaDataArgRef{serialized.substr(1)}});
            } else {
                metadata.options.emplace(option.id, CreateMetaDataArgDefinition(arg, option));
            }
            options.remove(option.id);
        }

        if (options && options.size() != 0) {
            throw std::runtime_error("Unknown options: " + Dump(options));
        }

        metadata.inputs = node["desc"].as<InputMapping>(InputMapping{});
        metadata.metaData = *transform;
        return true;
    }

    bool convert<AlgorithmBaseMetaData>::decode(YAML::Node const &node,
                                                AlgorithmBaseMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options =
                node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        return true;
    }

    bool convert<AlgorithmMetaData>::decode(YAML::Node const &node,
                                            AlgorithmMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options =
                node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        metadata.isGroup = node["isGroup"].as<bool>(false);
        metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
        return true;
    }

    bool convert<TradeSignalMetaData>::decode(YAML::Node const &node,
                                              TradeSignalMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options =
                node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        metadata.isGroup = node["isGroup"].as<bool>(false);
        metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
        metadata.type =
                TradeSignalTypeWrapper::FromString(node["type"].as<std::string>());

        metadata.algorithm = node["algorithm"].as<std::vector<AlgorithmNode>>();
        metadata.executor = node["executor"].as<AlgorithmNode>();
        return true;
    }
}