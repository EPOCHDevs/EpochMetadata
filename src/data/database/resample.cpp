#include "resample.h"

#include "epoch_frame/common.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/market_calendar.h"
#include "index/datetime_index.h"
#include "methods/time_grouper.h"
#include <epoch_script/data/model/exchange_calendar.h>
#include <common/epoch_thread_pool.h>
#include <epoch_frame/series.h>
#include <epoch_script/transforms/core/bar_resampler.h>
#include <oneapi/tbb/parallel_for.h>

namespace epoch_script::data {
epoch_frame::DataFrame Resampler::AdjustTimestamps(
    asset::Asset const &asset, epoch_frame::IndexPtr const &baseIndex,
    epoch_frame::DataFrame const &resampled, bool isIntradayTF) const {
  if (baseIndex->size() == 0 || resampled.num_rows() == 0) {
    return resampled;
  }

  if (!m_isIntraday || (m_isIntraday && isIntradayTF)) {
    return resampled;
  }

  auto resampledIndex = resampled.index();

  auto calendar =
      epoch_script::calendar::GetExchangeCalendarFromSpec(asset.GetSpec());
  using namespace epoch_frame;

  auto marketEnd =
      calendar->days_at_time(resampledIndex, MarketTimeType::MarketClose);

  AssertFromStream(marketEnd.size() == resampledIndex->size(),
                   marketEnd.size() << " != " << resampledIndex->size());

  // Create new DataFrame with adjusted timestamps
  return DataFrame{
      std::make_shared<DateTimeIndex>(marketEnd.contiguous_array().value()),
      resampled.table()};
}

std::vector<std::tuple<TimeFrameNotation, asset::Asset, epoch_frame::DataFrame>>
Resampler::Build(AssetDataFrameMap const &group) const {
  SPDLOG_INFO("Resampling {} assets to {}.", group.size(), m_timeFrames.size());
  std::vector<std::pair<asset::Asset, epoch_script::TimeFrame>> flattened;
  flattened.reserve(group.size() * m_timeFrames.size());
  for (auto const &asset : group | std::views::keys) {
    for (auto const &tf : m_timeFrames) {
      if (asset.IsFuturesContract() && !asset.IsFuturesContinuation()) {
        // Resampling contracts is not acceptable
        continue;
      }
      flattened.emplace_back(asset, tf);
    }
  }

  std::vector<std::optional<
      std::tuple<TimeFrameNotation, asset::Asset, epoch_frame::DataFrame>>>
      result;
  result.resize(flattened.size());

  epoch_frame::EpochThreadPool::getInstance().execute([&] {
    oneapi::tbb::parallel_for(
        oneapi::tbb::blocked_range<size_t>(0, flattened.size()),
        [&](auto const &range) {
          for (size_t i = range.begin(); i < range.end(); ++i) {
            auto const &[asset, tf] = flattened[i];
            auto df = group.at(asset);
            AssertFromStream(
                epoch_frame::arrow_utils::get_tz(df.index()->dtype()) == "UTC",
                "Resampler only supports UTC timezones");

            auto start = std::chrono::high_resolution_clock::now();
            df = this->AdjustTimestamps(
                asset, df.index(),
                epoch_script::transform::resample_ohlcv(df, tf.GetOffset()),
                tf.IsIntraDay());
            auto end = std::chrono::high_resolution_clock::now();
            SPDLOG_DEBUG("Resampled {} to {} in {} s", asset.GetSymbolStr(),
                        tf.ToString(),
                        std::chrono::duration<double>(end - start).count());
            result[i] = {tf.ToString(), asset, df};
          }
        });
  });

  return result |
         std::views::transform([](auto const &item) { return item.value(); }) |
         ranges::to_vector_v;
}
} // namespace epoch_script::data