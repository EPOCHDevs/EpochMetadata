//
// Created by dewe on 1/29/23.
//
#include "../../include/epoch_script/data/database/database.h"
#include "data/database/resample.h"
#include "../../include/epoch_script/data/database/updates/iwebsocket_manager.h"
#include "data/futures_continuation/continuations.h"
#include <epoch_script/data/factory.h>
#include <epoch_script/common/env_loader.h>
#include "data/database/database_impl.h"
#include "data/database/updates/alpaca_websocket_manager.h"

// Use epoch_data_sdk for all dataloader infrastructure
#include <epoch_data_sdk/dataloader/dataloader.hpp>
#include <epoch_data_sdk/dataloader/factory.hpp>

#include "epoch_script/core/constants.h"
#include "epoch_script/strategy/introspection.h"
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/model/asset/asset_specification.hpp>
#include <epoch_data_sdk/model/asset/index_constituents.hpp>
#include <epoch_data_sdk/model/builder/asset_builder.hpp>
#include <epoch_script/transforms/runtime/iorchestrator.h>
#include "transforms/components/data_sources/data_category_mapper.h"
#include <cctype>
#include <cstring>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <variant>

namespace epoch_script::data {

// Namespace aliases for epoch_data_sdk
namespace asset = data_sdk::asset;

// Import asset builder functions
using asset::MakeAsset;
using asset::AssetSpecificationQuery;

IWebSocketManagerPtr
WebSocketManagerSingleton::GetWebSocketManager(AssetClass assetClass) {
  try {
    return m_webSocketManager.at(assetClass);
  } catch (std::out_of_range const &e) {
    throw std::runtime_error(std::format(
        "WebSocket manager for asset class {} not found",
        epoch_core::AssetClassWrapper::ToLongFormString(assetClass)));
  }
}

WebSocketManagerSingleton::WebSocketManagerSingleton() {
  m_webSocketManager = asset::AssetClassMap<IWebSocketManagerPtr>();
  // NOTE: AlpacaWebSocketManager disabled - not used for backtesting
  // Re-enable when live trading support is needed
  // for (auto assetClass :
  //      {epoch_core::AssetClass::Stocks, epoch_core::AssetClass::Crypto}) {
  //   m_webSocketManager[assetClass] = std::make_unique<AlpacaWebSocketManager>(
  //       AlpacaWebSocketManagerOptions{.assetClass = assetClass,
  //                                     .key = ENV("ALPACA_API_KEY").c_str(),
  //                                     .secret = ENV("ALPACA_API_SECRET").c_str()});
  // }
}

namespace factory {

// NOTE: CreateDataStreamer removed for EpochScript - not needed for backtesting
// DataStreamer was only needed for StratifyX's campaign runner

IDataLoaderPtr CreateDataloader(DataModuleOption const& option) {
  // Use epoch_data_sdk factory to create API+cache dataloader
  SPDLOG_INFO("Creating API+cache dataloader from epoch_data_sdk");
  return data_sdk::dataloader::CreateApiCacheDataLoader(
      option.loader, EPOCH_DB_S3);
}

IFuturesContinuationConstructor::Ptr CreateFutureContinuations(DataModuleOption const& option) {
  auto futuresContinuation = option.futureContinuation;
  if (!futuresContinuation) {
    return {};
  }

  return std::make_unique<FuturesContinuationConstructor>(
      std::make_unique<FuturesContinuation>(
          MakeRolloverMethod(futuresContinuation->rollover,
                             futuresContinuation->arg),
          MakeAdjustmentMethod(futuresContinuation->type)));
}

epoch_script::runtime::IDataFlowOrchestrator::Ptr CreateTransforms(DataModuleOption const& option) {
  // Extract asset IDs from strategy assets
  std::set<std::string> assetIds;
  for (const auto& asset : option.loader.strategyAssets) {
    assetIds.insert(asset.GetID());
  }

  return epoch_script::runtime::CreateDataFlowRuntimeOrchestrator(
      assetIds,
      option.transformsConfigList);
}

IResamplerPtr CreateResampler(DataModuleOption const& option) {
    if (option.barResampleTimeFrames.empty()) {
        return {};
    }

    return std::make_unique<Resampler>(option.barResampleTimeFrames,
                                       option.loader.GetDataCategory() ==
                                           DataCategory::MinuteBars);
}

asset::AssetClassMap<IWebSocketManagerPtr> CreateWebSocketManager() {
  // if (!m_option.liveUpdates) {
    return {};
  // }
  //
  // asset::AssetClassMap<IWebSocketManagerPtr> result;
  // for (auto [assetClass, assets] :
  //      asset::ExtractAssetsByAssetClass(m_option.loader.dataloaderAssets)) {
  //   result
  //       .insert_or_assign(assetClass, WebSocketManagerSingleton::instance()
  //                                         .GetWebSocketManager(assetClass))
  //       .first->second->Subscribe(assets);
  // }
  // return result;
}

    DataModuleFactory::DataModuleFactory(DataModuleOption option)
    : m_option(std::move(option)) {}

    DataModuleOption DataModuleFactory::GetOption() const { return m_option; }


std::unique_ptr<Database> DataModuleFactory::CreateDatabase() {
  return std::make_unique<Database>(
      std::make_unique<DatabaseImpl>(DatabaseImplOptions{
          .dataloader = CreateDataloader(m_option),
          .dataTransform = CreateTransforms(m_option),
          .futuresContinuationConstructor = CreateFutureContinuations(m_option),
          .resampler = CreateResampler(m_option),
          .websocketManager = CreateWebSocketManager()}));
}

std::array<asset::AssetHashSet, 3>
MakeAssets(epoch_core::CountryCurrency baseCurrency,
           std::vector<std::string> const &assetIds, bool hasContinuation) {
  asset::AssetHashSet dataloaderAssets, strategyAssets, continuationAssets;
  for (auto id : assetIds) {
    using namespace asset;

    // Check if asset ID is an index that needs constituent expansion
    // Only check for indices if the ID does not contain a dash (-)
    if (id.find('-') == std::string::npos) {
      const auto &indexDB = asset::IndexConstituentsDatabase::GetInstance();
      auto constituentsOpt = indexDB.GetConstituents(id);

      if (constituentsOpt.has_value()) {
        const auto &constituents = constituentsOpt.value();
        SPDLOG_INFO("Expanding index {} to {} constituents", id, constituents.size());

        for (const auto &constituentId : constituents) {
          try {
            const auto constituentAsset = MakeAsset(AssetSpecificationQuery{constituentId});
            dataloaderAssets.insert(constituentAsset);
            strategyAssets.insert(constituentAsset);
          } catch (const std::exception &e) {
            SPDLOG_WARN("Failed to create asset for constituent {}: {}", constituentId, e.what());
          }
        }
        continue; // Skip adding the index itself
      }
    }

    // Not an index, create asset normally
    const auto asset = MakeAsset(AssetSpecificationQuery{id});

    if (asset.IsFuturesContract()) {
      // we need to load the futures contract
      dataloaderAssets.insert(asset);

      if (hasContinuation) {
        auto continuationAsset = asset.MakeFuturesContinuation();
        continuationAssets.insert(continuationAsset);
        strategyAssets.insert(continuationAsset);
      }
      continue;
    }
    if (asset.IsFX() or asset.IsCrypto()) {
      auto [base, counter] = asset.GetCurrencyPair();
      if (base != baseCurrency && counter != baseCurrency) {
        for (auto const &currency : {base, counter}) {
          const epoch_script::Symbol symbol{
              std::format("^{}{}", currency.ToString(),
                          CountryCurrencyWrapper::ToString(baseCurrency))};
          const asset::Asset newAsset =
              MakeAsset(symbol.get(), asset.GetAssetClass(),
                        asset.GetExchange(), asset.GetCurrency());
          if (!dataloaderAssets.contains(newAsset)) {
            SPDLOG_INFO(
                "Added {} to list of Dataloader assets for FX conversion.",
                symbol.get());
            dataloaderAssets.insert(newAsset);
          }
        }
      }
    }
    dataloaderAssets.insert(asset);
    strategyAssets.insert(asset);
  }
  return {dataloaderAssets, strategyAssets, continuationAssets};
}

std::optional<FuturesContinuationInput> MakeContinuations(
    asset::AssetHashSet const &assets,
    std::optional<epoch_script::strategy::TemplatedGenericFunction<
        epoch_core::RolloverType>> const &config) {
    if (assets.empty() || !config ||
        config->type == epoch_core::RolloverType::Null) {
        return std::nullopt;
        }

    FuturesContinuationInput continuationOption{
        .rollover = config->type,
        .type = epoch_core::lookupDefault(config->args, "adjustment", epoch_script::MetaDataOptionDefinition{"BackwardRatio"})
                    .GetSelectOption<AdjustmentType>(),
        .arg = 0};

    AssertFromFormat(continuationOption.rollover !=
                         epoch_core::RolloverType::Null,
                     "epoch_core::RolloverType is Null");
    if (continuationOption.rollover == epoch_core::RolloverType::LiquidityBased) {
        continuationOption.arg = static_cast<int>(
            epoch_core::lookupDefault(config->args, "ratio", epoch_script::MetaDataOptionDefinition{0.3}).GetDecimal() * 100);
    } else {
        continuationOption.arg = static_cast<int>(
            epoch_core::lookupDefault(config->args, "offset", epoch_script::MetaDataOptionDefinition{0.0}).GetInteger());
    }

    return continuationOption;
}

// NOTE: Typed config functions (CreateFinancialsConfig, CreateMacroEconomicsConfig, CreateAlternativeDataConfig)
// have been removed as the new data_sdk no longer uses typed configurations.
// All categories now use std::monostate in AuxiliaryCategoryConfig.

std::vector<DataCategory>
ExtractAuxiliaryCategoriesFromTransforms(
    epoch_script::transform::TransformConfigurationList const &configs) {

  // Use a set to deduplicate categories
  std::set<DataCategory> categorySet;

  for (auto const &config : configs) {
    auto const &metadata = config.GetTransformDefinition().GetMetadata();

    // Only process DataSource transforms
    if (metadata.category != epoch_core::TransformCategory::DataSource) {
      continue;
    }

    // Get the transform type/id and map to DataCategory using central mapper
    std::string transformType = config.GetTransformName();
    auto category = epoch_script::data_sources::GetDataCategoryForTransform(transformType);

    if (category.has_value()) {
      // Skip if it's a time-series category (those are primary, not auxiliary)
      if (!IsTimeSeriesCategory(*category)) {
        categorySet.insert(*category);
      }
    }
  }

  // Convert set to vector
  return std::vector<DataCategory>(categorySet.begin(), categorySet.end());
}

void ProcessConfigurations(
    std::vector<std::unique_ptr<
        epoch_script::transform::TransformConfiguration>> const
        &configurations,
    epoch_script::TimeFrame const &baseTimeframe,
    DataModuleOption &dataModuleOption) {

  for (auto const &definition : configurations) {
    dataModuleOption.transformsConfigList.emplace_back(*definition);
    auto timeframe = definition->GetTimeframe();
    if (timeframe != baseTimeframe) {
      dataModuleOption.barResampleTimeFrames.emplace_back(timeframe);
    }
  }

  // Auto-detect and add auxiliary categories from DataSource transforms
  auto detectedCategories = ExtractAuxiliaryCategoriesFromTransforms(
      dataModuleOption.transformsConfigList);

  // Add detected auxiliary categories to the existing categories set
  dataModuleOption.loader.categories.insert(detectedCategories.begin(), detectedCategories.end());
}

DataModuleOption
MakeDataModuleOption(CountryCurrency baseCurrency,
                     epoch_script::strategy::DatePeriodConfig const &period,
                     epoch_script::strategy::DataOption const &config,
                     DataCategory primaryCategory,
                     std::vector<DataCategory> const &auxiliaryCategories) {
  const auto [dataloaderAssets, strategyAssets, continuationAssets] =
      MakeAssets(baseCurrency, config.assets,
                 config.futures_continuation.has_value());
  const auto today = epoch_frame::DateTime::now().date();

  // Build flat categories set: {primaryCategory} ∪ auxiliaryCategories
  std::set<DataCategory> categories;
  categories.insert(primaryCategory);
  categories.insert(auxiliaryCategories.begin(), auxiliaryCategories.end());

  DataModuleOption dataModuleConfig{
      .loader = {.startDate = period.from,
                 .endDate = period.to,
                 .categories = categories,
                 .dataloaderAssets = dataloaderAssets,
                 .strategyAssets = strategyAssets,
                 .continuationAssets = continuationAssets,
                 .sourcePath = config.source.empty()
                     ? std::make_optional<std::filesystem::path>(DEFAULT_DATABASE_PATH)
                     : std::make_optional<std::filesystem::path>(config.source),
                 .cacheDir = config.cache_dir.empty()
                     ? std::nullopt
                     : std::make_optional<std::filesystem::path>(config.cache_dir)},
      .futureContinuation =
          MakeContinuations(continuationAssets, config.futures_continuation),
      .barResampleTimeFrames = {},
      .transformsConfigList = {},
      .liveUpdates = today >= period.from && today <= period.to};

  return dataModuleConfig;
}

DataModuleOption MakeDataModuleOptionFromStrategy(
    CountryCurrency baseCurrency,
    epoch_script::strategy::DatePeriodConfig const &period,
    epoch_script::strategy::StrategyConfig const &strategyConfig
) {
    // Auto-detect primary category by checking if strategy needs intraday data
    auto primaryCategory = strategy::IsIntradayCampaign(strategyConfig)
                         ? DataCategory::MinuteBars
                         : DataCategory::DailyBars;

    // Create base DataModuleOption
    auto dataModuleOption = MakeDataModuleOption(baseCurrency, period,
                                                 strategyConfig.data, primaryCategory);

    // Extract and process transforms from trade_signal if present
    if (strategyConfig.trade_signal.source.has_value()) {
        const auto& source = strategyConfig.trade_signal.source.value();
        auto compilationResult = source.GetCompilationResult();

        // Convert to TransformConfigurationPtrList for ProcessConfigurations
        epoch_script::transform::TransformConfigurationPtrList configPtrs;

        // Determine base timeframe from primary category
        auto baseTimeframe = primaryCategory == DataCategory::MinuteBars
            ? TimeFrame(std::string(epoch_script::tf_str::k1Min))
            : TimeFrame(std::string(epoch_script::tf_str::k1D));

        for (const auto& node : compilationResult) {
            auto timeframe = node.timeframe.value_or(baseTimeframe);
            auto config = std::make_unique<transform::TransformConfiguration>(
                TransformDefinition(node, timeframe)
            );
            configPtrs.push_back(std::move(config));
        }

        // Process configurations (adds transforms + resampling + auxiliary categories)
        ProcessConfigurations(configPtrs, baseTimeframe, dataModuleOption);
    }

    return dataModuleOption;
}

} // namespace factory
} // namespace epoch_script::data
