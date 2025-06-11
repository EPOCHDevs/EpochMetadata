//
// Created by adesola on 12/28/24.
//

#pragma once
#include <epoch_core/common_utils.h>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include "epoch_metadata/constants.h"

namespace epoch_metadata {

  template <class T, class B = T>
std::vector<T> LoadFromYAMLNode(YAML::Node const &node) {
    std::vector<T> result(node.size());
    std::ranges::transform(node, result.begin(), [](auto const &item) {
      auto value = item.second;
      value["id"] = item.first;
      return static_cast<T>(value.template as<B>());
    });
    return result;
  }

  template <class T, class B = T>
  std::vector<T> LoadFromFile(FileLoaderInterface const &loader,
                              std::string const &name) {
    return LoadFromYAMLNode<T, B>(loader(std::vformat("{}.yaml", std::format_args(std::make_format_args(name)))));
  }


inline std::string MakeBarChartURL(std::string const &indicator) {
  return std::format(
      "https://www.barchart.com/education/technical-indicators/{}", indicator);
}

inline std::string MakeQuantpediaURL(std::string const &indicator) {
  return std::format("https://www.quantpedia.com/{}", indicator);
}

inline std::string MakeInvestopediaURL(std::string const &indicator) {
  return std::format("https://www.investopedia.com/terms/{}/{}.asp",
                     indicator.at(0), indicator);
}

inline std::string MakeWikipediaURL(std::string const &indicator) {
  return std::format("https://en.wikipedia.org/wiki/{}", indicator);
}

inline std::string MakeStockChartURL(std::string const &indicator) {
  return std::format(
      "https://chartschool.stockcharts.com/table-of-contents/"
      "technical-indicators-and-overlays/technical-indicators/{}",
      indicator);
}

inline std::string MakeDescLink(std::string const &arg) {
  static std::unordered_map<std::string, std::string (*)(std::string const &)>
      DESC_PLACEHOLDER_CONVERTER_MAP{{"BAR_CHART_URL", MakeBarChartURL},
                                     {"WIKIPEDIA", MakeWikipediaURL},
                                     {"STOCK_CHART", MakeStockChartURL},
                                     {"INVESTOPEDIA", MakeInvestopediaURL},
                                     {"QUANTPEDIA", MakeQuantpediaURL}};

  if (arg.empty() || !arg.starts_with("$")) {
    return arg;
  }
  auto split = arg.find('/');
  AssertFromStream(
      split != std::string::npos,
      "desc starting with $ must be in form $PLACEHOLDER/key: got: " << arg);

  auto placeholder = arg.substr(1, split - 1);
  return epoch_core::lookup(DESC_PLACEHOLDER_CONVERTER_MAP,
                            placeholder)(arg.substr(split + 1));
}
} // namespace epoch_metadata