#pragma once

#include "epoch_metadata/transforms/metadata.h"
#include "../data_source.h"

namespace epoch_metadata::transform {

// Factory function to create metadata for TradingEconomics data sources
inline std::vector<epoch_metadata::transforms::TransformsMetaData>
MakeTradingEconomicsDataSources() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // 1. TradingEconomics Historical Series
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "trading_economics_series",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Trading Economics Series",
          .options =
              {
                  MetaDataOption{
                      .id = "symbol",
                      .name = "Series Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Trading Economics series symbol (e.g., USURTOT "
                              "for US unemployment rate)"},
                  MetaDataOption{
                      .id = "category",
                      .name = "Economic Category",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue =
                          MetaDataOptionDefinition(std::string("GDP")),
                      .selectOption =
                          {{"GDP", "Gross Domestic Product"},
                           {"Inflation", "Inflation Rate"},
                           {"InterestRate", "Interest Rate"},
                           {"Unemployment", "Unemployment Rate"},
                           {"GovernmentBond", "Government Bond Yields"},
                           {"BalanceOfTrade", "Balance of Trade"},
                           {"CurrentAccount", "Current Account"},
                           {"GovernmentDebt", "Government Debt to GDP"},
                           {"BusinessConfidence", "Business Confidence Index"},
                           {"ConsumerConfidence", "Consumer Confidence Index"},
                           {"RetailSales", "Retail Sales"},
                           {"IndustrialProduction", "Industrial Production"},
                           {"Manufacturing", "Manufacturing PMI"},
                           {"Services", "Services PMI"},
                           {"HousingStarts", "Housing Starts"},
                           {"NewHomeSales", "New Home Sales"},
                           {"CPI", "Consumer Price Index"},
                           {"PPI", "Producer Price Index"},
                           {"EmploymentRate", "Employment Rate"},
                           {"WageGrowth", "Wage Growth"},
                           {"MoneySupply", "Money Supply"},
                           {"BankLending", "Bank Lending Rate"},
                           {"AssetPurchases", "Central Bank Asset Purchases"},
                           {"CurrencyReserves", "Foreign Currency Reserves"}},
                      .desc = "Economic indicator category for filtering and "
                              "organization"},
                  MetaDataOption{
                      .id = "start_date",
                      .name = "Start Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Start date for historical data (YYYY-MM-DD), "
                              "optional"},
                  MetaDataOption{
                      .id = "end_date",
                      .name = "End Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date for historical data (YYYY-MM-DD), "
                              "optional"},
                  MetaDataOption{
                      .id = "output_type",
                      .name = "Output Format",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue =
                          MetaDataOptionDefinition(std::string("json")),
                      .selectOption = {{"json", "JSON Format"},
                                       {"xml", "XML Format"},
                                       {"csv", "CSV Format"}},
                      .desc = "Data format from Trading Economics API (default "
                              "json)"},
              },
          .isCrossSectional = false,
          .desc = "Load economic indicator time series from Trading Economics. "
                  "Provides historical macroeconomic data (GDP, inflation, "
                  "unemployment, etc.) for fundamental macro analysis and "
                  "regime detection.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "date", "Date", true},
                  {epoch_core::IODataType::Decimal, "value",
                   "Indicator Value", true},
                  {epoch_core::IODataType::String, "symbol", "Symbol", true},
                  {epoch_core::IODataType::String, "category", "Category",
                   true},
                  {epoch_core::IODataType::String, "country", "Country", true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"trading-economics", "macro", "economic-indicators", "gdp",
                   "inflation", "fundamentals"},
          .requiresTimeFrame = false,
          .strategyTypes = {"macro-analysis", "regime-detection",
                            "economic-calendar", "fundamental-macro"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access macroeconomic time series for regime-based strategies. "
              "Use GDP growth, inflation rates, unemployment for economic "
              "cycle detection. Combine with interest rates for monetary "
              "policy regime identification. Essential for top-down macro "
              "strategies and risk-on/risk-off switching.",
          .limitations =
              "Update frequency varies by indicator (monthly/quarterly/"
              "annual). Significant publication lag (weeks to months). "
              "Requires Trading Economics API subscription. Data revisions "
              "common. Requires external data loader. Symbol lookup needed "
              "for specific indicators.",
      });

  // 2. TradingEconomics Economic Calendar
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "trading_economics_calendar",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Trading Economics Calendar",
          .options =
              {
                  MetaDataOption{
                      .id = "country",
                      .name = "Country",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue =
                          MetaDataOptionDefinition(std::string("United States")),
                      .selectOption =
                          {{"United States", "United States"},
                           {"China", "China"},
                           {"Euro Area", "Euro Area"},
                           {"Japan", "Japan"},
                           {"United Kingdom", "United Kingdom"},
                           {"Germany", "Germany"},
                           {"France", "France"},
                           {"Italy", "Italy"},
                           {"Spain", "Spain"},
                           {"Canada", "Canada"},
                           {"Australia", "Australia"},
                           {"South Korea", "South Korea"},
                           {"India", "India"},
                           {"Brazil", "Brazil"},
                           {"Russia", "Russia"},
                           {"Mexico", "Mexico"},
                           {"Switzerland", "Switzerland"},
                           {"Sweden", "Sweden"},
                           {"Norway", "Norway"},
                           {"Denmark", "Denmark"},
                           {"All", "All Countries"}},
                      .desc = "Country for economic calendar events"},
                  MetaDataOption{
                      .id = "category",
                      .name = "Event Category",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue =
                          MetaDataOptionDefinition(std::string("All")),
                      .selectOption =
                          {{"All", "All Categories"},
                           {"GDP", "GDP Releases"},
                           {"Inflation", "Inflation Reports"},
                           {"Employment", "Employment Data"},
                           {"InterestRate", "Interest Rate Decisions"},
                           {"CentralBankSpeech", "Central Bank Speeches"},
                           {"Manufacturing", "Manufacturing PMI"},
                           {"Services", "Services PMI"},
                           {"RetailSales", "Retail Sales"},
                           {"HousingData", "Housing Data"},
                           {"TradeBalance", "Trade Balance"},
                           {"ConsumerConfidence", "Consumer Confidence"},
                           {"BusinessSentiment", "Business Sentiment"},
                           {"Earnings", "Corporate Earnings"}},
                      .desc = "Filter calendar by event category"},
                  MetaDataOption{
                      .id = "start_date",
                      .name = "Start Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Start date for calendar events (YYYY-MM-DD), "
                              "optional"},
                  MetaDataOption{
                      .id = "end_date",
                      .name = "End Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date for calendar events (YYYY-MM-DD), "
                              "optional"},
              },
          .isCrossSectional = false,
          .desc = "Load economic calendar events from Trading Economics. "
                  "Provides scheduled release dates for economic indicators "
                  "with actual, forecast, and previous values for event-driven "
                  "analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "date", "Event Date", true},
                  {epoch_core::IODataType::String, "country", "Country", true},
                  {epoch_core::IODataType::String, "category", "Category",
                   true},
                  {epoch_core::IODataType::Decimal, "actual", "Actual Value",
                   true},
                  {epoch_core::IODataType::Decimal, "previous",
                   "Previous Value", true},
                  {epoch_core::IODataType::Decimal, "forecast",
                   "Forecast Value", true},
                  {epoch_core::IODataType::String, "unit", "Unit", true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"trading-economics", "economic-calendar", "events", "macro",
                   "releases", "surprise"},
          .requiresTimeFrame = false,
          .strategyTypes = {"event-driven", "news-trading", "surprise-analysis",
                            "macro-events"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access economic calendar for event-driven strategies. Compare "
              "actual vs forecast to detect surprises. Trade around high-"
              "impact releases (NFP, CPI, FOMC). Use for volatility "
              "prediction around scheduled events. Combine with price action "
              "for release-day patterns.",
          .limitations =
              "Calendar data typically available 1-2 weeks in advance. "
              "Actual values updated at release time (real-time subscription "
              "needed for live data). Forecast consensus may change. Requires "
              "Trading Economics API subscription. Historical surprise "
              "analysis requires careful timestamp handling. Requires external "
              "data loader.",
      });

  return metadataList;
}

} // namespace epoch_metadata::transform
