//
// Created by dewe on 1/29/23.
//
#include "factory.h"
#include "common/env_loader.h"
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

IDataLoaderPtr DataModuleFactory::CreateDataloader() {
  // Use epoch_data_sdk factory to create API+cache dataloader
  SPDLOG_INFO("Creating API+cache dataloader from epoch_data_sdk");
  return data_sdk::dataloader::CreateApiCacheDataLoader(
      m_option.loader, EPOCH_DB_S3);
}

IFuturesContinuationConstructor::Ptr
DataModuleFactory::CreateFutureContinuations() {
  auto futuresContinuation = m_option.futureContinuation;
  if (!futuresContinuation) {
    return {};
  }

  return std::make_unique<FuturesContinuationConstructor>(
      std::make_unique<FuturesContinuation>(
          MakeRolloverMethod(futuresContinuation->rollover,
                             futuresContinuation->arg),
          MakeAdjustmentMethod(futuresContinuation->type)));
}

epoch_script::runtime::IDataFlowOrchestrator::Ptr DataModuleFactory::CreateTransforms() {
  // Extract asset IDs from strategy assets
  std::set<std::string> assetIds;
  for (const auto& asset : m_option.loader.strategyAssets) {
    assetIds.insert(asset.GetID());
  }

  return epoch_script::runtime::CreateDataFlowRuntimeOrchestrator(
      assetIds,
      m_option.transformsConfigList);
}

IResamplerPtr DataModuleFactory::CreateResampler() {
    if (m_option.barResampleTimeFrames.empty()) {
        return {};
    }

    return std::make_unique<Resampler>(m_option.barResampleTimeFrames,
                                       m_option.loader.GetDataCategory() ==
                                           DataCategory::MinuteBars);
}

asset::AssetClassMap<IWebSocketManagerPtr>
DataModuleFactory::CreateWebSocketManager() {
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

std::unique_ptr<Database> DataModuleFactory::CreateDatabase() {
  return std::make_unique<Database>(
      std::make_unique<DatabaseImpl>(DatabaseImplOptions{
          .dataloader = CreateDataloader(),
          .dataTransform = CreateTransforms(),
          .futuresContinuationConstructor = CreateFutureContinuations(),
          .resampler = CreateResampler(),
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

std::optional<FuturesContinuation::Input> MakeContinuations(
    asset::AssetHashSet const &assets,
    std::optional<epoch_script::strategy::TemplatedGenericFunction<
        epoch_core::RolloverType>> const &config) {
    if (assets.empty() || !config ||
        config->type == epoch_core::RolloverType::Null) {
        return std::nullopt;
        }

    FuturesContinuation::Input continuationOption{
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

// Helper functions to create typed configs for auxiliary categories
data_sdk::FinancialsConfig CreateFinancialsConfig(std::string const& transformType) {
  using namespace epoch_script::polygon;
  data_sdk::FinancialsConfig config;

  if (transformType == BALANCE_SHEET)
    config.type = data_sdk::FinancialsStatementType::BalanceSheet;
  else if (transformType == INCOME_STATEMENT)
    config.type = data_sdk::FinancialsStatementType::IncomeStatement;
  else if (transformType == CASH_FLOW)
    config.type = data_sdk::FinancialsStatementType::CashFlow;
  else if (transformType == FINANCIAL_RATIOS)
    config.type = data_sdk::FinancialsStatementType::FinancialRatios;

  return config;
}

data_sdk::MacroEconomicsConfig CreateMacroEconomicsConfig(
    epoch_script::transform::TransformConfiguration const& config) {
  data_sdk::MacroEconomicsConfig macro_config;

  // Extract the indicator enum from the transform's "category" option
  macro_config.indicator = config.GetOptionValue("category")
      .GetSelectOption<epoch_core::MacroEconomicsIndicator>();

  return macro_config;
}

data_sdk::AlternativeDataConfig CreateAlternativeDataConfig(std::string const& transformType) {
  using namespace epoch_script::sec;
  data_sdk::AlternativeDataConfig config;

  if (transformType == FORM_13F_HOLDINGS)
    config.source = data_sdk::AlternativeDataSource::SEC_Form13F;
  else if (transformType == INSIDER_TRADING)
    config.source = data_sdk::AlternativeDataSource::SEC_InsiderTrading;

  return config;
}

std::optional<DataCategory>
MapPolygonTransformToDataCategory(std::string const &transformType) {
  using namespace epoch_script::polygon;

  // Map Polygon transform IDs to DataCategory
  // Financial statements all map to Financials category
  if (transformType == BALANCE_SHEET || transformType == INCOME_STATEMENT ||
      transformType == CASH_FLOW || transformType == FINANCIAL_RATIOS) {
    return DataCategory::Financials;
  }

  // Tick-level market microstructure data
  // if (transformType == QUOTES || transformType == TRADES) {
  //   return DataCategory::TickData;
  // }

  // Indices (common_indices, indices) are NOT mapped here
  // They are loaded internally and are not asset-specific

  // Other potential mappings can be added here as needed
  // e.g., news → DataCategory::News (when available)

  return std::nullopt;
}

std::optional<DataCategory>
MapFREDTransformToDataCategory(std::string const &transformType) {
  using namespace epoch_script::fred;

  // Map FRED transform IDs to DataCategory
  // All economic indicators map to MacroEconomics category
  if (transformType == ECONOMIC_INDICATOR) {
    return DataCategory::MacroEconomics;
  }

  return std::nullopt;
}

std::optional<DataCategory>
MapSECTransformToDataCategory(std::string const &transformType) {
  using namespace epoch_script::sec;

  // Map SEC transform IDs to DataCategory
  // Institutional holdings and insider trading are alternative data
  if (transformType == FORM_13F_HOLDINGS || transformType == INSIDER_TRADING) {
    return DataCategory::AlternativeData;
  }

  return std::nullopt;
}

std::vector<AuxiliaryCategoryConfig>
ExtractAuxiliaryCategoriesFromTransforms(
    epoch_script::transform::TransformConfigurationList const &configs) {

  // Map from category to configs (to handle multiple transforms per category)
  std::unordered_map<DataCategory, std::vector<AuxiliaryCategoryConfig>> categoryMap;

  for (auto const &config : configs) {
    auto const &metadata = config.GetTransformDefinition().GetMetadata();

    // Only process DataSource transforms
    if (metadata.category != epoch_core::TransformCategory::DataSource) {
      continue;
    }

    // Get the transform type/id
    std::string transformType = config.GetTransformName();

    std::optional<DataCategory> category;

    // Check which data provider this transform belongs to
    if (epoch_script::polygon::ALL_POLYGON_TRANSFORMS.contains(transformType)) {
      category = MapPolygonTransformToDataCategory(transformType);
    } else if (epoch_script::fred::ALL_FRED_TRANSFORMS.contains(transformType)) {
      category = MapFREDTransformToDataCategory(transformType);
    } else if (epoch_script::sec::ALL_SEC_TRANSFORMS.contains(transformType)) {
      category = MapSECTransformToDataCategory(transformType);
    }

    if (category.has_value()) {
      // Skip if it's a time-series category (those are primary, not auxiliary)
      if (!IsTimeSeriesCategory(*category)) {
        // Create config with typed configuration based on category
        AuxiliaryCategoryConfig auxConfig(*category);

        if (*category == DataCategory::Financials) {
          auxConfig.config = CreateFinancialsConfig(transformType);
        } else if (*category == DataCategory::MacroEconomics) {
          auxConfig.config = CreateMacroEconomicsConfig(config);
        } else if (*category == DataCategory::AlternativeData) {
          auxConfig.config = CreateAlternativeDataConfig(transformType);
        }
        // Other categories (News, Dividends, Splits, etc.) use std::monostate (no specific config)

        categoryMap[*category].push_back(auxConfig);
      }
    }
  }

  // Flatten to vector - preserve all configs, including multiple per category
  std::vector<AuxiliaryCategoryConfig> result;
  for (auto &[category, configs] : categoryMap) {
    result.insert(result.end(), configs.begin(), configs.end());
  }

  return result;
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

  // Auto-detect and populate auxiliary categories from DataSource transforms
  auto detectedConfigs = ExtractAuxiliaryCategoriesFromTransforms(
      dataModuleOption.transformsConfigList);

#ifndef NDEBUG
  if (!detectedConfigs.empty()) {
    SPDLOG_DEBUG("Auto-detected {} auxiliary data category configs from transforms:",
                 detectedConfigs.size());
    for ([[maybe_unused]] const auto& config : detectedConfigs) {
      SPDLOG_DEBUG("  - {} (transform: {})",
                   DataCategoryWrapper::ToString(config.category),
                   config.parameters.count("_transform_id") ? config.parameters.at("_transform_id") : "unknown");
    }
  }
#endif

  // Merge with existing auxiliary category configs
  // Keep all configs, including multiple instances of the same category
  std::vector<AuxiliaryCategoryConfig> allConfigs;

  // Add existing configs
  for (const auto& config : dataModuleOption.loader.auxiliaryCategories) {
    allConfigs.push_back(config);
  }

  // Add detected configs (preserve all instances)
  for (const auto& config : detectedConfigs) {
    allConfigs.push_back(config);
  }

  // Update the dataModuleOption with all configs
  dataModuleOption.loader.auxiliaryCategories = std::move(allConfigs);

  // DEPRECATED: Old code reference (removed)
  // OLD: dataModuleOption.loader.auxiliaryCategories = categorySet;
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

  // Convert auxiliary categories to AuxiliaryCategoryConfig
  std::vector<AuxiliaryCategoryConfig> auxConfigs;
  auxConfigs.reserve(auxiliaryCategories.size());
  for (const auto& cat : auxiliaryCategories) {
    auxConfigs.emplace_back(cat);
  }

  DataModuleOption dataModuleConfig{
      .loader = {.startDate = period.from,
                 .endDate = period.to,
                 .primaryCategory = primaryCategory,
                 .auxiliaryCategories = auxConfigs,
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
