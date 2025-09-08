//
// Tests for gap_classify transform (daily and intraday inputs)
//

#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"

#include <arrow/builder.h>
#include <catch2/catch_test_macros.hpp>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include "epoch_metadata/constants.h"
#include "epoch_metadata/bar_attribute.h"

using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace epoch_frame;
using namespace std::chrono;

static TransformConfiguration make_gap_classify_cfg(const std::string &,
                                                    size_t fill_percent = 100) {
  return TransformConfiguration{TransformDefinition{YAML::Load(R"(
type: gap_classify
id: gap
timeframe: { interval: 1, type: day }
options: { fill_percent: )" + std::to_string(fill_percent) + R"( }
)")}};
}

// Helper to create nullable boolean array
template <typename T>
std::shared_ptr<arrow::ChunkedArray>
make_nullable_array(const std::vector<std::optional<T>> &values) {
  if constexpr (std::is_same_v<T, bool>) {
    arrow::BooleanBuilder builder;
    (void)builder.Reserve(values.size());
    for (const auto &val : values) {
      if (val.has_value()) {
        builder.UnsafeAppend(*val);
      } else {
        builder.UnsafeAppendNull();
      }
    }
    std::shared_ptr<arrow::Array> array;
    (void)builder.Finish(&array);
    return std::make_shared<arrow::ChunkedArray>(array);
  } else if constexpr (std::is_same_v<T, double>) {
    arrow::DoubleBuilder builder;
    (void)builder.Reserve(values.size());
    for (const auto &val : values) {
      if (val.has_value()) {
        builder.UnsafeAppend(*val);
      } else {
        builder.UnsafeAppendNull();
      }
    }
    std::shared_ptr<arrow::Array> array;
    (void)builder.Finish(&array);
    return std::make_shared<arrow::ChunkedArray>(array);
  } else if constexpr (std::is_same_v<T, int64_t>) {
    arrow::TimestampBuilder builder(
        arrow::timestamp(arrow::TimeUnit::NANO, "UTC"),
        arrow::default_memory_pool());
    (void)builder.Reserve(values.size());
    for (const auto &val : values) {
      if (val.has_value()) {
        builder.UnsafeAppend(*val);
      } else {
        builder.UnsafeAppendNull();
      }
    }
    std::shared_ptr<arrow::Array> array;
    (void)builder.Finish(&array);
    return std::make_shared<arrow::ChunkedArray>(array);
  }
}

TEST_CASE("gap_classify - Daily and Intraday") {
  const auto &C = epoch_metadata::EpochStratifyXConstants::instance();

  SECTION("Daily data: up filled and down partial") {
    TransformConfiguration cfg = make_gap_classify_cfg("gap");
    auto transformBase = MAKE_TRANSFORM(cfg);
    auto t = dynamic_cast<ITransform *>(transformBase.get());

    auto index = factory::index::make_datetime_index(
        {DateTime{2024y, January, 2d},   // d0
         DateTime{2024y, January, 3d},   // d1 -> gap up vs d0 close
         DateTime{2024y, January, 4d}}); // d2 -> gap down vs d1 close

    // d0: baseline
    // d1: prev close=100, open=103 (gap up 3). low=100 => filled fully
    // d2: prev close=104, open=97 (gap down 7). high=100 => partial fill 3/7
    DataFrame bars =
        make_dataframe<double>(index,
                               {
                                   {100.0, 104.0, 98.0},  // CLOSE
                                   {101.0, 103.0, 97.0},  // OPEN
                                   {105.0, 106.0, 101.0}, // HIGH
                                   {95.0, 100.0, 96.0}    // LOW
                               },
                               {C.CLOSE(), C.OPEN(), C.HIGH(), C.LOW()});

    auto result = t->TransformData(bars);

    // New nullable Arrow format: gap_up, gap_filled, fill_fraction, gap_size,
    // psc, psc_timestamp
    using std::nullopt;

    // gap_up: null, true (up gap), false (down gap)
    std::vector<std::optional<bool>> gap_up{nullopt, true, false};

    // gap_filled: null (placeholder - fill detection not implemented yet)
    std::vector<std::optional<bool>> gap_filled{nullopt, nullopt, nullopt};

    // fill_fraction: null (placeholder - fill detection not implemented yet)
    std::vector<std::optional<double>> fill_fraction{nullopt, nullopt, nullopt};

    // gap_size: null, 3.0, 7.0
    std::vector<std::optional<double>> gap_size{nullopt, 3.0, 7.0};

    // psc: null, 100.0, 104.0 (prior session close prices)
    std::vector<std::optional<double>> psc{nullopt, 100.0, 104.0};

    // psc_timestamp: null, timestamp of d0, timestamp of d1
    auto timestamp_view = index->array().to_timestamp_view();
    auto d0_timestamp = timestamp_view->Value(0);
    auto d1_timestamp = timestamp_view->Value(1);
    std::vector<std::optional<int64_t>> psc_timestamp{
        nullopt, std::make_optional(d0_timestamp),
        std::make_optional(d1_timestamp)};

    DataFrame expected = make_dataframe(
        index,
        {make_nullable_array(gap_up), make_nullable_array(gap_filled),
         make_nullable_array(fill_fraction), make_nullable_array(gap_size),
         make_nullable_array(psc), make_nullable_array(psc_timestamp)},
        {cfg.GetOutputId("gap_up"), cfg.GetOutputId("gap_filled"),
         cfg.GetOutputId("fill_fraction"), cfg.GetOutputId("gap_size"),
         cfg.GetOutputId("psc"), cfg.GetOutputId("psc_timestamp")});

    INFO("Daily gap_classify result\n" << result << "\n!=\n" << expected);
    REQUIRE(result.equals(expected, arrow::EqualOptions{}.atol(1e-6)));
  }

  SECTION("Daily data: thresholded fill (50%)") {
    TransformConfiguration cfg = make_gap_classify_cfg("gap", 50);
    auto transformBase = MAKE_TRANSFORM(cfg);
    auto t = dynamic_cast<ITransform *>(transformBase.get());

    auto index = factory::index::make_datetime_index(
        {DateTime{2024y, January, 2d},   // d0
         DateTime{2024y, January, 3d},   // d1 -> gap up vs d0 close
         DateTime{2024y, January, 4d}}); // d2 -> gap down vs d1 close

    // Same bars as prior daily test
    DataFrame bars =
        make_dataframe<double>(index,
                               {
                                   {100.0, 104.0, 98.0},  // CLOSE
                                   {101.0, 103.0, 97.0},  // OPEN
                                   {105.0, 106.0, 101.0}, // HIGH
                                   {95.0, 100.0, 96.0}    // LOW
                               },
                               {C.CLOSE(), C.OPEN(), C.HIGH(), C.LOW()});

    auto result = t->TransformData(bars);

    // Same data but with 50% fill threshold - using new nullable format
    using std::nullopt;

    std::vector<std::optional<bool>> gap_up{nullopt, true, false};
    std::vector<std::optional<bool>> gap_filled{nullopt, nullopt,
                                                nullopt}; // placeholder
    std::vector<std::optional<double>> fill_fraction{nullopt, nullopt,
                                                     nullopt}; // placeholder
    std::vector<std::optional<double>> gap_size{nullopt, 3.0, 7.0};
    std::vector<std::optional<double>> psc{nullopt, 100.0, 104.0};

    auto timestamp_view = index->array().to_timestamp_view();
    auto d0_timestamp = timestamp_view->Value(0);
    auto d1_timestamp = timestamp_view->Value(1);
    std::vector<std::optional<int64_t>> psc_timestamp{
        nullopt, std::make_optional(d0_timestamp),
        std::make_optional(d1_timestamp)};

    DataFrame expected = make_dataframe(
        index,
        {make_nullable_array(gap_up), make_nullable_array(gap_filled),
         make_nullable_array(fill_fraction), make_nullable_array(gap_size),
         make_nullable_array(psc), make_nullable_array(psc_timestamp)},
        {cfg.GetOutputId("gap_up"), cfg.GetOutputId("gap_filled"),
         cfg.GetOutputId("fill_fraction"), cfg.GetOutputId("gap_size"),
         cfg.GetOutputId("psc"), cfg.GetOutputId("psc_timestamp")});

    INFO("Daily gap_classify (50%) result\n" << result << "\n!=\n" << expected);
    REQUIRE(result.equals(expected, arrow::EqualOptions{}.atol(1e-6)));
  }

  SECTION("Intraday data: up filled, down partial") {
    TransformConfiguration cfg = make_gap_classify_cfg("gap");
    auto transformBase = MAKE_TRANSFORM(cfg);
    auto t = dynamic_cast<ITransform *>(transformBase.get());

    auto index = factory::index::make_datetime_index(
        {DateTime{2024y, September, 1d, 10h, 0min, 0s},
         DateTime{2024y, September, 1d, 15h, 59min, 0s},
         DateTime{2024y, September, 2d, 9h, 30min, 0s}, // new day -> up
         DateTime{2024y, September, 2d, 10h, 0min, 0s},
         DateTime{2024y, September, 3d, 9h, 30min, 0s}}); // new day -> down

    DataFrame bars =
        make_dataframe<double>(index,
                               {
                                   {100.0, 102.0, 105.0, 106.0, 99.0},  // CLOSE
                                   {100.0, 100.0, 104.0, 105.0, 100.0}, // OPEN
                                   {102.0, 103.0, 106.0, 107.0, 101.0}, // HIGH
                                   {99.0, 99.0, 102.0, 104.0, 98.0}     // LOW
                               },
                               {C.CLOSE(), C.OPEN(), C.HIGH(), C.LOW()});

    auto result = t->TransformData(bars);

    // Intraday gaps using new nullable format
    using std::nullopt;

    // gap_up: null, null, true, null, false
    std::vector<std::optional<bool>> gap_up{nullopt, nullopt, true, nullopt,
                                            false};
    std::vector<std::optional<bool>> gap_filled{
        nullopt, nullopt, nullopt, nullopt, nullopt}; // placeholder
    std::vector<std::optional<double>> fill_fraction{
        nullopt, nullopt, nullopt, nullopt, nullopt}; // placeholder
    // gap_size: null, null, 2.0, null, 6.0
    std::vector<std::optional<double>> gap_size{nullopt, nullopt, 2.0, nullopt,
                                                6.0};
    // psc: null, null, 102.0, null, 106.0 (prior session close prices)
    std::vector<std::optional<double>> psc{nullopt, nullopt, 102.0, nullopt,
                                           106.0};

    auto timestamp_view = index->array().to_timestamp_view();
    auto d1_timestamp =
        timestamp_view->Value(1); // prev close for gap at index 2
    auto d3_timestamp =
        timestamp_view->Value(3); // prev close for gap at index 4
    std::vector<std::optional<int64_t>> psc_timestamp{
        nullopt, nullopt, std::make_optional(d1_timestamp), nullopt,
        std::make_optional(d3_timestamp)};

    DataFrame expected = make_dataframe(
        index,
        {make_nullable_array(gap_up), make_nullable_array(gap_filled),
         make_nullable_array(fill_fraction), make_nullable_array(gap_size),
         make_nullable_array(psc), make_nullable_array(psc_timestamp)},
        {cfg.GetOutputId("gap_up"), cfg.GetOutputId("gap_filled"),
         cfg.GetOutputId("fill_fraction"), cfg.GetOutputId("gap_size"),
         cfg.GetOutputId("psc"), cfg.GetOutputId("psc_timestamp")});

    INFO("Intraday gap_classify result\n" << result << "\n!=\n" << expected);
    REQUIRE(result.equals(expected, arrow::EqualOptions{}.atol(1e-6)));
  }
}
