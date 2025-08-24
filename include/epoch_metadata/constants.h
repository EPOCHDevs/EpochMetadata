//
// Created by adesola on 4/8/25.
//

#pragma once
#include <filesystem>
#include <functional>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata {
constexpr auto ARG = "SLOT";
constexpr auto ARG0 = "SLOT0";
constexpr auto ARG1 = "SLOT1";
constexpr auto ARG2 = "SLOT2";
constexpr auto ARG3 = "SLOT3";

using FileLoaderInterface = std::function<YAML::Node(std::string const &)>;
using AIGeneratedStrategiesLoader = std::function<std::vector<std::string>()>;
} // namespace epoch_metadata