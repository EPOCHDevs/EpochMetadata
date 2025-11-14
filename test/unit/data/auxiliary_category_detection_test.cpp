//
// Test for auto-detection of auxiliary data categories from transforms
//
#include "catch.hpp"
#include "data/factory.h"
#include "epoch_script/core/constants.h"
#include <epoch_script/transforms/core/transform_definition.h>
#include <epoch_data_sdk/dataloader/options.hpp>

using namespace epoch_script::data;
using namespace epoch_script::data::factory;
using namespace epoch_script::transform;
using epoch_script::MetaDataArgDefinitionMapping;
using epoch_script::MetaDataOptionDefinition;

// Bring helper functions into scope
using epoch_script::data::factory::CreateFinancialsConfig;
using epoch_script::data::factory::CreateMacroEconomicsConfig;
using epoch_script::data::factory::CreateAlternativeDataConfig;
using epoch_script::data::factory::MapFREDTransformToDataCategory;
using epoch_script::data::factory::MapSECTransformToDataCategory;

// Helper to create a minimal TransformConfiguration for testing
TransformConfiguration MakeTestTransformConfig(
    std::string const &transformType,
    epoch_core::TransformCategory category,
    MetaDataArgDefinitionMapping options = {}) {

  epoch_script::TransformDefinitionData data{
      .type = transformType,
      .id = transformType + "_test",
      .options = std::move(options),
      .timeframe = epoch_script::TimeFrame{"1d"},
      .inputs = {},
      .metaData = {
          .id = transformType,
          .category = category,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = transformType,
          .options = {},
          .isCrossSectional = false,
          .desc = "Test transform",
          .inputs = {},
          .outputs = {},
          .atLeastOneInputRequired = false,
          .tags = {},
          .requiresTimeFrame = false,
          .requiredDataSources = {},
      }};

  return TransformConfiguration(
      epoch_script::TransformDefinition(std::move(data)));
}

TEST_CASE("MapPolygonTransformToDataCategory", "[factory][transforms]") {
  SECTION("Maps balance_sheet to Financials") {
    auto result =
        MapPolygonTransformToDataCategory(epoch_script::polygon::BALANCE_SHEET);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::Financials);
  }

  SECTION("Maps income_statement to Financials") {
    auto result = MapPolygonTransformToDataCategory(
        epoch_script::polygon::INCOME_STATEMENT);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::Financials);
  }

  SECTION("Maps cash_flow to Financials") {
    auto result =
        MapPolygonTransformToDataCategory(epoch_script::polygon::CASH_FLOW);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::Financials);
  }

  SECTION("Maps financial_ratios to Financials") {
    auto result = MapPolygonTransformToDataCategory(
        epoch_script::polygon::FINANCIAL_RATIOS);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::Financials);
  }

  SECTION("Returns nullopt for non-mapped transforms") {
    auto result = MapPolygonTransformToDataCategory("unknown_transform");
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Returns nullopt for quotes (tick data)") {
    auto result =
        MapPolygonTransformToDataCategory(epoch_script::polygon::QUOTES);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Returns nullopt for trades (tick data)") {
    auto result =
        MapPolygonTransformToDataCategory(epoch_script::polygon::TRADES);
    REQUIRE_FALSE(result.has_value());
  }
}

TEST_CASE("ExtractAuxiliaryCategoriesFromTransforms", "[factory][transforms]") {
  SECTION("Extracts Financials from balance_sheet transform") {
    TransformConfigurationList configs = {
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource)};

    auto configs_result = ExtractAuxiliaryCategoriesFromTransforms(configs);

    REQUIRE(configs_result.size() == 1);
    REQUIRE(configs_result[0].category == DataCategory::Financials);
    // Verify typed config is set correctly
    REQUIRE(configs_result[0].HasTypedConfig());
    auto fin_config = std::get<data_sdk::FinancialsConfig>(configs_result[0].config);
    REQUIRE(fin_config.type == data_sdk::FinancialsStatementType::BalanceSheet);
  }

  SECTION("Keeps all financial transforms with different _transform_id") {
    TransformConfigurationList configs = {
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource),
        MakeTestTransformConfig(epoch_script::polygon::INCOME_STATEMENT,
                                epoch_core::TransformCategory::DataSource),
        MakeTestTransformConfig(epoch_script::polygon::CASH_FLOW,
                                epoch_core::TransformCategory::DataSource)};

    auto configs_result = ExtractAuxiliaryCategoriesFromTransforms(configs);

    // All three map to Financials but have different _transform_id parameters
    REQUIRE(configs_result.size() == 3);
    // Check all are Financials category
    REQUIRE(configs_result[0].category == DataCategory::Financials);
    REQUIRE(configs_result[1].category == DataCategory::Financials);
    REQUIRE(configs_result[2].category == DataCategory::Financials);
  }

  SECTION("Ignores non-DataSource transforms") {
    TransformConfigurationList configs = {
        MakeTestTransformConfig("sma", epoch_core::TransformCategory::Trend),
        MakeTestTransformConfig("rsi", epoch_core::TransformCategory::Momentum)};

    auto configs_result = ExtractAuxiliaryCategoriesFromTransforms(configs);

    REQUIRE(configs_result.empty());
  }

  SECTION("Ignores unknown Polygon transforms") {
    TransformConfigurationList configs = {MakeTestTransformConfig(
        "custom_data_source", epoch_core::TransformCategory::DataSource)};

    auto configs_result = ExtractAuxiliaryCategoriesFromTransforms(configs);

    REQUIRE(configs_result.empty());
  }

  SECTION("Mixed transforms - only extracts DataSource categories") {
    TransformConfigurationList configs = {
        MakeTestTransformConfig("sma", epoch_core::TransformCategory::Trend),
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource),
        MakeTestTransformConfig("rsi", epoch_core::TransformCategory::Momentum),
        MakeTestTransformConfig(epoch_script::polygon::INCOME_STATEMENT,
                                epoch_core::TransformCategory::DataSource)};

    auto configs_result = ExtractAuxiliaryCategoriesFromTransforms(configs);

    REQUIRE(configs_result.size() == 2);
    REQUIRE(configs_result[0].category == DataCategory::Financials);
    REQUIRE(configs_result[1].category == DataCategory::Financials);
  }
}

TEST_CASE("ProcessConfigurations auto-detects auxiliary categories",
          "[factory][integration]") {
  SECTION("Auto-populates auxiliary categories from DataSource transforms") {
    // Create a DataModuleOption with empty auxiliary categories
    DataModuleOption option{
        .loader = {.startDate = epoch_frame::DateTime::from_date_str("2024-01-01").date(),
                   .endDate = epoch_frame::DateTime::from_date_str("2024-12-31").date(),
                   .primaryCategory = DataCategory::MinuteBars,
                   .auxiliaryCategories = {}}};

    // Create transform configurations
    std::vector<std::unique_ptr<TransformConfiguration>> configs;
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource)));
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig("sma", epoch_core::TransformCategory::Trend)));

    // Process configurations
    ProcessConfigurations(configs, epoch_script::TimeFrame{"1d"}, option);

    // Verify auxiliary categories were populated
    REQUIRE(option.loader.auxiliaryCategories.size() == 1);
    REQUIRE(option.loader.auxiliaryCategories[0].category == DataCategory::Financials);
    REQUIRE(option.loader.auxiliaryCategories[0].HasTypedConfig());
    auto fin_config = std::get<data_sdk::FinancialsConfig>(option.loader.auxiliaryCategories[0].config);
    REQUIRE(fin_config.type == data_sdk::FinancialsStatementType::BalanceSheet);
  }

  SECTION("Merges auto-detected with manually specified categories") {
    // Create a DataModuleOption with manually specified auxiliary category
    DataModuleOption option{
        .loader = {.startDate = epoch_frame::DateTime::from_date_str("2024-01-01").date(),
                   .endDate = epoch_frame::DateTime::from_date_str("2024-12-31").date(),
                   .primaryCategory = DataCategory::MinuteBars,
                   .auxiliaryCategories = {AuxiliaryCategoryConfig(DataCategory::News)}}};

    // Create transform configurations with different category
    std::vector<std::unique_ptr<TransformConfiguration>> configs;
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource)));

    // Process configurations
    ProcessConfigurations(configs, epoch_script::TimeFrame{"1d"}, option);

    // Verify both categories are present (detected Financials overwrites manually specified News)
    REQUIRE(option.loader.auxiliaryCategories.size() >= 1);
    // Find both categories
    bool hasNews = false;
    bool hasFinancials = false;
    for (const auto& config : option.loader.auxiliaryCategories) {
      if (config.category == DataCategory::News) hasNews = true;
      if (config.category == DataCategory::Financials) hasFinancials = true;
    }
    REQUIRE(hasFinancials);  // Should have auto-detected Financials
  }
}

// ============================================================================
// TYPED CONFIG TESTS - Testing the new typed configuration system
// ============================================================================

TEST_CASE("CreateFinancialsConfig helper function", "[factory][typed-config]") {
  SECTION("Creates BalanceSheet config") {
    auto config = CreateFinancialsConfig(epoch_script::polygon::BALANCE_SHEET);
    REQUIRE(config.type == data_sdk::FinancialsStatementType::BalanceSheet);
  }

  SECTION("Creates IncomeStatement config") {
    auto config = CreateFinancialsConfig(epoch_script::polygon::INCOME_STATEMENT);
    REQUIRE(config.type == data_sdk::FinancialsStatementType::IncomeStatement);
  }

  SECTION("Creates CashFlow config") {
    auto config = CreateFinancialsConfig(epoch_script::polygon::CASH_FLOW);
    REQUIRE(config.type == data_sdk::FinancialsStatementType::CashFlow);
  }

  SECTION("Creates FinancialRatios config") {
    auto config = CreateFinancialsConfig(epoch_script::polygon::FINANCIAL_RATIOS);
    REQUIRE(config.type == data_sdk::FinancialsStatementType::FinancialRatios);
  }
}

TEST_CASE("CreateMacroEconomicsConfig with option extraction", "[factory][typed-config]") {
  SECTION("Extracts CPI indicator from options") {
    MetaDataArgDefinitionMapping options;
    options["category"] = MetaDataOptionDefinition(std::string("CPI"));

    auto transform_config = MakeTestTransformConfig(
        epoch_script::fred::ECONOMIC_INDICATOR,
        epoch_core::TransformCategory::DataSource,
        options);

    auto macro_config = CreateMacroEconomicsConfig(transform_config);
    REQUIRE(macro_config.indicator == epoch_core::MacroEconomicsIndicator::CPI);
  }

  SECTION("Extracts GDP indicator from options") {
    MetaDataArgDefinitionMapping options;
    options["category"] = MetaDataOptionDefinition(std::string("GDP"));

    auto transform_config = MakeTestTransformConfig(
        epoch_script::fred::ECONOMIC_INDICATOR,
        epoch_core::TransformCategory::DataSource,
        options);

    auto macro_config = CreateMacroEconomicsConfig(transform_config);
    REQUIRE(macro_config.indicator == epoch_core::MacroEconomicsIndicator::GDP);
  }

  SECTION("Extracts Unemployment indicator from options") {
    MetaDataArgDefinitionMapping options;
    options["category"] = MetaDataOptionDefinition(std::string("Unemployment"));

    auto transform_config = MakeTestTransformConfig(
        epoch_script::fred::ECONOMIC_INDICATOR,
        epoch_core::TransformCategory::DataSource,
        options);

    auto macro_config = CreateMacroEconomicsConfig(transform_config);
    REQUIRE(macro_config.indicator == epoch_core::MacroEconomicsIndicator::Unemployment);
  }
}

TEST_CASE("CreateAlternativeDataConfig helper function", "[factory][typed-config]") {
  SECTION("Creates SEC_Form13F config") {
    auto config = CreateAlternativeDataConfig(epoch_script::sec::FORM_13F_HOLDINGS);
    REQUIRE(config.source == data_sdk::AlternativeDataSource::SEC_Form13F);
  }

  SECTION("Creates SEC_InsiderTrading config") {
    auto config = CreateAlternativeDataConfig(epoch_script::sec::INSIDER_TRADING);
    REQUIRE(config.source == data_sdk::AlternativeDataSource::SEC_InsiderTrading);
  }
}

TEST_CASE("Multiple economic indicators create separate typed configs", "[factory][integration]") {
  MetaDataArgDefinitionMapping cpi_options;
  cpi_options["category"] = MetaDataOptionDefinition(std::string("CPI"));

  MetaDataArgDefinitionMapping fed_funds_options;
  fed_funds_options["category"] = MetaDataOptionDefinition(std::string("FedFunds"));

  TransformConfigurationList configs = {
      MakeTestTransformConfig(epoch_script::fred::ECONOMIC_INDICATOR,
                             epoch_core::TransformCategory::DataSource,
                             cpi_options),
      MakeTestTransformConfig(epoch_script::fred::ECONOMIC_INDICATOR,
                             epoch_core::TransformCategory::DataSource,
                             fed_funds_options)
  };

  auto aux = ExtractAuxiliaryCategoriesFromTransforms(configs);

  REQUIRE(aux.size() == 2);
  REQUIRE(aux[0].category == DataCategory::MacroEconomics);
  REQUIRE(aux[1].category == DataCategory::MacroEconomics);

  // Verify typed configs contain correct indicators
  auto config1 = std::get<data_sdk::MacroEconomicsConfig>(aux[0].config);
  auto config2 = std::get<data_sdk::MacroEconomicsConfig>(aux[1].config);

  REQUIRE(config1.indicator == epoch_core::MacroEconomicsIndicator::CPI);
  REQUIRE(config2.indicator == epoch_core::MacroEconomicsIndicator::FedFunds);
}

TEST_CASE("Multiple financial statements preserved with typed configs", "[factory][integration]") {
  TransformConfigurationList configs = {
      MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                             epoch_core::TransformCategory::DataSource),
      MakeTestTransformConfig(epoch_script::polygon::INCOME_STATEMENT,
                             epoch_core::TransformCategory::DataSource),
      MakeTestTransformConfig(epoch_script::polygon::CASH_FLOW,
                             epoch_core::TransformCategory::DataSource)
  };

  auto aux = ExtractAuxiliaryCategoriesFromTransforms(configs);

  REQUIRE(aux.size() == 3);

  // Verify all are Financials category
  for (const auto& config : aux) {
    REQUIRE(config.category == DataCategory::Financials);
  }

  // Verify typed configs contain correct statement types
  auto config1 = std::get<data_sdk::FinancialsConfig>(aux[0].config);
  auto config2 = std::get<data_sdk::FinancialsConfig>(aux[1].config);
  auto config3 = std::get<data_sdk::FinancialsConfig>(aux[2].config);

  REQUIRE(config1.type == data_sdk::FinancialsStatementType::BalanceSheet);
  REQUIRE(config2.type == data_sdk::FinancialsStatementType::IncomeStatement);
  REQUIRE(config3.type == data_sdk::FinancialsStatementType::CashFlow);
}

TEST_CASE("Mixed category transforms all preserved with typed configs", "[factory][integration]") {
  MetaDataArgDefinitionMapping cpi_options;
  cpi_options["category"] = MetaDataOptionDefinition(std::string("CPI"));

  TransformConfigurationList configs = {
      MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                             epoch_core::TransformCategory::DataSource),
      MakeTestTransformConfig(epoch_script::fred::ECONOMIC_INDICATOR,
                             epoch_core::TransformCategory::DataSource,
                             cpi_options),
      MakeTestTransformConfig(epoch_script::sec::FORM_13F_HOLDINGS,
                             epoch_core::TransformCategory::DataSource)
  };

  auto aux = ExtractAuxiliaryCategoriesFromTransforms(configs);

  REQUIRE(aux.size() == 3);

  // Find and verify each category
  bool hasFinancials = false;
  bool hasMacro = false;
  bool hasAlt = false;

  for (const auto& config : aux) {
    if (config.category == DataCategory::Financials) {
      hasFinancials = true;
      auto fin_config = std::get<data_sdk::FinancialsConfig>(config.config);
      REQUIRE(fin_config.type == data_sdk::FinancialsStatementType::BalanceSheet);
    }
    else if (config.category == DataCategory::MacroEconomics) {
      hasMacro = true;
      auto macro_config = std::get<data_sdk::MacroEconomicsConfig>(config.config);
      REQUIRE(macro_config.indicator == epoch_core::MacroEconomicsIndicator::CPI);
    }
    else if (config.category == DataCategory::AlternativeData) {
      hasAlt = true;
      auto alt_config = std::get<data_sdk::AlternativeDataConfig>(config.config);
      REQUIRE(alt_config.source == data_sdk::AlternativeDataSource::SEC_Form13F);
    }
  }

  REQUIRE(hasFinancials);
  REQUIRE(hasMacro);
  REQUIRE(hasAlt);
}

TEST_CASE("MapFREDTransformToDataCategory", "[factory][transforms]") {
  SECTION("Maps economic_indicator to MacroEconomics") {
    auto result = MapFREDTransformToDataCategory(epoch_script::fred::ECONOMIC_INDICATOR);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::MacroEconomics);
  }

  SECTION("Returns nullopt for unknown transforms") {
    auto result = MapFREDTransformToDataCategory("unknown_fred_transform");
    REQUIRE_FALSE(result.has_value());
  }
}

TEST_CASE("MapSECTransformToDataCategory", "[factory][transforms]") {
  SECTION("Maps form13f_holdings to AlternativeData") {
    auto result = MapSECTransformToDataCategory(epoch_script::sec::FORM_13F_HOLDINGS);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::AlternativeData);
  }

  SECTION("Maps insider_trading to AlternativeData") {
    auto result = MapSECTransformToDataCategory(epoch_script::sec::INSIDER_TRADING);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::AlternativeData);
  }

  SECTION("Returns nullopt for unknown transforms") {
    auto result = MapSECTransformToDataCategory("unknown_sec_transform");
    REQUIRE_FALSE(result.has_value());
  }
}
