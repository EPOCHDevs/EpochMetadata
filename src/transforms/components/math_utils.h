#pragma once
//
// Created by dewe on 4/7/23.
//
#include "cassert"
#include "numeric"
#include "span"
#include "valarray"
#include "vector"

namespace epoch_script::math {

inline std::vector<double> minus(const std::vector<double> &a,
                                 const std::vector<double> &b) {
  std::vector<double> result(std::min(a.size(), b.size()), std::nan(""));
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                 std::minus<double>{});
  return result;
}

inline std::vector<double> plus(const std::vector<double> &a,
                                const std::vector<double> &b) {
  std::vector<double> result(std::min(a.size(), b.size()), std::nan(""));
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                 std::plus<double>{});
  return result;
}

template <typename T>
using Comparator = std::vector<bool> (*)(const std::span<T> &,
                                         const std::vector<T> &);

template <typename T>
std::vector<bool> less(const std::span<T> &a, const std::vector<T> &b) {
  std::vector<bool> result(std::min(a.size(), b.size()), true);
  std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::less<T>{});
  return result;
}

template <typename T>
std::vector<bool> greater(const std::span<T> &a, const std::vector<T> &b) {
  std::vector<bool> result(std::min(a.size(), b.size()), true);
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                 std::greater<T>{});
  return result;
}

inline bool any(std::vector<bool> const &data) {
  return std::ranges::any_of(data, [](bool value) { return value; });
}

template <typename T> std::vector<size_t> nonzero(std::vector<T> const &data) {
  std::vector<size_t> result;
  result.reserve(data.size());

  const auto start = data.begin();
  for (auto it = start; it != data.end(); it++) {
    if (*it > 0) {
      result.push_back(std::distance(start, it));
    }
  }
  return result;
}

inline void bool_and(std::vector<bool> &a, std::vector<bool> const &b) {
  assert(a.size() == b.size());
  std::transform(a.begin(), a.end(), b.begin(), a.begin(),
                 std::logical_and<bool>());
}

template <typename T>
std::vector<bool> boolrelextrema(std::span<T> const &data,
                                 Comparator<T> comparator, int order = 1) {
  assert(order > 1);

  const size_t dataLen = data.size();

  std::valarray<int64_t> locs(dataLen);
  std::iota(std::begin(locs), std::end(locs), 0);

  std::vector<bool> results(dataLen, true);
  std::vector<T> plus(data.begin(), data.end());
  std::vector<T> minus(data.begin(), data.end());
  for (int shift = 1; shift < order + 1; ++shift) {
    std::shift_left(plus.begin(), plus.end(), 1);
    std::shift_right(minus.begin(), minus.end(), 1);
    bool_and(results, comparator(data, plus));
    bool_and(results, comparator(data, minus));

    if (!any(results)) {
      return results;
    }
  }
  return results;
}

template <class T>
std::vector<size_t> argrelmin(std::span<T> const &data, int order = 1) {
  return nonzero(boolrelextrema(data, epoch_script::math::less, order));
}

template <class T>
std::vector<size_t> argrelmax(std::span<T> const &data, int order = 1) {
  return nonzero(boolrelextrema(data, epoch_script::math::greater, order));
}

template <template <class T, class... Args> class Container, class T,
          class... Args>
double mean(Container<T, Args...> const &data) {
  return std::accumulate(std::begin(data), std::end(data), 0.0,
                         std::plus<T>{}) /
         static_cast<double>(data.size());
}

template <template <class T, class... Args> class Container, class T,
          class... Args>
double stddev(Container<T, Args...> const &data, double mu) {
  const double var = std::accumulate(std::begin(data), std::end(data), 0.0,
                                     [mu](auto &&agg, T num) {
                                       return agg + std::pow(num - mu, 2);
                                     }) /
                     static_cast<double>(data.size());
  return std::sqrt(var);
}

} // namespace epoch_script::math