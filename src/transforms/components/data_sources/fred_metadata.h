#pragma once

#include <epochflow/transforms/core/metadata.h>
#include "../data_source.h"

namespace epochflow::transform {

// Factory function to create metadata for FRED economic data source
inline std::vector<epochflow::transforms::TransformsMetaData>
MakeFREDDataSource() {
  std::vector<epochflow::transforms::TransformsMetaData> metadataList;

  // Single FRED transform with category SelectOption
  metadataList.emplace_back(
      epochflow::transforms::TransformsMetaData{
          .id = "economic_indicator",
          .category = epoch_core::TransformCategory::DataSource,
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
                              {"Consumer Price Index (CPI-U)", "CPI"},
                              {"Core CPI (ex Food & Energy)", "CoreCPI"},
                              {"Personal Consumption Expenditures Price Index",
                               "PCE"},
                              {"Core PCE (Fed's Preferred Measure)", "CorePCE"},

                              // Interest Rates & Monetary Policy
                              {"Federal Funds Effective Rate", "FedFunds"},
                              {"3-Month Treasury Bill Rate", "Treasury3M"},
                              {"2-Year Treasury Rate", "Treasury2Y"},
                              {"5-Year Treasury Rate", "Treasury5Y"},
                              {"10-Year Treasury Rate", "Treasury10Y"},
                              {"30-Year Treasury Rate", "Treasury30Y"},

                              // Employment & Labor Market
                              {"Unemployment Rate", "Unemployment"},
                              {"Nonfarm Payrolls", "NonfarmPayrolls"},
                              {"Initial Jobless Claims (Weekly)", "InitialClaims"},

                              // Economic Growth & Production
                              {"Real Gross Domestic Product", "GDP"},
                              {"Industrial Production Index",
                               "IndustrialProduction"},
                              {"Retail Sales", "RetailSales"},
                              {"Housing Starts", "HousingStarts"},

                              // Market Sentiment & Money Supply
                              {"Consumer Sentiment (University of Michigan)",
                               "ConsumerSentiment"},
                              {"M2 Money Supply", "M2"},
                              {"S&P 500 Index", "SP500"},
                              {"VIX Volatility Index (CBOE)", "VIX"},
                          },
                      .desc = "Select the economic indicator series to load"},
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
                  {epoch_core::IODataType::String, "observation_date",
                   "Economic Period", true},
                  {epoch_core::IODataType::Decimal, "value", "Indicator Value",
                   true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"fred", "macro", "economic-indicators", "inflation",
                   "interest-rates", "gdp", "employment"},
          .requiresTimeFrame = true,
          .requiredDataSources = {"c"},
          .strategyTypes = {"macro-analysis", "regime-detection",
                            "economic-calendar", "risk-on-risk-off"},
          .assetRequirements = {},
          .usageContext =
              "Access Federal Reserve economic data for macro-driven "
              "strategies. Date range auto-derived from connected market data. "
              "Returns publication events (non-null only on release dates) - "
              "strategy decides how to use (compare, trigger, lag, etc.). "
              "Use for economic cycle identification, monetary policy regime "
              "detection, and risk-on/risk-off switching. Combine inflation + "
              "rates for policy stance, unemployment + GDP for cycle phase. "
              "Requires connection to market data source.",
          .limitations =
              "Publication frequency varies: daily (rates/VIX), weekly (claims), "
              "monthly (CPI/employment), quarterly (GDP). Significant lag between "
              "period end and publication (weeks to months). Values appear ONLY "
              "on publication dates (not forward-filled). FRED data is US-centric. "
              "External loader must implement ALFRED point-in-time filtering to "
              "avoid look-ahead bias from data revisions. Requires external FRED "
              "data loader with API key.",
      });

  return metadataList;
}

} // namespace epochflow::transform
