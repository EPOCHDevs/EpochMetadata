#include <DataFrame/DataFrame.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <DataFrame/DataFrameTypes.h>
#include <catch.hpp>
#include <filesystem>

#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"

#include "epoch_metadata/constants.h"
#include "epoch_metadata/bar_attribute.h"
#include "transforms/src/config_helper.h"
#include "transforms/src/hosseinmoein/indicators/zscore.h"

using namespace epoch_frame;
using namespace epoch_metadata::transform;

TEST_CASE("ZScore rolling", "[hosseinmoein][zscore]") {
  auto C = epoch_metadata::EpochStratifyXConstants::instance();
  auto path = std::format("{}/test_data/hmdf/IBM.csv",
                          std::filesystem::current_path().string());

  hmdf::StdDataFrame<std::string> df;
  df.read(path.c_str(), hmdf::io_format::csv2);

  auto index_arr =
      Series{factory::array::make_array(df.get_index())}.str().strptime(
          arrow::compute::StrptimeOptions{"%Y-%m-%d", arrow::TimeUnit::NANO});
  auto index = factory::index::make_index(
      index_arr.value(), MonotonicDirection::Increasing, "Date");

  auto input_df = make_dataframe<double>(
      index, {df.get_column<double>("IBM_Close")}, {C.CLOSE()});

  const auto tf =
      epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
  const int64_t window = 20;

  YAML::Node inputs_yaml;
  inputs_yaml["SLOT"] = C.CLOSE();
  YAML::Node options_yaml;
  options_yaml["window"] = window;
  auto cfg = run_op("zscore", "zscore_id", inputs_yaml, options_yaml, tf);

  ZScore z{cfg};
  auto out = z.TransformData(input_df);

  // Expected rolling z-score: for each window, use HMDF ZScoreVisitor over the
  // window and take the last value
  const auto vals = input_df[C.CLOSE()].contiguous_array().to_vector<double>();
  std::vector<double> exp(vals.size(),
                          std::numeric_limits<double>::quiet_NaN());
  for (size_t i = 0; i < vals.size(); ++i) {
    if (i + 1 < static_cast<size_t>(window))
      continue;
    size_t start = i + 1 - static_cast<size_t>(window);
    hmdf::StdDataFrame<int64_t> tmp;
    tmp.load_index(
        hmdf::StdDataFrame<int64_t>::gen_sequence_index(0, window, 1));
    std::vector<double> w(vals.begin() + start, vals.begin() + i + 1);
    tmp.load_column("x", w);
    hmdf::ZScoreVisitor<double> v;
    tmp.single_act_visit<double>("x", v);
    auto vec = v.get_result();
    exp[i] = vec.back();
  }

  auto lhs = out[cfg.GetOutputId("result")].contiguous_array().slice(
      window, vals.size() - window);
  auto rhs = Array{factory::array::make_contiguous_array(exp)}.slice(
      window, vals.size() - window);
  INFO(lhs << "\n!==\n" << rhs);
  REQUIRE(lhs.is_approx_equal(rhs));
}
