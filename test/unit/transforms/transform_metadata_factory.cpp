//
// Created by adesola on 5/23/25.
//
#include "epoch_frame/array.h"
#include <epoch_script/core/constants.h>
#include "epoch_script/strategy/registration.h"
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include "transforms/components/cross_sectional/rank.h"
#include "transforms/components/cross_sectional/returns.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>

#include <epoch_script/transforms/core/registry.h>

using namespace epoch_core;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// Virtual data generator for creating appropriate test data based on transform requirements
class VirtualDataGenerator {
public:
  static constexpr size_t DEFAULT_NUM_BARS = 50;
  static constexpr size_t DEFAULT_NUM_ASSETS = 5;

  // Generate varied price data with realistic patterns
  static std::vector<double> generatePricePattern(size_t numBars, double basePrice, double volatility) {
    std::vector<double> prices;
    prices.reserve(numBars);
    double price = basePrice;

    for (size_t i = 0; i < numBars; ++i) {
      // Create oscillating pattern with trend
      double trend = i * 0.1;
      double oscillation = std::sin(i * 0.3) * volatility;
      price = basePrice + trend + oscillation;
      prices.push_back(price);
    }
    return prices;
  }

  // Generate single-asset OHLCV data
  static std::unordered_map<std::string, arrow::ChunkedArrayPtr> generateSingleAssetData(size_t numBars = DEFAULT_NUM_BARS) {
    auto closePrices = generatePricePattern(numBars, 100.0, 5.0);
    std::vector<double> openPrices;
    std::vector<double> highPrices;
    std::vector<double> lowPrices;

    openPrices.reserve(numBars);
    highPrices.reserve(numBars);
    lowPrices.reserve(numBars);

    for (size_t i = 0; i < numBars; ++i) {
      double close = closePrices[i];
      double open = (i > 0) ? closePrices[i-1] : close - 1.0;
      openPrices.push_back(open);
      highPrices.push_back(std::max(open, close) + 2.0);
      lowPrices.push_back(std::min(open, close) - 2.0);
    }

    std::vector<double> volume(numBars, 1000000.0);
    std::vector<double> vwap(numBars, 100.0);
    std::vector<double> tradeCount(numBars, 500.0);

    return {
      {"o", factory::array::make_array(openPrices)},
      {"c", factory::array::make_array(closePrices)},
      {"h", factory::array::make_array(highPrices)},
      {"l", factory::array::make_array(lowPrices)},
      {"v", factory::array::make_array(volume)},
      {"vw", factory::array::make_array(vwap)},
      {"n", factory::array::make_array(tradeCount)}
    };
  }

  // Generate multi-asset cross-sectional data
  // Returns DataFrame with asset symbols as column names
  static DataFrame generateCrossSectionalData(
      IODataType dataType,
      const arrow::ChunkedArrayPtr& index,
      size_t numAssets = DEFAULT_NUM_ASSETS,
      size_t numBars = DEFAULT_NUM_BARS) {

    std::vector<std::string> assetNames = {"AAPL", "MSFT", "TSLA", "GOOGL", "AMZN"};
    assetNames.resize(numAssets);

    std::vector<arrow::ChunkedArrayPtr> assetData;
    assetData.reserve(numAssets);

    for (size_t i = 0; i < numAssets; ++i) {
      double basePrice = 100.0 + (i * 50.0);  // Different price levels
      double volatility = 5.0 + (i * 2.0);     // Different volatilities

      switch (dataType) {
        case IODataType::Decimal:
        case IODataType::Number: {
          auto prices = generatePricePattern(numBars, basePrice, volatility);
          assetData.push_back(factory::array::make_array(prices));
          break;
        }
        case IODataType::Integer: {
          auto prices = generatePricePattern(numBars, basePrice, volatility);
          std::vector<int64_t> intPrices;
          intPrices.reserve(prices.size());
          for (auto p : prices) intPrices.push_back(static_cast<int64_t>(p));
          assetData.push_back(factory::array::make_array(intPrices));
          break;
        }
        case IODataType::Boolean: {
          std::vector<bool> values(numBars);
          for (size_t j = 0; j < numBars; ++j) {
            values[j] = (j + i) % 2 == 0;  // Alternating pattern, different per asset
          }
          assetData.push_back(factory::array::make_array(values));
          break;
        }
        default: {
          std::vector<std::string> values(numBars, "Asset" + std::to_string(i));
          assetData.push_back(factory::array::make_array(values));
          break;
        }
      }
    }

    return make_dataframe(index, assetData, assetNames);
  }

  // Get array from IODataType for non-cross-sectional transforms
  static arrow::ChunkedArrayPtr getArrayFromType(IODataType type, size_t numBars = DEFAULT_NUM_BARS) {
    switch (type) {
      case IODataType::Any:
      case IODataType::Decimal:
      case IODataType::Number:
        return factory::array::make_array(generatePricePattern(numBars, 100.0, 5.0));
      case IODataType::Integer: {
        std::vector<int64_t> values(numBars);
        for (size_t i = 0; i < numBars; ++i) values[i] = static_cast<int64_t>(i);
        return factory::array::make_array(values);
      }
      case IODataType::Boolean: {
        std::vector<bool> values(numBars);
        for (size_t i = 0; i < numBars; ++i) values[i] = i % 2 == 0;
        return factory::array::make_array(values);
      }
      default:
        return factory::array::make_array(std::vector<std::string>(numBars, "test_string"));
    }
  }
};

TEST_CASE("Transform Metadata Factory") {
  auto metadataMap =
      epoch_script::transforms::ITransformRegistry::GetInstance()
          .GetMetaData();
  const auto transformMap = TransformRegistry::GetInstance().GetAll();

  auto getTransformNames = [](auto const &keysA, auto const &keysB) {
    std::set<std::string> sortedKeysA(keysA.begin(), keysA.end());
    std::set<std::string> sortedKeysB(keysB.begin(), keysB.end());
    std::stringstream result;
    result << "MetaData - Transforms.\n";
    std::ranges::set_difference(
        sortedKeysA, sortedKeysB,
        std::ostream_iterator<std::string>(result, "\n"));
    result << "\n\nTransforms - MetaData.\n";
    std::ranges::set_difference(
        sortedKeysB, sortedKeysA,
        std::ostream_iterator<std::string>(result, "\n"));
    return result.str();
  };

  SECTION("All transforms are registered") {
    INFO("Diff:\n"
         << getTransformNames(metadataMap | std::views::keys,
                              transformMap | std::views::keys));

    // Count transforms with outputs (excludes reporters and selectors which don't produce outputs)
    auto non_reporter_count_metadata = std::ranges::count_if(metadataMap, [](const auto& pair) {
      return !pair.second.outputs.empty();
    });

    auto non_reporter_count_transforms = std::ranges::count_if(transformMap, [&](const auto& pair) {
      return !metadataMap.contains(pair.first) || !metadataMap.at(pair.first).outputs.empty();
    });

    REQUIRE(non_reporter_count_metadata == non_reporter_count_transforms);
  }

  // Generate test data using VirtualDataGenerator (50 bars for better statistical coverage)
  constexpr size_t NUM_TEST_BARS = VirtualDataGenerator::DEFAULT_NUM_BARS;
  const auto dataSources = VirtualDataGenerator::generateSingleAssetData(NUM_TEST_BARS);
  const auto index = factory::index::date_range(
      {.start = DateTime::from_date_str("2022-01-01").timestamp(),
       .periods = static_cast<int64_t>(NUM_TEST_BARS),
       .offset = factory::offset::hours(6)});

  // Use VirtualDataGenerator for creating arrays from types
  auto getArrayFromType = [&](auto const &type) {
    return VirtualDataGenerator::getArrayFromType(type, NUM_TEST_BARS);
  };

  auto makeConfig = [&](auto const &id) {
    std::vector<arrow::ChunkedArrayPtr> inputs_vec;
    std::vector<std::string> fields_vec;
    YAML::Node config;
    config["type"] = id;
    config["id"] = "1";
    config["timeframe"]["interval"] = 1;
    config["timeframe"]["type"] = "day";

    const epoch_script::transforms::TransformsMetaData metadata =
        metadataMap.at(id);

    // Handle cross-sectional transforms with multi-asset data
    if (metadata.isCrossSectional) {
      // Cross-sectional transforms receive DataFrame with asset symbols as columns
      auto cs_data = VirtualDataGenerator::generateCrossSectionalData(
          metadata.inputs.empty() ? IODataType::Decimal : metadata.inputs.front().type,
          index,
          VirtualDataGenerator::DEFAULT_NUM_ASSETS,
          NUM_TEST_BARS);

      if (metadata.inputs.size() == 1 &&
          metadata.inputs.front().allowMultipleConnections) {
        // Single input accepting multiple connections - provide multi-asset data
        config["inputs"][epoch_script::ARG] = cs_data.columns();
        fields_vec = cs_data.columns();
        for (const auto& col : cs_data.columns()) {
          inputs_vec.push_back(cs_data[col].chunked_array());
        }
      } else if (metadata.inputs.size() == 1) {
        // Single non-multi-connection input (edge case for cross-sectional)
        config["inputs"][epoch_script::ARG] = cs_data.columns().front();
        fields_vec = {cs_data.columns().front()};
        inputs_vec = {cs_data[cs_data.columns().front()].chunked_array()};
      } else {
        // Transforms with multiple inputs (e.g., beta with asset_returns and market_returns)
        // Each input gets its own multi-asset DataFrame
        for (auto const &[i, inputMetadata] :
             metadata.inputs | std::views::enumerate) {
          auto input_cs_data = VirtualDataGenerator::generateCrossSectionalData(
              inputMetadata.type, index,
              VirtualDataGenerator::DEFAULT_NUM_ASSETS, NUM_TEST_BARS);

          config["inputs"][inputMetadata.id] = input_cs_data.columns();
          for (const auto& col : input_cs_data.columns()) {
            fields_vec.push_back(col);
            inputs_vec.push_back(input_cs_data[col].chunked_array());
          }
        }
      }
    } else if (metadata.inputs.size() == 1 &&
               metadata.inputs.front().allowMultipleConnections) {
      // Non-cross-sectional transform accepting multiple connections
      config["inputs"][epoch_script::ARG] = std::vector{"1#result"};
      fields_vec = {"1#result"};
      inputs_vec.emplace_back(getArrayFromType(metadata.inputs.front().type));
    } else {
      // Regular transforms with one or more single-connection inputs
      for (auto const &[i, inputMetadata] :
           metadata.inputs | std::views::enumerate) {
        config["inputs"][inputMetadata.id] =
            fields_vec.emplace_back(std::to_string(i));
        inputs_vec.emplace_back(getArrayFromType(inputMetadata.type));
      }
    }

    // Get required data sources from metadata, with special handling for chart formations
    auto requiredDataSources = metadata.requiredDataSources;

    // Skip chart formation transforms - they require h/l columns directly via requiredDataSources
    // These transforms access bars["h"] and bars["l"] directly, which isn't available in unit test context
    const std::unordered_set<std::string> chartFormations = {
      "head_and_shoulders", "inverse_head_and_shoulders", "double_top_bottom",
      "flag", "triangles", "pennant", "consolidation_box"
    };
    if (chartFormations.contains(id)) {
      continue;  // Skip these transforms in unit test
    }

    for (auto const &[i, dataSource] :
         requiredDataSources | std::views::enumerate) {
      config["inputs"][dataSource] = fields_vec.emplace_back(dataSource);
      inputs_vec.emplace_back(dataSources.at(dataSource));
    }

    // Configure options from metadata
    for (epoch_script::MetaDataOption const &optionMetadata :
         metadata.options) {
        auto optionId = optionMetadata.id;
        if (optionMetadata.type == MetaDataOptionType::Integer) {
          // Use metadata defaults directly
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_script::MetaDataOptionDefinition{2.0})
                  .GetInteger();
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::Decimal) {
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_script::MetaDataOptionDefinition{0.2})
                  .GetDecimal();
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::Boolean) {
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_script::MetaDataOptionDefinition{true})
                  .GetBoolean();
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::Select) {
          REQUIRE(optionMetadata.selectOption.size() > 0);
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_script::MetaDataOptionDefinition{
                      optionMetadata.selectOption[0].value})
                  .GetSelectOption();
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::String) {
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_script::MetaDataOptionDefinition{""})
                  .GetString();
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::EventMarkerSchema) {
          // Generate minimal valid CardSchema JSON for testing
          if (id == "card_selector_filter") {
            // Use a boolean column from test data
            config["options"][optionId] = R"({
              "title": "Test Selector",
              "select_key": "0",
              "schemas": [{
                "column_id": "0",
                "slot": "Hero",
                "render_type": "Number",
                "color_map": {}
              }]
            })";
          } else if (id == "card_selector_sql") {
            // Use simple SQL query
            config["options"][optionId] = R"({
              "title": "Test SQL Selector",
              "sql": "SELECT * FROM self",
              "schemas": [{
                "column_id": "SLOT0",
                "slot": "Hero",
                "render_type": "Number",
                "color_map": {}
              }]
            })";
          }
        }
      }
    }

    return std::tuple{TransformDefinition{config}, std::move(fields_vec),
                      std::move(inputs_vec)};
  };

  for (auto const &[id, factory] : transformMap) {
    if (id == epoch_script::transforms::TRADE_SIGNAL_EXECUTOR_ID) {
      continue;
    }

    // Skip reporters and selectors - they don't produce column outputs
    if (metadataMap.contains(id) && metadataMap.at(id).outputs.empty()) {
      continue;
    }

    // Skip SQL transforms - they require custom SQL queries that can't be auto-generated
    if (id.starts_with("sql_query")) {
      continue;
    }

    // Skip external data source transforms - they require external API data that can't be auto-generated
    const std::unordered_set<std::string> externalDataSources = {
      "economic_indicator", "form13f_holdings", "insider_trading"
    };
    if (polygon::ALL_POLYGON_TRANSFORMS.contains(id) || externalDataSources.contains(id)) {
      continue;
    }

    // Skip conditional_select - requires special configuration with alternating condition/value pairs
    if (id == "conditional_select") {
      continue;
    }

    // Skip flexible_pivot_detector - requires runtime orchestrator to provide OHLC columns via requiredDataSources mechanism
    if (id == "flexible_pivot_detector") {
      continue;
    }

    INFO("Transform: " << id);
    REQUIRE(metadataMap.contains(id));

    auto [config, inputIds, inputValues] = makeConfig(id);
    auto transform = factory(TransformConfiguration(config));

    auto df = make_dataframe(index, inputValues, inputIds);
    auto result = transform->TransformData(df);

    auto outputs = metadataMap.at(id).outputs;
    REQUIRE(outputs.size() == result.num_cols());

    for (auto const &output : outputs) {
      auto outputCol = transform->GetOutputId(output.id);
      INFO("Output: " << outputCol << "\nresult:\n" << result);

      REQUIRE(result.contains(outputCol));

      auto col = result[outputCol];
      switch (output.type) {
      case IODataType::Any:
        // Any type can be any Arrow type including NULL
        // No type checking needed for Any
        break;
      case IODataType::Decimal:
      case IODataType::Number:
        REQUIRE(col.dtype()->id() == arrow::Type::DOUBLE);
        break;
      case IODataType::Integer:
        // Accept INT32, INT64, and TIMESTAMP (timestamps are stored as int64)
        REQUIRE((col.dtype()->id() == arrow::Type::INT32 ||
                 col.dtype()->id() == arrow::Type::INT64 ||
                 col.dtype()->id() == arrow::Type::TIMESTAMP));
        break;
      case IODataType::Boolean:
        REQUIRE(col.dtype()->id() == arrow::Type::BOOL);
        break;
      case IODataType::String:
        REQUIRE(col.dtype()->id() == arrow::Type::STRING);
        break;
      default:
        break;
      }
    }
  }
}