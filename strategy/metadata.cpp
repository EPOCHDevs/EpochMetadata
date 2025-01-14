//
// Created by dewe on 9/10/24.
//
#include "metadata.h"
#include "../common_utils.h"

using namespace stratifyx::metadata;
using namespace stratifyx::metadata::strategy;

namespace YAML {
    bool convert<AlgorithmBaseMetaData>::decode(YAML::Node const &node, AlgorithmBaseMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options = node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        return true;
    }

    bool convert<AlgorithmMetaData>::decode(YAML::Node const &node, AlgorithmMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options = node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        metadata.isGroup = node["isGroup"].as<bool>(false);
        metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
        return true;
    }

    bool convert<TradeSignalMetaData>::decode(YAML::Node const &node, TradeSignalMetaData &metadata) {
        metadata.id = node["id"].as<std::string>();
        metadata.name = node["name"].as<std::string>("");
        metadata.options = node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
        metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
        metadata.isGroup = node["isGroup"].as<bool>(false);
        metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
        metadata.type = TradeSignalTypeWrapper::FromString(node["type"].as<std::string>());
        metadata.algorithm = YAML::Dump(node["algorithm"]);
        metadata.executor = YAML::Dump(node["executor"]);
        return true;
    }
}