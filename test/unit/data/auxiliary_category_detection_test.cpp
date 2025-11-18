//
// Test for auto-detection of auxiliary data categories from transforms
//
#include "catch.hpp"
#include <epoch_script/data/factory.h>
#include "epoch_script/core/constants.h"
#include <epoch_script/transforms/core/transform_definition.h>
#include <epoch_data_sdk/dataloader/options.hpp>
#include "transforms/components/data_sources/data_category_mapper.h"

using namespace epoch_script::data;
using namespace epoch_script::data::factory;
using namespace epoch_script::transform;
using epoch_script::MetaDataArgDefinitionMapping;
using epoch_script::MetaDataOptionDefinition;

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

TEST_CASE("GetDataCategoryForTransform", "[factory][transforms]") {
  SECTION("Maps balance_sheet to BalanceSheets") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform(
        epoch_script::polygon::BALANCE_SHEET);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::BalanceSheets);
  }

  SECTION("Maps income_statement to IncomeStatements") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform(
        epoch_script::polygon::INCOME_STATEMENT);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::IncomeStatements);
  }

  SECTION("Maps cash_flow to CashFlowStatements") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform(
        epoch_script::polygon::CASH_FLOW);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::CashFlowStatements);
  }

  SECTION("Returns nullopt for non-mapped transforms") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform("unknown_transform");
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Maps news transforms") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform(
        epoch_script::polygon::NEWS);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::News);
  }

  SECTION("Maps dividends transforms") {
    auto result = epoch_script::data_sources::GetDataCategoryForTransform(
        epoch_script::polygon::DIVIDENDS);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == DataCategory::Dividends);
  }
}

TEST_CASE("ExtractAuxiliaryCategoriesFromTransforms", "[factory][transforms]") {
  SECTION("Extracts BalanceSheets from balance_sheet transform") {
    std::string code = "balance_sheet_data = balance_sheet(timeframe=\"1D\")";
    auto source = epoch_script::strategy::PythonSource(code, true);
    auto manager = epoch_script::runtime::CreateTransformManager(source);

    auto categories = ExtractAuxiliaryCategoriesFromTransforms(*manager->GetTransforms());

    REQUIRE(categories.size() == 1);
    REQUIRE(categories[0] == DataCategory::BalanceSheets);
  }

  SECTION("Extracts different financial categories") {
    std::string code = R"(
balance_sheet_data = balance_sheet(timeframe="1D")
income_stmt_data = income_statement(timeframe="1D")
cash_flow_data = cash_flow(timeframe="1D")
)";
    auto source = epoch_script::strategy::PythonSource(code, true);
    auto manager = epoch_script::runtime::CreateTransformManager(source);

    auto categories = ExtractAuxiliaryCategoriesFromTransforms(*manager->GetTransforms());

    REQUIRE(categories.size() == 3);
    std::set<DataCategory> categorySet(categories.begin(), categories.end());
    REQUIRE(categorySet.count(DataCategory::BalanceSheets) == 1);
    REQUIRE(categorySet.count(DataCategory::IncomeStatements) == 1);
    REQUIRE(categorySet.count(DataCategory::CashFlowStatements) == 1);
  }

  SECTION("Ignores non-DataSource transforms") {
    std::string code = R"(
prices = market_data_source(timeframe="1D")
sma_val = sma(prices.close, 20, timeframe="1D")
rsi_val = rsi(prices.close, 14, timeframe="1D")
)";
    auto source = epoch_script::strategy::PythonSource(code, true);
    auto manager = epoch_script::runtime::CreateTransformManager(source);

    auto categories = ExtractAuxiliaryCategoriesFromTransforms(*manager->GetTransforms());

    REQUIRE(categories.empty());
  }

  SECTION("Mixed transforms - only extracts DataSource categories") {
    std::string code = R"(
prices = market_data_source(timeframe="1D")
sma_val = sma(prices.close, 20, timeframe="1D")
balance_sheet_data = balance_sheet(timeframe="1D")
rsi_val = rsi(prices.close, 14, timeframe="1D")
income_stmt_data = income_statement(timeframe="1D")
)";
    auto source = epoch_script::strategy::PythonSource(code, true);
    auto manager = epoch_script::runtime::CreateTransformManager(source);

    auto categories = ExtractAuxiliaryCategoriesFromTransforms(*manager->GetTransforms());

    REQUIRE(categories.size() == 2);
    std::set<DataCategory> categorySet(categories.begin(), categories.end());
    REQUIRE(categorySet.count(DataCategory::BalanceSheets) == 1);
    REQUIRE(categorySet.count(DataCategory::IncomeStatements) == 1);
  }
}

TEST_CASE("ProcessConfigurations auto-detects auxiliary categories",
          "[factory][integration]") {
  SECTION("Auto-populates auxiliary categories from DataSource transforms") {
    // Create a DataModuleOption with only MinuteBars category
    DataModuleOption option{
        .loader = {.startDate = epoch_frame::DateTime::from_date_str("2024-01-01").date(),
                   .endDate = epoch_frame::DateTime::from_date_str("2024-12-31").date(),
                   .categories = {DataCategory::MinuteBars}}};

    // Create transform configurations
    std::vector<std::unique_ptr<TransformConfiguration>> configs;
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource)));
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig("sma", epoch_core::TransformCategory::Trend)));

    // Process configurations
    ProcessConfigurations(configs, epoch_script::TimeFrame{"1d"}, option);

    // Verify auxiliary categories were added to categories set
    REQUIRE(option.loader.categories.count(DataCategory::BalanceSheets) == 1);
    REQUIRE(option.loader.categories.count(DataCategory::MinuteBars) == 1);
    REQUIRE(option.loader.categories.size() == 2);
  }

  SECTION("Merges auto-detected with existing categories") {
    // Create a DataModuleOption with manually specified categories
    DataModuleOption option{
        .loader = {.startDate = epoch_frame::DateTime::from_date_str("2024-01-01").date(),
                   .endDate = epoch_frame::DateTime::from_date_str("2024-12-31").date(),
                   .categories = {DataCategory::MinuteBars, DataCategory::News}}};

    // Create transform configurations with different category
    std::vector<std::unique_ptr<TransformConfiguration>> configs;
    configs.push_back(std::make_unique<TransformConfiguration>(
        MakeTestTransformConfig(epoch_script::polygon::BALANCE_SHEET,
                                epoch_core::TransformCategory::DataSource)));

    // Process configurations
    ProcessConfigurations(configs, epoch_script::TimeFrame{"1d"}, option);

    // Verify all categories are present
    REQUIRE(option.loader.categories.count(DataCategory::MinuteBars) == 1);
    REQUIRE(option.loader.categories.count(DataCategory::News) == 1);
    REQUIRE(option.loader.categories.count(DataCategory::BalanceSheets) == 1);
    REQUIRE(option.loader.categories.size() == 3);
  }
}

TEST_CASE("Mixed data source categories", "[factory][integration]") {
  SECTION("Multiple different data source categories are detected") {
    std::string code = R"(
balance_sheet_data = balance_sheet(timeframe="1D")
news_data = news(timeframe="1D")
divs = dividends(timeframe="1D")
)";
    auto source = epoch_script::strategy::PythonSource(code, true);
    auto manager = epoch_script::runtime::CreateTransformManager(source);

    auto categories = ExtractAuxiliaryCategoriesFromTransforms(*manager->GetTransforms());

    REQUIRE(categories.size() == 3);

    std::set<DataCategory> categorySet(categories.begin(), categories.end());
    REQUIRE(categorySet.count(DataCategory::BalanceSheets) == 1);
    REQUIRE(categorySet.count(DataCategory::News) == 1);
    REQUIRE(categorySet.count(DataCategory::Dividends) == 1);
  }
}
