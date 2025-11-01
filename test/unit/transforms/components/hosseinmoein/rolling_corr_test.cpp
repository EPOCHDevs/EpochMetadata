#include <catch.hpp>
#include <filesystem>

#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include "epoch_frame/scalar.h"

#include "epochflow/bar_attribute.h"
#include "epochflow/constants.h"
#include "epochflow/transforms/config_helper.h"

#include "transforms/src/hosseinmoein/statistics/rolling_corr.h"

using namespace epoch_frame;
using namespace epochflow::transform;

TEST_CASE("RollingCorr basic correlation",
          "[hosseinmoein][rolling_corr]") {
  const auto tf =
      epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  const int64_t window = 20;
  YAML::Node inputs_yaml;
  inputs_yaml["x"] = "x";
  inputs_yaml["y"] = "y";
  YAML::Node options_yaml;
  options_yaml["window"] = window;
  auto cfg = run_op("rolling_corr", "rolling_corr_id", inputs_yaml, options_yaml, tf);

  // Build synthetic x, y with perfect positive correlation
  const size_t N = 200;
  std::vector<int64_t> ticks(N);
  std::iota(ticks.begin(), ticks.end(), 0);
  auto idx_arr = epoch_frame::factory::array::make_contiguous_array(ticks);
  auto index =
      factory::index::make_index(idx_arr, MonotonicDirection::Increasing, "i");

  std::vector<double> xvec(N), yvec(N);
  for (size_t i = 0; i < N; ++i) {
    xvec[i] = static_cast<double>(i);
    yvec[i] = 2.0 * xvec[i] + 3.0;  // Perfect linear relationship
  }
  auto df_xy = make_dataframe<double>(index, {xvec, yvec}, {"x", "y"});

  RollingCorr corr{cfg};
  auto out = corr.TransformData(df_xy);

  // Check that we have correlation output
  REQUIRE(out.has_column(cfg.GetOutputId("correlation")));

  // After window size, correlation should be close to 1.0 (perfect positive)
  const auto corr_series = out[cfg.GetOutputId("correlation")];
  const auto corr_values = corr_series.contiguous_array().to_view<double>()->raw_values();

  // Check a few values after the window
  for (size_t i = window; i < N; ++i) {
    REQUIRE(corr_values[i] > 0.99);  // Should be very close to 1.0
  }
}

TEST_CASE("RollingCorr negative correlation",
          "[hosseinmoein][rolling_corr]") {
  const auto tf =
      epochflow::EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  const int64_t window = 20;
  YAML::Node inputs_yaml;
  inputs_yaml["x"] = "x";
  inputs_yaml["y"] = "y";
  YAML::Node options_yaml;
  options_yaml["window"] = window;
  auto cfg = run_op("rolling_corr", "rolling_corr_id", inputs_yaml, options_yaml, tf);

  // Build synthetic x, y with perfect negative correlation
  const size_t N = 200;
  std::vector<int64_t> ticks(N);
  std::iota(ticks.begin(), ticks.end(), 0);
  auto idx_arr = epoch_frame::factory::array::make_contiguous_array(ticks);
  auto index =
      factory::index::make_index(idx_arr, MonotonicDirection::Increasing, "i");

  std::vector<double> xvec(N), yvec(N);
  for (size_t i = 0; i < N; ++i) {
    xvec[i] = static_cast<double>(i);
    yvec[i] = -2.0 * xvec[i] + 100.0;  // Perfect negative linear relationship
  }
  auto df_xy = make_dataframe<double>(index, {xvec, yvec}, {"x", "y"});

  RollingCorr corr{cfg};
  auto out = corr.TransformData(df_xy);

  const auto corr_series = out[cfg.GetOutputId("correlation")];
  const auto corr_values = corr_series.contiguous_array().to_view<double>()->raw_values();

  // After window size, correlation should be close to -1.0 (perfect negative)
  for (size_t i = window; i < N; ++i) {
    REQUIRE(corr_values[i] < -0.99);  // Should be very close to -1.0
  }
}
