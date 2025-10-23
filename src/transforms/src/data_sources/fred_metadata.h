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

} // namespace epoch_metadata::transform
