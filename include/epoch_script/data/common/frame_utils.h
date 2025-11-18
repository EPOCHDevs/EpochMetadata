#pragma once

#include "epoch_frame/datetime.h"
#include "epoch_script/core/bar_attribute.h"
#include <cstdio>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/index.h>
#include <epoch_frame/series.h>
#include <optional>
#include <random>
#include <string>
#include <vector>

using namespace epoch_frame::factory;

namespace epoch_script {
static epoch_frame::DataFrame
make_random_ohlcv(epoch_frame::IndexPtr const &index,
                  std::optional<std::string> const &symbol = std::nullopt) {
  std::mt19937_64 rng(123456789);
  std::uniform_real_distribution<double> dist(100, 1000);
  std::vector<double> high(index->size());
  std::ranges::generate(high, [&]() { return dist(rng); });

  std::vector<double> low(index->size());
  std::vector<double> close(index->size());
  std::vector<double> open(index->size());
  std::vector<double> volume(index->size());
  std::vector<std::string> contracts(index->size());
  int i = 0;
  std::ranges::generate(low, [&]() {
    auto h = high[i];
    dist = std::uniform_real_distribution<double>(0, h);
    const auto l = dist(rng);

    dist = std::uniform_real_distribution<double>(l, h);
    close[i] = dist(rng);

    dist = std::uniform_real_distribution<double>(l, h);
    open[i] = dist(rng);

    dist = std::uniform_real_distribution<double>(0, 10000);
    volume[i] = dist(rng);

    if (symbol) {
      contracts[i] = *symbol;
    }
    ++i;
    return l;
  });

  auto df = epoch_frame::make_dataframe<double>(
      index, {open, high, low, close, volume}, {"o", "h", "l", "c", "v"});
  if (symbol) {
    return df.assign("s", epoch_frame::make_series(df.index(), contracts));
  }
  return df;
}

template <typename I, typename C, typename T, typename... Args>
epoch_frame::DataFrame
make_single_row_dataframe(I const &date, std::vector<T> const &values,
                          std::vector<C> const &columns, Args &&...args) {
  // Force UTC for all single-row datetime indices
  (void)std::initializer_list<int>{(static_cast<void>(args), 0)...};
  auto index = epoch_frame::factory::index::make_datetime_index(
      std::vector<I>{date}, "", "UTC");

  std::vector<std::vector<T>> data;
  for (auto const &value : values) {
    data.push_back({value});
  }
  std::vector<std::string> column_names;
  if constexpr (std::is_same_v<std::decay_t<C>, epoch_script::BarAttribute>) {
    for (auto const &column : columns) {
      column_names.push_back(column());
    }
  } else {
    column_names = columns;
  }

  if constexpr (std::is_same_v<T, epoch_frame::Scalar>) {
    arrow::FieldVector fields;
    for (auto const &[column, value] : std::views::zip(column_names, values)) {
      fields.push_back(arrow::field(column, value.type()));
    }
    return epoch_frame::make_dataframe(index, data, columns, fields);
  } else {
    return epoch_frame::make_dataframe<T>(index, data, column_names);
  }
}
} // namespace epoch_script
