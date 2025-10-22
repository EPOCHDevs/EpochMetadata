#pragma once

#include "epoch_metadata/transforms/metadata.h"
#include "../data_source.h"

namespace epoch_metadata::transform {

// Factory function to create metadata for FRED economic data source
inline std::vector<epoch_metadata::transforms::TransformsMetaData>
MakeFREDDataSource() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // Single FRED transform with category SelectOption
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "economic_indicator",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Economic Indicator",
          .options =
              {
                  MetaDataOption{
                      .id = "category",
                      .name = "Economic Indicator",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue =
                          MetaDataOptionDefinition(std::string("CPI")),
                      .selectOption =
                          {
                              // Inflation Indicators
                              {"CPI", "Consumer Price Index (CPI-U)"},
                              {"CoreCPI", "Core CPI (ex Food & Energy)"},
                              {"PCE",
                               "Personal Consumption Expenditures Price Index"},
                              {"CorePCE", "Core PCE (Fed's Preferred Measure)"},

                              // Interest Rates & Monetary Policy
                              {"FedFunds", "Federal Funds Effective Rate"},
                              {"Treasury3M", "3-Month Treasury Bill Rate"},
                              {"Treasury2Y", "2-Year Treasury Rate"},
                              {"Treasury5Y", "5-Year Treasury Rate"},
                              {"Treasury10Y", "10-Year Treasury Rate"},
                              {"Treasury30Y", "30-Year Treasury Rate"},

                              // Employment & Labor Market
                              {"Unemployment", "Unemployment Rate"},
                              {"NonfarmPayrolls", "Nonfarm Payrolls"},
                              {"InitialClaims", "Initial Jobless Claims (Weekly)"},

                              // Economic Growth & Production
                              {"GDP", "Real Gross Domestic Product"},
                              {"IndustrialProduction",
                               "Industrial Production Index"},
                              {"RetailSales", "Retail Sales"},
                              {"HousingStarts", "Housing Starts"},

                              // Market Sentiment & Money Supply
                              {"ConsumerSentiment",
                               "Consumer Sentiment (University of Michigan)"},
                              {"M2", "M2 Money Supply"},
                              {"SP500", "S&P 500 Index"},
                              {"VIX", "VIX Volatility Index (CBOE)"},
                          },
                      .desc = "Select the economic indicator series to load"},
                  MetaDataOption{
                      .id = "from_date",
                      .name = "From Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc =
                          "Start date for observations (YYYY-MM-DD format)"},
                  MetaDataOption{
                      .id = "to_date",
                      .name = "To Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date for observations (YYYY-MM-DD format)"},
                  MetaDataOption{
                      .id = "published_from",
                      .name = "Published From (ALFRED)",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc =
                          "Optional: Point-in-time filter for backtesting. "
                          "Get data as it existed on this date (YYYY-MM-DD). "
                          "Use ALFRED realtime period to avoid look-ahead bias "
                          "from data revisions."},
              },
          .isCrossSectional = false,
          .desc =
              "Load Federal Reserve Economic Data (FRED) for macro analysis. "
              "Provides economic indicators like inflation, interest rates, "
              "GDP, employment data, and market indices. Non-asset-specific - "
              "applies globally to strategy.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "date", "Observation Date",
                   true},
                  {epoch_core::IODataType::Decimal, "value", "Indicator Value",
                   true},
                  {epoch_core::IODataType::String, "published_at",
                   "Published At (ALFRED)", true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"fred", "macro", "economic-indicators", "inflation",
                   "interest-rates", "gdp", "employment"},
          .requiresTimeFrame = false,
          .strategyTypes = {"macro-analysis", "regime-detection",
                            "economic-calendar", "risk-on-risk-off"},
          .assetRequirements = {},
          .usageContext =
              "Access Federal Reserve economic data for macro-driven "
              "strategies. Use for economic cycle identification (expansion/"
              "recession), monetary policy regime detection (tightening/"
              "easing), and risk-on/risk-off switching. Combine inflation + "
              "rates for monetary policy stance, unemployment + GDP for cycle "
              "phase, VIX for market stress. Non-ticker-based - same data "
              "applies to all assets in strategy.",
          .limitations =
              "Data publication frequency varies by indicator: daily (rates/"
              "VIX), weekly (claims), monthly (CPI/employment/retail), "
              "quarterly (GDP). Significant lag between period end and "
              "publication (weeks to months). FRED data is US-centric. "
              "CRITICAL: Use 'published_from' (ALFRED) for realistic "
              "backtesting to get data as it existed historically - avoids "
              "look-ahead bias from revisions. Requires external FRED data "
              "loader.",
      });

  return metadataList;
}

} // namespace epoch_metadata::transform
