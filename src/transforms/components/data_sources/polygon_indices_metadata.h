#ifndef EPOCH_METADATA_POLYGON_INDICES_METADATA_H
#define EPOCH_METADATA_POLYGON_INDICES_METADATA_H

#include <epoch_script/transforms/core/metadata.h>

namespace epoch_script::transform {

inline std::vector<epoch_script::transforms::TransformsMetaData> MakePolygonIndicesDataSources() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // Common Indices with SelectOption dropdown
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
      .id = "common_indices",
      .category = epoch_core::TransformCategory::DataSource,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Common Indices",
      .options =
          {
              MetaDataOption{
                  .id = "index",
                  .name = "Index",
                  .type = epoch_core::MetaDataOptionType::Select,
                  .defaultValue = MetaDataOptionDefinition(std::string("SPX")),
                  .selectOption =
                      {
                          {"S&P 500", "SPX"},
                          {"Dow Jones Industrial Average", "DJI"},
                          {"NASDAQ 100", "NDX"},
                          {"Russell 2000", "RUT"},
                          {"CBOE Volatility Index", "VIX"},
                          {"NYSE Composite", "NYA"},
                          {"Philadelphia Gold and Silver Index", "XAU"},
                          {"Russell 1000", "RUI"},
                          {"Russell 3000", "RUA"},
                          {"FTSE 100", "FTSE"},
                      },
                  .desc = "Select the market index"},
              MetaDataOption{
                  .id = "data_type",
                  .name = "Data Type",
                  .type = epoch_core::MetaDataOptionType::Select,
                  .defaultValue = MetaDataOptionDefinition(std::string("eod")),
                  .selectOption =
                      {
                          {"End of Day (Daily)", "eod"},
                          {"Intraday (1-minute bars)", "intraday"},
                      },
                  .desc = "Select whether to fetch end-of-day (daily) or intraday (1-minute) data"},
          },
      .desc =
          "Historical price data for major market indices including S&P 500, Dow Jones, NASDAQ, Russell indices, and VIX. "
          "Provides open, high, low, and close prices for the selected index.",
      .inputs = {},
      .outputs =
          {
              {epoch_core::IODataType::Decimal, "o", "Open", true},
              {epoch_core::IODataType::Decimal, "h", "High", true},
              {epoch_core::IODataType::Decimal, "l", "Low", true},
              {epoch_core::IODataType::Decimal, "c", "Close", true},
          },
      .requiresTimeFrame = false,
      .requiredDataSources = {"c"},
      .strategyTypes = {"market-regime", "index-analysis", "correlation", "hedge"},
      .assetRequirements = {"single-asset", "multi-asset"},
      .usageContext =
          "Use this node to access historical index data for market analysis, correlation studies, "
          "or hedging strategies. Select from popular indices like S&P 500, NASDAQ 100, or VIX.",
      .limitations =
          "Data availability and update frequency depend on Polygon.io subscription level. "
          "External loader must handle API authentication and rate limiting.",
  });

  // Dynamic Indices with ticker parameter
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
      .id = "indices",
      .category = epoch_core::TransformCategory::DataSource,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "Indices",
      .options =
          {
              MetaDataOption{
                  .id = "ticker",
                  .name = "Index Ticker",
                  .type = epoch_core::MetaDataOptionType::String,
                  .defaultValue = MetaDataOptionDefinition(std::string("SPX")),
                  .desc = "Index ticker symbol (e.g., SPX, DJI, NDX)"},
              MetaDataOption{
                  .id = "data_type",
                  .name = "Data Type",
                  .type = epoch_core::MetaDataOptionType::Select,
                  .defaultValue = MetaDataOptionDefinition(std::string("eod")),
                  .selectOption =
                      {
                          {"End of Day (Daily)", "eod"},
                          {"Intraday (1-minute bars)", "intraday"},
                      },
                  .desc = "Select whether to fetch end-of-day (daily) or intraday (1-minute) data"},
          },
      .desc =
          "Historical price data for any market index by ticker symbol. "
          "Provides open, high, low, and close prices for the specified index ticker.",
      .inputs = {},
      .outputs =
          {
              {epoch_core::IODataType::Decimal, "o", "Open", true},
              {epoch_core::IODataType::Decimal, "h", "High", true},
              {epoch_core::IODataType::Decimal, "l", "Low", true},
              {epoch_core::IODataType::Decimal, "c", "Close", true},
          },
      .requiresTimeFrame = false,
      .requiredDataSources = {"c"},
      .strategyTypes = {"market-regime", "index-analysis", "correlation", "hedge"},
      .assetRequirements = {"single-asset", "multi-asset"},
      .usageContext =
          "Use this node to access historical data for any market index by specifying its ticker symbol. "
          "Useful for custom indices or international indices not in the common list.",
      .limitations =
          "Data availability and update frequency depend on Polygon.io subscription level. "
          "External loader must handle API authentication and rate limiting.",
  });

  return metadataList;
}

}  // namespace epoch_script::transform

#endif  // EPOCH_METADATA_POLYGON_INDICES_METADATA_H
