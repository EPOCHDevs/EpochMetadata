#include "transforms/src/bar_resampler.h"
#include "transforms/src/config_helper.h"
#include "epoch_metadata/bar_attribute.h"

#include "epoch_metadata/transforms/transform_configuration.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include <arrow/compute/api_vector.h>
#include <arrow/type_fwd.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/serialization.h>
#include <index/datetime_index.h>

TEST_CASE("SMC Test") {
  using namespace epoch_frame;
  using namespace epoch_metadata;
  using namespace epoch_metadata::transform;
  constexpr auto test_instrument = "EURUSD";
  auto path = std::format("{}/test_data/{}/{}_15M.csv",
                          std::filesystem::current_path().string(),
                          test_instrument, test_instrument);
  auto df_result = read_csv_file(path, CSVReadOptions{});
  INFO(df_result.status().ToString());
  auto df = df_result.ValueOrDie();
  arrow::compute::StrptimeOptions str_options{"%Y.%m.%d %H:%M:%S",
                                              arrow::TimeUnit::NANO};
  auto index = df["Date"].str().strptime(str_options).dt().tz_localize("UTC");
  df = df.set_index(std::make_shared<DateTimeIndex>(index.value()));

  auto const &C = epoch_metadata::EpochStratifyXConstants::instance();
  std::unordered_map<std::string, std::string> replacements{
      {"Open", C.OPEN()},
      {"High", C.HIGH()},
      {"Low", C.LOW()},
      {"Close", C.CLOSE()},
      {"Volume", C.VOLUME()}};
  df = df.rename(replacements);
  df = df.assign(C.VOLUME(), df[C.VOLUME()].cast(arrow::float64()));

  // Store the transform shared_ptr to avoid stack address issue
  std::shared_ptr<ITransformBase> shlTransformBase;

  auto make_shl = [&]() {
    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto config = swing_highs_lows("swing_highs_lows", 5, timeframe);
    shlTransformBase = MAKE_TRANSFORM(config);
    return dynamic_cast<ITransform *>(shlTransformBase.get());
  };

  SECTION("Sessions") {
    auto file_name = "sessions";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();

    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto config = sessions("sessions", "London", timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto sessions_transform = dynamic_cast<ITransform *>(transformBase.get());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = sessions_transform->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected =
        expected.assign("Active", expected["Active"].cast(arrow::boolean()))
            .assign("High", expected["High"].cast(arrow::float64()))
            .assign("Low", expected["Low"].cast(arrow::float64()))
            .assign("Opened", expected["Opened"].cast(arrow::boolean()))
            .assign("Closed", expected["Closed"].cast(arrow::boolean()));

    auto expected_df = expected.rename(
        {{"Active", sessions_transform->GetOutputId("active")},
         {"High", sessions_transform->GetOutputId("high")},
         {"Low", sessions_transform->GetOutputId("low")},
         {"Opened", sessions_transform->GetOutputId("opened")},
         {"Closed", sessions_transform->GetOutputId("closed")}});
    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      INFO(expected_df[column].contiguous_array()->Diff(
          *result[column].contiguous_array().value()));

      CHECK(result[column].equals(expected_df[column]));
    }
  }

  SECTION("Previous High Low") {
    std::string timeframe(GENERATE("1D", "4h", "W"));
    auto dir = std::filesystem::current_path().string();
    SECTION(std::format("Timeframe: {}", timeframe)) {
      auto file_name = "previous_high_low";
      auto expected =
          read_csv_file(std::format("{}/test_data/{}/{}_result_data_{}.csv",
                                    dir, test_instrument, file_name, timeframe),
                        {})
              .ValueOrDie();

      auto transform_timeframe =
          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
      std::string type;
      int64_t interval = 1;

      if (timeframe == "1D") {
        type = "day";
        interval = 1;
      } else if (timeframe == "4h") {
        type = "hour";
        interval = 4;
      } else if (timeframe == "W") {
        type = "week";
        interval = 1;
      }

      auto config = previous_high_low("previous_high_low", interval, type,
                                      transform_timeframe);
      auto transformBase = MAKE_TRANSFORM(config);
      auto phl_transform = dynamic_cast<ITransform *>(transformBase.get());

      auto start = std::chrono::high_resolution_clock::now();
      auto result = phl_transform->TransformData(df);
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration<double>(end - start);
      std::cout << "Took: " << duration.count() << " s" << std::endl;

      expected = expected.assign(
          "PreviousHigh", expected["PreviousHigh"].cast(arrow::float64()));
      expected = expected.assign(
          "PreviousLow", expected["PreviousLow"].cast(arrow::float64()));
      expected = expected.assign("BrokenHigh",
                                 expected["BrokenHigh"].cast(arrow::boolean()));
      expected = expected.assign("BrokenLow",
                                 expected["BrokenLow"].cast(arrow::boolean()));

      auto output_path = std::format("{}/test_data/{}/{}_{}_out.csv", dir,
                                     test_instrument, file_name, timeframe);
      auto status_is_ok =
          epoch_frame::write_csv_file(result.reset_index("t"), output_path)
              .ok();
      REQUIRE(status_is_ok);

      auto expected_df = expected.rename(
          {{"PreviousHigh", phl_transform->GetOutputId("previous_high")},
           {"PreviousLow", phl_transform->GetOutputId("previous_low")},
           {"BrokenHigh", phl_transform->GetOutputId("broken_high")},
           {"BrokenLow", phl_transform->GetOutputId("broken_low")}});
      for (const auto &column : expected_df.column_names()) {
        INFO("Column: " << column);
        INFO(expected_df[column].contiguous_array()->Diff(
            *result[column].contiguous_array().value()));

        CHECK(result[column].equals(expected_df[column]));
      }
    }
  }

  SECTION("Order Block") {
    auto file_name = "ob";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();

    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto shl = make_shl();

    auto config =
        order_blocks("ob", shl->GetOutputId("high_low"), false, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto ob_transform = dynamic_cast<ITransform *>(transformBase.get());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = shl->TransformData(df);
    df = df.assign(shl->GetOutputId("high_low"),
                   result[shl->GetOutputId("high_low")]);
    result = ob_transform->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected = expected.assign("OB", expected["OB"].cast(arrow::int64()));
    expected = expected.assign("MitigatedIndex",
                               expected["MitigatedIndex"].cast(arrow::int64()));

    auto expected_df = expected.rename(
        {{"OB", ob_transform->GetOutputId("ob")},
         {"Top", ob_transform->GetOutputId("top")},
         {"Bottom", ob_transform->GetOutputId("bottom")},
         {"OBVolume", ob_transform->GetOutputId("ob_volume")},
         {"MitigatedIndex", ob_transform->GetOutputId("mitigated_index")},
         {"Percentage", ob_transform->GetOutputId("percentage")}});
    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      INFO(expected_df[column].contiguous_array()->Diff(
          *result[column].contiguous_array().value()));

      CHECK(result[column].equals(expected_df[column]));
    }
  }

  SECTION("Fair Value Gap") {
    bool is_join_consecutive = GENERATE(false, true);
    SECTION(std::format("Join Consecutive: {}", is_join_consecutive)) {
      auto file_name = is_join_consecutive ? "fvg_consecutive" : "fvg";
      auto expected =
          read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                    std::filesystem::current_path().string(),
                                    test_instrument, file_name),
                        {})
              .ValueOrDie();

      auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
      auto config = fair_value_gap("fvg", is_join_consecutive, timeframe);
      auto transformBase = MAKE_TRANSFORM(config);
      auto fvg_transform = dynamic_cast<ITransform *>(transformBase.get());

      auto start = std::chrono::high_resolution_clock::now();
      auto result = fvg_transform->TransformData(df);
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration<double>(end - start);
      std::cout << "Took: " << duration.count() << " s" << std::endl;

      auto result_df = DataFrame{result};
      expected = expected.assign(
          "MitigatedIndex", expected["MitigatedIndex"].cast(arrow::int64()));
      expected = expected.assign("FVG", expected["FVG"].cast(arrow::int64()));

      auto expected_df = expected.rename(
          {{"FVG", fvg_transform->GetOutputId("fvg")},
           {"Top", fvg_transform->GetOutputId("top")},
           {"Bottom", fvg_transform->GetOutputId("bottom")},
           {"MitigatedIndex", fvg_transform->GetOutputId("mitigated_index")}});

      for (const auto &column : expected_df.column_names()) {
        INFO("Column: " << column);
        REQUIRE(result[column].equals(expected_df[column]));
      }
    }
  }

  SECTION("Swing Highs Lows") {
    auto file_name = "swing_highs_lows";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();
    auto shl = make_shl();

    auto start = std::chrono::high_resolution_clock::now();
    auto result = shl->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected =
        expected.assign("HighLow", expected["HighLow"].cast(arrow::int64()));

    auto expected_df =
        expected.rename({{"HighLow", shl->GetOutputId("high_low")},
                         {"Level", shl->GetOutputId("level")}});

    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      REQUIRE(result[column].equals(expected_df[column]));
    }
  }

  SECTION("BOS CHoCH") {
    auto file_name = "bos_choch";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();

    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto shl = make_shl();

    auto config = bos_choch("bos_choch", shl->GetOutputId("high_low"),
                            shl->GetOutputId("level"), true, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto bos_choch_transform = dynamic_cast<ITransform *>(transformBase.get());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = shl->TransformData(df);
    df = df.assign(shl->GetOutputId("high_low"),
                   result[shl->GetOutputId("high_low")])
             .assign(shl->GetOutputId("level"),
                     result[shl->GetOutputId("level")]);
    result = bos_choch_transform->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected = expected.assign("BOS", expected["BOS"].cast(arrow::int64()));
    expected = expected.assign("CHOCH", expected["CHOCH"].cast(arrow::int64()));
    expected = expected.assign("BrokenIndex",
                               expected["BrokenIndex"].cast(arrow::int64()));

    auto expected_df = expected.rename(
        {{"BOS", bos_choch_transform->GetOutputId("bos")},
         {"CHOCH", bos_choch_transform->GetOutputId("choch")},
         {"Level", bos_choch_transform->GetOutputId("level")},
         {"BrokenIndex", bos_choch_transform->GetOutputId("broken_index")}});
    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      INFO(expected_df[column].contiguous_array()->Diff(
          *result[column].contiguous_array().value()));

      CHECK(result[column].equals(expected_df[column]));
    }
  }

  SECTION("Liquidity") {
    auto file_name = "liquidity";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();

    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto shl = make_shl();

    auto config = liquidity("liquidity", shl->GetOutputId("high_low"),
                            shl->GetOutputId("level"), 0.01, timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto liquidity_transform = dynamic_cast<ITransform *>(transformBase.get());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = shl->TransformData(df);
    df = df.assign(shl->GetOutputId("high_low"),
                   result[shl->GetOutputId("high_low")])
             .assign(shl->GetOutputId("level"),
                     result[shl->GetOutputId("level")]);
    result = liquidity_transform->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected = expected.assign("Liquidity",
                               expected["Liquidity"].cast(arrow::float64()));
    expected = expected.assign("End", expected["End"].cast(arrow::float64()));
    expected =
        expected.assign("Swept", expected["Swept"].cast(arrow::float64()));

    auto expected_df = expected.rename(
        {{"Liquidity", liquidity_transform->GetOutputId("liquidity")},
         {"Level", liquidity_transform->GetOutputId("level")},
         {"End", liquidity_transform->GetOutputId("end")},
         {"Swept", liquidity_transform->GetOutputId("swept")}});

    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      INFO(expected_df[column].contiguous_array()->Diff(
          *result[column].contiguous_array().value()));

      CHECK(result[column].equals(expected_df[column]));
    }
  }

  SECTION("Retracements") {
    auto file_name = "retracements";
    auto expected =
        read_csv_file(std::format("{}/test_data/{}/{}_result_data.csv",
                                  std::filesystem::current_path().string(),
                                  test_instrument, file_name),
                      {})
            .ValueOrDie();

    auto timeframe = epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
    auto shl = make_shl();

    auto config = retracements("retracements", shl->GetOutputId("high_low"),
                               shl->GetOutputId("level"), timeframe);
    auto transformBase = MAKE_TRANSFORM(config);
    auto retracements_transform =
        dynamic_cast<ITransform *>(transformBase.get());

    auto start = std::chrono::high_resolution_clock::now();
    auto result = shl->TransformData(df);
    df = df.assign(shl->GetOutputId("high_low"),
                   result[shl->GetOutputId("high_low")])
             .assign(shl->GetOutputId("level"),
                     result[shl->GetOutputId("level")]);
    result = retracements_transform->TransformData(df);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Took: " << duration.count() << " s" << std::endl;

    expected = expected.assign("Direction",
                               expected["Direction"].cast(arrow::int64()));
    expected =
        expected.assign("CurrentRetracement%",
                        expected["CurrentRetracement%"].cast(arrow::float64()));
    expected =
        expected.assign("DeepestRetracement%",
                        expected["DeepestRetracement%"].cast(arrow::float64()));

    auto expected_df = expected.rename(
        {{"Direction", retracements_transform->GetOutputId("direction")},
         {"CurrentRetracement%",
          retracements_transform->GetOutputId("current_retracement")},
         {"DeepestRetracement%",
          retracements_transform->GetOutputId("deepest_retracement")}});

    for (const auto &column : expected_df.column_names()) {
      INFO("Column: " << column);
      INFO(expected_df[column].contiguous_array()->Diff(
          *result[column].contiguous_array().value()));

      CHECK(result[column].equals(expected_df[column]));
    }
  }
}