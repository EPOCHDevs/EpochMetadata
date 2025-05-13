
#pragma once
#include <algorithm>
#include <cctype>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>

inline std::string beautify(std::string const &id) {
  auto view = std::views::split(id, "_") |
              std::views::transform([](auto const &part) {
                std::string part_string{part.begin(), part.end()};
                part_string[0] = static_cast<char>(
                    std::toupper(static_cast<unsigned char>(part_string[0])));
                std::ranges::transform(part_string | std::views::drop(1),
                                       part_string.begin() + 1, tolower);
                return part_string;
              }) |
              std::views::join_with(' ');

  std::stringstream output;
  std::ranges::copy(view, std::ostream_iterator<char>(output));
  return output.str();
};