//
// Created by adesola on 1/4/25.
//

#pragma once
#include <epoch_script/core/bar_attribute.h>
#include "stdexcept"
#include <vector>

namespace epoch_script::futures {

struct FuturesConstructedBars {
  std::vector<double> o{}, h{}, l{}, c{}, v{}, oi{};
  std::vector<int64_t> t{};
    std::vector<std::string> s{};

  void reserve(int N);

  static FuturesConstructedBars
  constant(std::vector<double> const &closePrices,
           std::vector<int64_t> const &timestamps,
            std::vector<std::string> const & contracts);

  std::vector<double> &operator[](epoch_script::BarAttribute::Type type);

  const std::vector<double> &operator[](epoch_script::BarAttribute::Type type) const;
  bool operator==(const FuturesConstructedBars &other) const = default;

};
} // namespace epoch_script::futures