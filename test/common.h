//
// Created by adesola on 4/9/25.
//

#pragma once
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata {
    const auto DEFAULT_YAML_LOADER  = [](std::string const& _path) {
        return YAML::LoadFile(std::filesystem::path{METADATA_FILES_DIR} / _path);
    };

    const auto LoadAIGeneratedStrategies  = [] {
        std::vector<std::string> buffers;

        auto dir = std::filesystem::path{METADATA_FILES_DIR} / "ai_generated_strategies";
       auto directories = std::filesystem::directory_iterator(dir);
        buffers.reserve(std::ranges::distance(directories));

        directories = std::filesystem::directory_iterator(dir);
        for (auto const& jsonFile : directories) {
            // 1) Open the file
            std::ifstream in{jsonFile.path(), std::ios::binary};
            if (!in) {
                throw std::runtime_error{"Failed to open " + jsonFile.path().string() + " for reading."};
            }

            // 2) Read entire contents into a string
            std::string json;
            json.assign(
              std::istreambuf_iterator{in},
              std::istreambuf_iterator<char>{}
            );
            buffers.emplace_back(json);
        }
        return buffers;
    };

}