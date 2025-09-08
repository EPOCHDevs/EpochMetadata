//
// Created by adesola on 9/1/24.
//

#include "gap_returns.h"
#include "epoch_frame/aliases.h"
#include <bits/chrono.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/index.h>
#include <epoch_metadata/bar_attribute.h>

namespace epoch_metadata::transform {

int64_t floorToDay(std::optional<int64_t> const &timestamp) {
  return static_cast<int64_t>(std::floor(*timestamp / 86400000000000));
}

epoch_frame::DataFrame
GapReturns::TransformData(epoch_frame::DataFrame const &bars) const {
  auto t = bars.index()->array().to_timestamp_view();
  auto open = bars[epoch_metadata::EpochStratifyXConstants::instance().OPEN()]
                  .contiguous_array()
                  .to_view<double>();
  auto close = bars[epoch_metadata::EpochStratifyXConstants::instance().CLOSE()]
                   .contiguous_array()
                   .to_view<double>();

  std::vector<double> gapReturns(open->length(), std::nan(""));

  if (open->length() > 0) {

    auto gapReturnsIt = gapReturns.begin() + 1;
    auto closeIt = close->begin();
    auto openIt = open->begin() + 1;
    auto timeIt = t->begin() + 1;

    while (gapReturnsIt != gapReturns.end()) {
      if (floorToDay(*(timeIt - 1)) != floorToDay(*timeIt) && *closeIt &&
          *openIt) {
        *gapReturnsIt = (**openIt - **closeIt) / **closeIt;
      }
      ++gapReturnsIt;
      ++closeIt;
      ++openIt;
      ++timeIt;
    }
  }

  return epoch_frame::make_dataframe<double>(bars.index(), {gapReturns},
                                             {GetOutputId()});
}
} // namespace epoch_metadata::transform