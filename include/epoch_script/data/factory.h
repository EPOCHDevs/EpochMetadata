#pragma once
//
// Created by dewe on 1/29/23.
//
#include "data/database/database.h"
#include "data/database/resample.h"
#include "data/database/updates/iwebsocket_manager.h"
#include "data/futures_continuation/continuations.h"

// Use epoch_data_sdk for all dataloader infrastructure
#include <epoch_data_sdk/dataloader/dataloader.hpp>
#include <epoch_data_sdk/dataloader/options.hpp>

#include <epoch_script/strategy/data_options.h>
#include <epoch_script/strategy/date_period_config.h>
#include <epoch_script/strategy/strategy_config.h>
#include <epoch_script/transforms/runtime/iorchestrator.h>
#include <utility>

namespace epoch_script::data {

// Namespace aliases for epoch_data_sdk types
using IDataLoader = data_sdk::IDataLoader;
using IDataLoaderPtr = std::shared_ptr<IDataLoader>;
using DataloaderOption = data_sdk::DataLoaderOptions;

class WebSocketManagerSingleton {
public:
  static WebSocketManagerSingleton &instance() {
    static WebSocketManagerSingleton instance;
    return instance;
  }

  IWebSocketManagerPtr GetWebSocketManager(AssetClass assetClass);

private:
  WebSocketManagerSingleton();

  asset::AssetClassMap<IWebSocketManagerPtr> m_webSocketManager;
};

struct DataModuleOption {
  DataloaderOption loader;

  std::optional<FuturesContinuation::Input> futureContinuation = {};

  std::vector<epoch_script::TimeFrame> barResampleTimeFrames = {};

  epoch_script::transform::TransformConfigurationList transformsConfigList{};

  bool liveUpdates = false;
};

namespace factory {
class DataModuleFactory {
public:
  explicit DataModuleFactory(DataModuleOption option)
      : m_option(std::move(option)) {}

  // NOTE: CreateDataStreamer removed for EpochScript - not needed for backtesting
  // Only Database is needed for test framework

  std::unique_ptr<Database> CreateDatabase();

  IDataLoaderPtr CreateDataloader();

  asset::AssetClassMap<IWebSocketManagerPtr> CreateWebSocketManager();

  epoch_script::runtime::IDataFlowOrchestrator::Ptr CreateTransforms();

  IFuturesContinuationConstructor::Ptr CreateFutureContinuations();

  IResamplerPtr CreateResampler();

  [[nodiscard]] DataModuleOption GetOption() const { return m_option; }

  ~DataModuleFactory() = default;

protected:
  DataModuleOption m_option;
};

using DataModuleFactoryPtr = std::unique_ptr<DataModuleFactory>;
} // namespace factory

namespace factory {
// Extract auxiliary data categories from transform configurations
// Uses central GetDataCategoryForTransform() from data_category_mapper
std::vector<DataCategory>
ExtractAuxiliaryCategoriesFromTransforms(
    epoch_script::transform::TransformConfigurationList const &configs);

void ProcessConfigurations(
    std::vector<std::unique_ptr<
        epoch_script::transform::TransformConfiguration>> const &configs,
    epoch_script::TimeFrame const &baseTimeframe,
    DataModuleOption &dataModuleOption);

DataModuleOption
MakeDataModuleOption(CountryCurrency baseCurrency,
                     epoch_script::strategy::DatePeriodConfig const &period,
                     epoch_script::strategy::DataOption const &config,
                     DataCategory primaryCategory,
                     std::vector<DataCategory> const &auxiliaryCategories = {});

// Strategy-aware factory - auto-detects primaryCategory from StrategyConfig
// by checking if any component requires intraday data
DataModuleOption
MakeDataModuleOptionFromStrategy(CountryCurrency baseCurrency,
                                 epoch_script::strategy::DatePeriodConfig const &period,
                                 epoch_script::strategy::StrategyConfig const &strategyConfig);

std::array<asset::AssetHashSet, 3>
MakeAssets(epoch_core::CountryCurrency baseCurrency,
           std::vector<std::string> const &assetIds, bool hasContinuation);
} // namespace factory
} // namespace epoch_script::data