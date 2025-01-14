//
// Created by adesola on 12/28/24.
//

#pragma once
#include <filesystem>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>
#include <common_utils.h>


namespace metadata {
    constexpr const char *ARG = "*";
    constexpr const char *ARG0 = "*0";
    constexpr const char *ARG1 = "*1";
    constexpr const char *ARG2 = "*2";
    constexpr const char *ARG3 = "*3";

    template<class T, class B=T>
    std::vector<T> LoadFromFile(std::string const &name) {
        auto node = YAML::LoadFile(std::filesystem::path(METADATA_FILES_LOC) / fmt::format("{}.yaml", name));
        std::vector<T> result(node.size());
        std::ranges::transform(node, result.begin(), [](auto const &item) {
            auto value = item.second;
            value["id"] = item.first;
            return static_cast<T>(value.template as<B>());
        });
        return result;
    }

    inline std::string MakeBarChartURL(std::string const &indicator) {
        return fmt::format("https://www.barchart.com/education/technical-indicators/{}", indicator);
    }

    inline std::string MakeQuantpediaURL(std::string const &indicator) {
        return fmt::format("https://www.quantpedia.com/{}", indicator);
    }

    inline std::string MakeInvestopediaURL(std::string const &indicator) {
        return fmt::format("https://www.investopedia.com/terms/{}/{}.asp", indicator.at(0), indicator);
    }

    inline std::string MakeWikipediaURL(std::string const &indicator) {
        return fmt::format("https://en.wikipedia.org/wiki/{}", indicator);
    }

    inline std::string MakeStockChartURL(std::string const &indicator) {
        return fmt::format(
                "https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/{}",
                indicator);
    }

    inline std::string MakeDescLink( std::string const& arg) {
        static std::unordered_map<std::string, std::string(*)(std::string const &)> DESC_PLACEHOLDER_CONVERTER_MAP{
                {"BAR_CHART_URL", MakeBarChartURL},
                {"WIKIPEDIA",     MakeWikipediaURL},
                {"STOCK_CHART",   MakeStockChartURL},
                {"INVESTOPEDIA",  MakeInvestopediaURL},
                {"QUANTPEDIA",  MakeQuantpediaURL}
        };

        if (arg.empty() || !arg.starts_with("$")) {
            return arg;
        }
        auto split = arg.find('/');
        AssertWithTraceFromStream(split != std::string::npos, "desc starting with $ must be in form $PLACEHOLDER/key: got: " << arg);

        auto placeholder = arg.substr(1, split-1);
        return stratifyx::lookup(DESC_PLACEHOLDER_CONVERTER_MAP, placeholder)(arg.substr(split + 1));
    }
}