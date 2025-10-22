#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/transforms/config_helper.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include <arrow/compute/api.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/serialization.h>
#include <index/datetime_index.h>
#include <iostream>

// Test consolidation box with various parameter combinations
int main() {
  using namespace epoch_frame;
  using namespace epoch_metadata;
  using namespace epoch_metadata::transform;

  constexpr auto test_instrument = "EURUSD";
  auto path = std::format("{}/{}/{}_4H.csv", SMC_TEST_DATA_DIR, test_instrument,
                          test_instrument);
  auto df_result = read_csv_file(path, CSVReadOptions{});
  if (!df_result.ok()) {
    std::cerr << "Failed to read CSV: " << df_result.status().ToString() << std::endl;
    return 1;
  }
  auto df = df_result.ValueOrDie();

  // Parse timestamp index
  arrow::compute::StrptimeOptions str_options{"%d.%m.%Y %H:%M:%S",
                                              arrow::TimeUnit::NANO};
  auto index = df["Date"].str().strptime(str_options).dt().tz_localize("UTC");
  df = df.set_index(std::make_shared<DateTimeIndex>(index.value()));

  // Rename columns to standard OHLC names
  auto const &C = epoch_metadata::EpochStratifyXConstants::instance();
  std::unordered_map<std::string, std::string> replacements{
      {"Open", C.OPEN()},
      {"High", C.HIGH()},
      {"Low", C.LOW()},
      {"Close", C.CLOSE()},
      {"Volume", C.VOLUME()}};
  df = df.rename(replacements);
  df = df.assign(C.VOLUME(), df[C.VOLUME()].cast(arrow::float64()));

  auto timeframe = C.DAILY_FREQUENCY;

  std::cout << "Total rows in dataset: " << df.num_rows() << std::endl << std::endl;

  // Test various parameter combinations and data ranges
  struct TestCase {
    std::string name;
    int start;
    int end;
    int lookback;
    int min_pivot_points;
    double r_squared_min;
    double max_slope;
  };

  std::vector<TestCase> test_cases = {
      // Original parameters
      {"Original (1000-1500)", 1000, 1500, 40, 5, 0.75, 0.0001},

      // Relax R² requirement
      {"Relaxed R² (1000-1500)", 1000, 1500, 40, 5, 0.5, 0.0001},
      {"Very relaxed R² (1000-1500)", 1000, 1500, 40, 5, 0.3, 0.0001},

      // Relax slope requirement
      {"Relaxed slope (1000-1500)", 1000, 1500, 40, 5, 0.75, 0.001},
      {"Very relaxed slope (1000-1500)", 1000, 1500, 40, 5, 0.75, 0.01},

      // Different lookback
      {"Shorter lookback (1000-1500)", 1000, 1500, 20, 5, 0.75, 0.0001},
      {"Longer lookback (1000-1500)", 1000, 1500, 60, 5, 0.75, 0.0001},

      // Try different data ranges
      {"Original params (0-500)", 0, 500, 40, 5, 0.75, 0.0001},
      {"Original params (500-1000)", 500, 1000, 40, 5, 0.75, 0.0001},
      {"Original params (1500-2000)", 1500, 2000, 40, 5, 0.75, 0.0001},
      {"Original params (5000-5500)", 5000, 5500, 40, 5, 0.75, 0.0001},

      // Most relaxed
      {"Very relaxed all (1000-1500)", 1000, 1500, 40, 5, 0.3, 0.01},
  };

  for (const auto& tc : test_cases) {
    auto df_slice = df.iloc({tc.start, tc.end});

    auto config = consolidation_box_cfg("consolidation_box", tc.lookback,
                                        tc.min_pivot_points, tc.r_squared_min,
                                        tc.max_slope, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    auto result = transform->TransformData(df_slice);
    auto bool_array = result[transform->GetOutputId("box_detected")].contiguous_array();
    auto arrow_result = arrow::compute::Sum(*bool_array.value());

    size_t detections = 0;
    if (arrow_result.ok() && arrow_result->is_scalar()) {
      auto scalar = arrow_result->scalar();
      if (scalar->is_valid) {
        detections = static_cast<size_t>(
            std::static_pointer_cast<arrow::Int64Scalar>(scalar)->value);
      }
    }

    std::cout << tc.name << ":" << std::endl;
    std::cout << "  lookback=" << tc.lookback
              << ", min_pivots=" << tc.min_pivot_points
              << ", r²_min=" << tc.r_squared_min
              << ", max_slope=" << tc.max_slope << std::endl;
    std::cout << "  Detections: " << detections << std::endl << std::endl;
  }

  return 0;
}
