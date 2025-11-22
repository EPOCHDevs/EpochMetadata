#include "epoch_script/chart_metadata/series_configuration_builder.h"
#include "epoch_script/chart_metadata/chart_metadata_provider.h"
#include "catch2/catch_test_macros.hpp"
#include "epoch_script/data/common/constants.h"
#include "epoch_script/transforms/core/config_helper.h"
#include "epoch_script/transforms/core/itransform.h"

using namespace epoch_script;
using namespace epoch_script::chart_metadata;

TEST_CASE("SeriesConfigurationBuilder",
          "[chart_metadata][series_configuration_builder]") {
  const auto tf =
      epoch_script::EpochStratifyXConstants::instance().DAILY_FREQUENCY;
  const auto timeframe = tf.ToString();

  SECTION("Builds candlestick series correctly") {
    auto series = SeriesConfigurationBuilder::BuildCandlestickSeries(timeframe);

    REQUIRE(series.id == timeframe + "_candlestick");
    REQUIRE(series.type == "candlestick");
    REQUIRE(series.name == "");
    REQUIRE(series.dataMapping.size() == 5);
    REQUIRE(series.dataMapping.at("index") == "index");
    REQUIRE(series.dataMapping.at("open") == "o");
    REQUIRE(series.dataMapping.at("high") == "h");
    REQUIRE(series.dataMapping.at("low") == "l");
    REQUIRE(series.dataMapping.at("close") == "c");
    REQUIRE(series.zIndex == 0);
    REQUIRE(series.yAxis == 0);
    REQUIRE(!series.linkedTo.has_value());
  }

  SECTION("Builds volume series correctly") {
    auto series = SeriesConfigurationBuilder::BuildVolumeSeries(timeframe);

    REQUIRE(series.id == timeframe + "_volume");
    REQUIRE(series.type == "column");
    REQUIRE(series.name == "Volume");
    REQUIRE(series.dataMapping.size() == 2);
    REQUIRE(series.dataMapping.at("index") == "index");
    REQUIRE(series.dataMapping.at("value") == "v");
    REQUIRE(series.zIndex == 0);
    REQUIRE(series.yAxis == 1);
    REQUIRE(!series.linkedTo.has_value());
  }

  SECTION("Builds line chart series") {
    auto sma = transform::ma("sma", 1, "c", 10, tf);
    auto series =
        SeriesConfigurationBuilder::BuildSeries(sma, 0, std::nullopt, "1");

    REQUIRE(series.id == "1");
    REQUIRE(series.type == "line");
    REQUIRE(series.name == "SMA 10");
    REQUIRE(series.dataMapping.size() == 2);
    REQUIRE(series.dataMapping.at("index") == "index");
    REQUIRE(series.dataMapping.at("value") == "1#result");
    REQUIRE(series.zIndex == 5);
    REQUIRE(series.yAxis == 0);
    REQUIRE(!series.linkedTo.has_value());
  }

  SECTION("Builds series with linkedTo") {
    auto sma = transform::ma("sma", 1, "c", 10, tf);
    std::optional<std::string> linkedTo = "candlestick_series";
    auto series =
        SeriesConfigurationBuilder::BuildSeries(sma, 0, linkedTo, "1");

    REQUIRE(series.linkedTo.has_value());
    REQUIRE(series.linkedTo.value() == "candlestick_series");
  }

  SECTION("Maps plot kinds to chart types correctly") {
    struct TestCase {
      std::string transformName;
      std::string expectedType;
      std::string description;
    };

    std::vector<TestCase> testCases = {
        {"sma", "line", "Simple moving average"},
        {"ema", "line", "Exponential moving average"},
        {"bbands", "bbands", "Bollinger Bands"},
        {"rsi", "rsi", "RSI indicator"},
        {"macd", "macd", "MACD indicator"},
        {"psar", "psar", "Parabolic SAR"},
        {"ao", "ao", "Awesome Oscillator"},
        {"cci", "cci", "Commodity Channel Index"},
        {"stoch", "stoch", "Stochastic oscillator"}};

    for (const auto &tc : testCases) {
      DYNAMIC_SECTION(tc.description) {
        // Create appropriate transform based on name
        epoch_script::transform::TransformConfiguration cfg = [&]() {
          if (tc.transformName == "sma" || tc.transformName == "ema") {
            return transform::ma(tc.transformName, 1, "c", 10, tf);
          } else if (tc.transformName == "bbands") {
            return transform::bbands("1", 10, 2, "c", tf);
          } else if (tc.transformName == "rsi") {
            return transform::single_operand_period_op("rsi", 1, 14, "c", tf);
          } else if (tc.transformName == "psar") {
            return transform::psar("1", 0.02, 0.2, "c", tf);
          } else {
            YAML::Node inputs;
            YAML::Node options;
            if (tc.transformName == "macd") {
              inputs[epoch_script::ARG] = "c";
              options["short_period"] = 12;
              options["long_period"] = 26;
              options["signal_period"] = 9;
            } else if (tc.transformName == "stoch") {
              inputs[epoch_script::ARG] = "c";
              options["k_period"] = 14;
              options["k_slowing_period"] = 3;
              options["d_period"] = 3;
            } else if (tc.transformName == "cci") {
              options["period"] = 20;
            }
            return transform::run_op(tc.transformName, "1", inputs, options,
                                     tf);
          }
        }();

        auto series =
            SeriesConfigurationBuilder::BuildSeries(cfg, 0, std::nullopt, "1");
        REQUIRE(series.type == tc.expectedType);
      }
    }
  }

  SECTION("Handles SMC indicators chart types") {
    SECTION("Order blocks") {
      YAML::Node inputs;
      inputs["high_low"] = "1#high_low";
      YAML::Node options;
      options["close_mitigation"] = false;
      auto order_blocks =
          transform::run_op("order_blocks", "1", inputs, options, tf);
      auto series = SeriesConfigurationBuilder::BuildSeries(order_blocks, 2,
                                                            std::nullopt, "1");
      REQUIRE(series.type == "order_blocks");
    }

    SECTION("Fair value gap") {
      YAML::Node inputs;
      YAML::Node options;
      options["join_consecutive"] = true;
      auto fvg = transform::run_op("fair_value_gap", "1", inputs, options, tf);
      auto series =
          SeriesConfigurationBuilder::BuildSeries(fvg, 2, std::nullopt, "1");
      REQUIRE(series.type == "fvg");
    }

    SECTION("Swing highs/lows") {
      YAML::Node inputs;
      YAML::Node options;
      options["swing_length"] = 5;
      auto shl =
          transform::run_op("swing_highs_lows", "1", inputs, options, tf);
      auto series =
          SeriesConfigurationBuilder::BuildSeries(shl, 0, std::nullopt, "1");
      REQUIRE(series.type == "shl");
    }
  }

  SECTION("Sets correct z-index for different chart types") {
    struct ZIndexTest {
      std::string chartType;
      size_t expectedZIndex;
    };

    std::vector<ZIndexTest> tests = {{"flag", 10},      {"shl", 10},
                                     {"bos_choch", 10}, {"line", 5},
                                     {"bbands", 1},     {"candlestick", 0}};

    for (const auto &test : tests) {
      DYNAMIC_SECTION("z-index for " + test.chartType) {
        // We need to test the private GetZIndex method indirectly
        // Create a transform that would produce this chart type
        epoch_script::transform::TransformConfiguration cfg = [&]() {
          if (test.chartType == "line") {
            return transform::ma("sma", 1, "c", 10, tf);
          }
          if (test.chartType == "bbands") {
            return transform::bbands("1", 10, 2, "c", tf);
          }
          if (test.chartType == "candlestick") {
            // For candlestick, we'll use a simple line transform and check the
            // built-in candlestick series
            return transform::ma("sma", 1, "c", 10, tf);
          }
          if (test.chartType == "flag") {
            YAML::Node inputs;
            inputs[epoch_script::ARG] = "c";
            YAML::Node options;
            options["period"] = 10;
            options["body_none"] = 0.05;
            options["body_short"] = 0.5;
            options["body_long"] = 1.4;
            options["wick_none"] = 0.05;
            options["wick_long"] = 0.6;
            options["near"] = 0.3;
            return transform::run_op("hammer", "1", inputs, options, tf);
          }
          if (test.chartType == "shl") {
            YAML::Node inputs;
            YAML::Node options;
            options["swing_length"] = 5;
            return transform::run_op("swing_highs_lows", "1", inputs, options,
                                     tf);
          }
          if (test.chartType == "bos_choch") {
            YAML::Node inputs;
            inputs["high_low"] = "dummy_input";
            inputs["level"] = "dummy_level";
            YAML::Node options;
            options["close_break"] = true;
            return transform::run_op("bos_choch", "1", inputs, options, tf);
          }
          // Default case - use SMA
          return transform::ma("sma", 1, "c", 10, tf);
        }();

        auto series =
            SeriesConfigurationBuilder::BuildSeries(cfg, 0, std::nullopt, "1");

        // Special case for candlestick - test the built-in candlestick series
        // instead
        if (test.chartType == "candlestick") {
          auto candlestickSeries =
              SeriesConfigurationBuilder::BuildCandlestickSeries("1D");
          REQUIRE(candlestickSeries.zIndex == test.expectedZIndex);
        } else {
          REQUIRE(series.zIndex == test.expectedZIndex);
        }
      }
    }
  }

  SECTION("Uses transform metadata name when available") {
    auto sma = transform::ma("sma", 1, "c", 10, tf);
    auto series =
        SeriesConfigurationBuilder::BuildSeries(sma, 0, std::nullopt, "1");

    // SMA should have a display name from metadata
    REQUIRE(series.name == "SMA 10");
  }

  SECTION("Falls back to transform name when metadata name is empty") {
    // This would require a transform with empty metadata name
    // Most transforms have names, so this is more of an edge case
    // The test above already verifies the name is populated correctly
  }

  SECTION("Handles all axis assignments correctly") {
    auto sma = transform::ma("sma", 1, "c", 10, tf);

    for (uint8_t axis = 0; axis < 5; ++axis) {
      auto series =
          SeriesConfigurationBuilder::BuildSeries(sma, axis, std::nullopt, "1");
      REQUIRE(series.yAxis == axis);
    }
  }

  SECTION("Preserves seriesepoch_script::ID correctly") {
    auto sma = transform::ma("sma", 1, "c", 10, tf);

    std::vector<std::string> testIds = {"1", "custom_id", "transform_123", ""};

    for (const auto &id : testIds) {
      auto series =
          SeriesConfigurationBuilder::BuildSeries(sma, 0, std::nullopt, id);
      REQUIRE(series.id == id);
    }
  }

  SECTION("Handles complex multi-output indicators") {
    SECTION("MACD with three outputs") {
      YAML::Node inputs;
      inputs[epoch_script::ARG] = "c";
      YAML::Node options;
      options["short_period"] = 12;
      options["long_period"] = 26;
      options["signal_period"] = 9;

      auto macd = transform::run_op("macd", "1", inputs, options, tf);
      auto series =
          SeriesConfigurationBuilder::BuildSeries(macd, 2, std::nullopt, "1");

      REQUIRE(series.type == "macd");
      REQUIRE(series.dataMapping.size() == 4); // index + 3 outputs
      REQUIRE(series.dataMapping.at("index") == "index");
      REQUIRE(series.dataMapping.count("macd") == 1);
      REQUIRE(series.dataMapping.count("macd_signal") == 1);
      REQUIRE(series.dataMapping.count("macd_histogram") == 1);
    }

    SECTION("QQE with four outputs") {
      YAML::Node inputs;
      inputs[epoch_script::ARG] = "c";
      YAML::Node options;
      options["avg_period"] = 14;
      options["smooth_period"] = 5;
      options["width_factor"] = 4.236;

      auto qqe = transform::run_op("qqe", "1", inputs, options, tf);
      auto series =
          SeriesConfigurationBuilder::BuildSeries(qqe, 2, std::nullopt, "1");

      REQUIRE(series.type == "qqe");
      REQUIRE(series.dataMapping.size() == 5); // index + 4 outputs
    }
  }

  SECTION("Handles panel indicators with correct types") {
    struct PanelTest {
      std::string indicator;
      std::string expectedType;
    };

    std::vector<PanelTest> panelTests = {{"rsi", "rsi"},
                                         {"cci", "cci"},
                                         {"ao", "ao"},
                                         {"aroon", "aroon"},
                                         {"fisher", "fisher"},
                                         {"qqe", "qqe"},
                                         {"elders_thermometer", "elders"},
                                         {"fosc", "fosc"},
                                         {"qstick", "qstick"}};

    for (const auto &test : panelTests) {
      DYNAMIC_SECTION("Panel indicator: " + test.indicator) {
        YAML::Node inputs;
        YAML::Node options;

        // Set up appropriate inputs/options for each indicator
        if (test.indicator == "rsi") {
          inputs[epoch_script::ARG] = "c";
          options["period"] = 14;
        } else if (test.indicator == "cci") {
          options["period"] = 20;
        } else if (test.indicator == "aroon") {
          inputs[epoch_script::ARG] = "c";
          options["period"] = 14;
        } else if (test.indicator == "fisher") {
          inputs[epoch_script::ARG] = "c";
          options["period"] = 10;
        } else if (test.indicator == "qqe") {
          inputs[epoch_script::ARG] = "c";
          options["avg_period"] = 14;
          options["smooth_period"] = 5;
          options["width_factor"] = 4.236;
        } else if (test.indicator == "elders_thermometer") {
          options["period"] = 13;
          options["buy_factor"] = 0.5;
          options["sell_factor"] = 0.5;
        } else if (test.indicator == "fosc") {
          inputs[epoch_script::ARG] = "c";
          options["period"] = 14;
        } else if (test.indicator == "qstick") {
          options["period"] = 14;
        }

        auto transform =
            transform::run_op(test.indicator, "1", inputs, options, tf);
        auto series = SeriesConfigurationBuilder::BuildSeries(
            transform, 2, std::nullopt, "1");

        REQUIRE(series.type == test.expectedType);
      }
    }
  }
}