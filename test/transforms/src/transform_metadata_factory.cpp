//
// Created by adesola on 5/23/25.
//
#include "epoch_frame/array.h"
#include "epoch_metadata/strategy/registration.h"
#include "transforms/src/config_helper.h"
#include "transforms/src/cross_sectional/rank.h"
#include "transforms/src/cross_sectional/returns.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>

#include "epoch_metadata/transforms/registry.h"

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

TEST_CASE("Transform Metadata Factory") {
  auto metadataMap =
      epoch_metadata::transforms::ITransformRegistry::GetInstance()
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
    REQUIRE(metadataMap.size() == transformMap.size());
  }

  std::vector<double> closePrices{6, 5, 6, 5, 6, 5, 6, 5,   6, 5, 6,
                                  5, 6, 5, 6, 5, 6, 5, 5.5, 5, 5, 9};
  std::vector<double> openPrices{5, 6, 5, 6, 5, 6, 5, 6,   5, 6, 5,
                                 6, 5, 6, 5, 6, 5, 5, 5.5, 5, 5, 9};
  std::vector<double> highPrices{7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7,
                                 7, 7, 7, 7, 7, 7, 5, 10, 7, 8, 9};
  std::vector<double> lowPrices{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                4, 4, 4, 4, 4, 4, 2, 3, 4, 5, 9};
  std::vector<double> volume(closePrices.size(), 1);

  const std::unordered_map<std::string, arrow::ChunkedArrayPtr> dataSources{
        {"o", factory::array::make_array(openPrices)},
        {"c", factory::array::make_array(closePrices)},
        {"h", factory::array::make_array(highPrices)},
        {"l", factory::array::make_array(lowPrices)},
        {"v", factory::array::make_array(volume)}};
  const auto index = factory::index::date_range(
      {.start = DateTime::from_date_str("2022-01-01").timestamp(),
       .periods = static_cast<int64_t>(closePrices.size()),
       .offset = factory::offset::hours(6)});

  std::unordered_map<std::string, YAML::Node> optionOverrides;
  optionOverrides["psar"]["acceleration_factor_step"] = 0.02;
  optionOverrides["psar"]["acceleration_factor_maximum"] = 2;

  auto getArrayFromType = [&](auto const &type) {
    switch (type) {
      case IODataType::Any:
      case IODataType::Decimal:
      case IODataType::Number:
        return factory::array::make_array(closePrices);
      case IODataType::Integer:
        return factory::array::make_array(
            std::vector<int64_t>(closePrices.size(), 0));
      case IODataType::Boolean:
        return factory::array::make_array(
            std::vector<bool>(closePrices.size(), false));
      default:
        return factory::array::make_array(
            std::vector<std::string>(closePrices.size(), ""));
    }
    std::unreachable();
  };

  auto makeConfig = [&](auto const &id) {
    std::vector<arrow::ChunkedArrayPtr> inputs_vec;
    std::vector<std::string> fields_vec;
    YAML::Node config;
    config["type"] = id;
    config["id"] = "1";
    config["timeframe"]["interval"] = 1;
    config["timeframe"]["type"] = "day";

    const epoch_metadata::transforms::TransformsMetaData metadata =
        metadataMap.at(id);
    if (metadata.isCrossSectional) {
      if (metadata.inputs.size() == 1 &&
          metadata.inputs.front().allowMultipleConnections) {
        config["inputs"][epoch_metadata::ARG] = std::vector{"1#result"};
          } else {
            config["inputs"][epoch_metadata::ARG] = "1#result";
          }
      fields_vec = {"1#result"};
      inputs_vec = {getArrayFromType(metadata.inputs.front().type)};
    } else if (metadata.inputs.size() == 1 &&
      metadata.inputs.front().allowMultipleConnections) {
        config["inputs"][epoch_metadata::ARG] = std::vector{"1#result"};
        fields_vec = {"1#result"};
        inputs_vec.emplace_back(getArrayFromType(metadata.inputs.front().type));
      } else {
        for (auto const &[i, inputMetadata] :
         metadata.inputs | std::views::enumerate) {
          config["inputs"][inputMetadata.id] =
              fields_vec.emplace_back(std::to_string(i));
          inputs_vec.emplace_back(getArrayFromType(inputMetadata.type));
         }
      }

      for (auto const &[i, dataSource] :
           metadata.requiredDataSources | std::views::enumerate) {
        config["inputs"][dataSource] = fields_vec.emplace_back(dataSource);
        inputs_vec.emplace_back(dataSources.at(dataSource));
      }

    if (optionOverrides.contains(id)) {
      for (auto const &node : optionOverrides[id]) {
        config["options"][node.first.template as<std::string>()] =
            node.second.template as<double>();
      }
    } else {
      for (epoch_metadata::MetaDataOption const &optionMetadata :
           metadata.options) {
        auto optionId = optionMetadata.id;
        if (optionMetadata.type == MetaDataOptionType::Integer) {
          if (optionMetadata.id == "min_training_samples") {
            config["options"][optionId] = 1;
          }
          else {
            const auto defaultInteger =
    optionMetadata.defaultValue
        .value_or(epoch_metadata::MetaDataOptionDefinition{2.0})
        .GetInteger();
            if (optionId.contains("long")) {
              config["options"][optionId] =
                  optionMetadata.defaultValue
                      .value_or(epoch_metadata::MetaDataOptionDefinition{5.0})
                      .GetInteger();
            } else {
              config["options"][optionId] = defaultInteger;
            }
          }
        } else if (optionMetadata.type ==
                   epoch_core::MetaDataOptionType::Decimal) {
          config["options"][optionId] =
              optionMetadata.defaultValue
                  .value_or(epoch_metadata::MetaDataOptionDefinition{0.2})
                  .GetDecimal();
                   } else if (optionMetadata.type ==
                              epoch_core::MetaDataOptionType::Boolean) {
                     config["options"][optionId] =
                         optionMetadata.defaultValue
                             .value_or(epoch_metadata::MetaDataOptionDefinition{true})
                             .GetBoolean();
                              } else if (optionMetadata.type ==
                                         epoch_core::MetaDataOptionType::Select) {
                                REQUIRE(optionMetadata.selectOption.size() > 0);
                                config["options"][optionId] =
                                    optionMetadata.defaultValue
                                        .value_or(epoch_metadata::MetaDataOptionDefinition{
                                            optionMetadata.selectOption[0].value})
                                        .GetSelectOption();
                                         }
           }
    }

    return std::tuple{TransformDefinition{config}, std::move(fields_vec),
                      std::move(inputs_vec)};
  };

  for (auto const &[id, factory] : transformMap) {
    if (id == epoch_metadata::transforms::TRADE_SIGNAL_EXECUTOR_ID) {
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
        case IODataType::Decimal:
        case IODataType::Number:
          REQUIRE(col.dtype()->id() == arrow::Type::DOUBLE);
          break;
        case IODataType::Integer:
          REQUIRE(col.dtype()->id() == arrow::Type::INT64);
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