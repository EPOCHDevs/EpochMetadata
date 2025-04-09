//
// Created by adesola on 4/8/25.
//

#pragma once
#include <functional>
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata {
    constexpr auto ARG = "*";
    constexpr auto ARG0 = "*0";
    constexpr auto ARG1 = "*1";
    constexpr auto ARG2 = "*2";
    constexpr auto ARG3 = "*3";

    using FileLoaderInterface = std::function<YAML::Node(std::string const&)>;
} // namespace epoch_metadata