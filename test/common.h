//
// Created by adesola on 4/9/25.
//

#pragma once
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata {
    const auto DEFAULT_YAML_LOADER  = [](std::string const& _path) {
        return YAML::LoadFile(std::filesystem::path{METADATA_FILES_DIR} / _path);
    };

}