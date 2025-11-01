//
// Created to test intradayOnly and requiresSession enforcement in runners
//

#include "transforms/tbb_nodes/function_node.h"
#include "transforms/tbb_nodes/icache_manager.h"
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_definition.h>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <epoch_frame/datetime.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_script/time_frame.h>
#include <model/asset/asset.h>
#include <epoch_script/core/constants.h>

using namespace epoch_script;
using namespace epoch_script::transform;
using namespace epoch_frame;

namespace {

// Simple pass-through transform used for testing
class PassThroughTransform final : public ITransform {
public:
  explicit PassThroughTransform(TransformConfiguration config)
      : ITransform(std::move(config)) {}

  epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &df) const override {
    return df; // pass-through
  }
};

// Minimal fake cache manager to intercept inputs/outputs
class FakeCacheManager final : public transform::ICacheManager {
public:
  void InitializeBaseData(TimeFrameAssetDataFrameMap data,
                          const asset::AssetHashSet &allowed) override {
    (void)allowed;
    m_baseData = std::move(data);
  }

  epoch_frame::DataFrame
  GatherInputs(const asset::Asset &asset,
               const ITransformBase &transformer) const override {
    auto tf = transformer.GetTimeframe().ToString();
    return m_baseData.at(tf).at(asset);
  }

  void InsertConfig(const std::string &, TransformConfiguration) override {}

  void StoreTransformOutput(const asset::Asset &asset, const ITransformBase &,
                            const epoch_frame::DataFrame &data) override {
    m_lastOutput[asset] = data;
  }

  TimeFrameAssetDataFrameMap BuildFinalOutput() override { return {}; }

  asset::Assets GetAssets() const override { return m_assets; }

  void SetAssets(asset::Assets assets) { m_assets = std::move(assets); }

  const epoch_frame::DataFrame &GetLastOutput(const asset::Asset &asset) const {
    if (m_lastOutput.find(asset) == m_lastOutput.end()) {
      throw std::runtime_error(
          "No output stored for asset - transform may have failed");
    }
    return m_lastOutput.at(asset);
  }

private:
  TimeFrameAssetDataFrameMap m_baseData;
  mutable std::unordered_map<asset::Asset, epoch_frame::DataFrame,
                             asset::AssetHash>
      m_lastOutput;
  asset::Assets m_assets;
};

// Helpers
transform::TransformConfiguration
MakeConfig(std::string const &id, epoch_script::TimeFrame const &tf,
           std::optional<epoch_frame::SessionRange> sessionRange,
           bool intradayOnly) {
  epoch_script::transforms::TransformsMetaData meta{};
  meta.id = id;
  meta.intradayOnly = intradayOnly;
  // outputs not required for this test; fake cache stores what is passed in

  TransformDefinitionData data{.type = id,
                               .id = id,
                               .options = {},
                               .timeframe = tf,
                               .inputs = {},
                               .metaData = meta,
                               .sessionRange = sessionRange};
  return transform::TransformConfiguration{TransformDefinition{data}};
}

asset::Asset MakeAAPL() {
  return EpochStratifyXAssetConstants::instance().AAPL;
}

// Build a simple intra-day index between [start, end] every delta minutes
epoch_frame::IndexPtr MakeIntradayIndex(epoch_frame::Date startDate,
                                        epoch_frame::Time startTime,
                                        epoch_frame::Time endTime,
                                        int minutesStep) {
  std::vector<epoch_frame::DateTime> times;
  auto t = startTime;

  // Safety counter to prevent infinite loops
  int maxIterations = 100;
  int iterations = 0;

  // Build across the same day
  while (iterations < maxIterations) {
    times.emplace_back(startDate, t);
    if (t >= endTime)
      break;

    // Convert to total minutes, add step, then convert back
    auto currentMinutes =
        std::chrono::duration_cast<std::chrono::minutes>(t.to_duration())
            .count();
    auto newTotalMinutes = currentMinutes + minutesStep;
    auto newHours = newTotalMinutes / 60;
    auto newMinutes = newTotalMinutes % 60;

    auto newTime = epoch_frame::Time(std::chrono::hours(newHours),
                                     std::chrono::minutes(newMinutes));

    // Debug: Check if time is actually advancing
    if (newTime <= t) {
      std::cout << "ERROR: Time not advancing! Current: " << currentMinutes
                << " minutes, New: " << newTotalMinutes << " minutes"
                << std::endl;
      break;
    }

    t = newTime;
    iterations++;
  }

  if (iterations >= maxIterations) {
    std::cout << "ERROR: Hit maximum iterations in MakeIntradayIndex"
              << std::endl;
  }

  return epoch_frame::factory::index::make_datetime_index(times);
}

} // namespace

TEST_CASE("MakeIntradayIndex creates correct time series", "[intraday_index]") {
  auto date = epoch_frame::Date::from_ymd(
      std::chrono::year(2024) / std::chrono::month(1) / std::chrono::day(2));
  auto startTime = epoch_frame::Time(std::chrono::hours(9));
  auto endTime = epoch_frame::Time(std::chrono::hours(11));

  auto index = MakeIntradayIndex(date, startTime, endTime, 30);

  // Should have: 09:00, 09:30, 10:00, 10:30, 11:00 = 5 entries
  REQUIRE(index->size() == 5);

  // Check first and last times
  auto firstTime = index->at(0).to_datetime().time();
  auto lastTime = index->at(index->size() - 1).to_datetime().time();

  REQUIRE(firstTime == startTime);
  REQUIRE(lastTime == endTime);
}

TEST_CASE("intradayOnly=true skips non-intraday timeframes",
          "[transform_runner]") {
  auto asset = MakeAAPL();

  // Daily timeframe
  auto tfDaily =
      epoch_script::TimeFrame(epoch_frame::factory::offset::days(1));
  auto cfg =
      MakeConfig("test_op", tfDaily, std::nullopt, /*intradayOnly*/ true);

  PassThroughTransform transform(cfg);

  // Prepare cache and assets
  auto cache = std::make_unique<FakeCacheManager>();
  cache->SetAssets(asset::Assets{asset});

  // No need to initialize base data; transform should be skipped before gather
  transform::MsgType msg{.cache = std::move(cache),
                         .logger = std::make_unique<Logger>()};

  // Run
  ApplyDefaultTransform(transform, msg);

  // Expect empty output stored for the asset
  const auto &out =
      static_cast<FakeCacheManager *>(msg.cache.get())->GetLastOutput(asset);
  REQUIRE(out.empty());
}

TEST_CASE("requiresSession slices to provided SessionRange",
          "[transform_runner]") {
  auto asset = MakeAAPL();

  // Intraday timeframe (30 minutes)
  auto tf =
      epoch_script::TimeFrame(epoch_frame::factory::offset::minutes(30));

  // Session range 09:30 - 16:00
  epoch_frame::SessionRange session{
      .start =
          epoch_frame::Time(std::chrono::hours(9), std::chrono::minutes(30)),
      .end = epoch_frame::Time(std::chrono::hours(16))};
  auto cfg = MakeConfig("test_op", tf, session, /*intradayOnly*/ false);
  PassThroughTransform transform(cfg);

  // Build intra-day base data from 09:00 to 17:00 inclusive at 30-minute
  // intervals
  auto date = epoch_frame::Date::from_ymd(
      std::chrono::year(2024) / std::chrono::month(1) / std::chrono::day(2));
  auto index = MakeIntradayIndex(date, epoch_frame::Time(std::chrono::hours(9)),
                                 epoch_frame::Time(std::chrono::hours(17)), 30);
  // single dummy column
  std::vector<double> x(index->size(), 1.0);
  auto df = epoch_frame::make_dataframe(
      index, std::vector{epoch_frame::factory::array::make_array(x)},
      std::vector<std::string>{"x"});

  // Base data bucket keyed by timeframe string
  TimeFrameAssetDataFrameMap base;
  base[cfg.GetTimeframe().ToString()][asset] = df;

  auto cache = std::make_unique<FakeCacheManager>();
  cache->SetAssets(asset::Assets{asset});
  cache->InitializeBaseData(std::move(base), {});

  transform::MsgType msg{.cache = std::move(cache),
                         .logger = std::make_unique<Logger>()};

  // Run
  ApplyDefaultTransform(transform, msg);

  // Verify output was sliced to session range
  const auto &out =
      static_cast<FakeCacheManager *>(msg.cache.get())->GetLastOutput(asset);
  REQUIRE_FALSE(out.empty());

  // Count expected rows: inclusive mask on [09:30, 16:00] with 30-min steps
  // from 09:00..17:00
  size_t expected = 0;
  for (size_t i = 0; i < df.size(); ++i) {
    const auto tt = df.index()->at(i).to_datetime().time();
    const bool inNormal = (session.start < session.end) &&
                          (session.start <= tt && tt <= session.end);
    const bool inWrap = (session.start >= session.end) &&
                        (tt >= session.start || tt <= session.end);
    if (inNormal || inWrap)
      ++expected;
  }
  REQUIRE(out.size() == expected);
}
