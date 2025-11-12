//
// Created by adesola on 1/26/25.
//
#include "constructed_bars.h"

namespace epoch_script::futures {
void FuturesConstructedBars::reserve(int N) {
  o.reserve(N);
  h.reserve(N);
  l.reserve(N);
  c.reserve(N);
  v.reserve(N);
  oi.reserve(N);
  t.reserve(N);
  s.reserve(N);
}

FuturesConstructedBars
FuturesConstructedBars::constant(std::vector<double> const &closePrices,
                                 std::vector<int64_t> const &timestamps,
                                 std::vector<std::string> const & contracts) {
  return {closePrices,
          closePrices,
          closePrices,
          closePrices,
          std::vector<double>(closePrices.size(), 1),
          std::vector<double>(closePrices.size(), 1),
          timestamps,
  contracts};
}

std::vector<double> &
FuturesConstructedBars::operator[](epoch_script::BarAttribute::Type type) {
  switch (type) {
  case epoch_script::BarAttribute::High:
    return h;
  case epoch_script::BarAttribute::Low:
    return l;
  case epoch_script::BarAttribute::Close:
    return c;
  case epoch_script::BarAttribute::Open:
    return o;
  case epoch_script::BarAttribute::Volume:
    return v;
  case epoch_script::BarAttribute::OpenInterest:
    return oi;
  default:
    throw std::runtime_error(
        "AppendDoubleByType only supports ohlcv oi attributes");
  }
}

  const std::vector<double> &
FuturesConstructedBars::operator[](epoch_script::BarAttribute::Type type) const{
  switch (type) {
    case epoch_script::BarAttribute::High:
      return h;
    case epoch_script::BarAttribute::Low:
      return l;
    case epoch_script::BarAttribute::Close:
      return c;
    case epoch_script::BarAttribute::Open:
      return o;
    case epoch_script::BarAttribute::Volume:
      return v;
    case epoch_script::BarAttribute::OpenInterest:
      return oi;
    default:
      throw std::runtime_error(
          "AppendDoubleByType only supports ohlcv oi attributes");
  }
}
} // namespace epoch_script::futures