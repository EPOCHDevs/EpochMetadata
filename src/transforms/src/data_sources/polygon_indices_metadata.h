#ifndef EPOCH_METADATA_POLYGON_INDICES_METADATA_H
#define EPOCH_METADATA_POLYGON_INDICES_METADATA_H

#include "epoch_metadata/transforms/metadata.h"

namespace epoch_metadata::transform {

inline std::vector<epoch_metadata::transforms::TransformsMetaData> MakePolygonIndicesDataSources() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // Common Indices with SelectOption dropdown
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
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
                          {"SPX", "S&P 500"},
                          {"DJI", "Dow Jones Industrial Average"},
                          {"NDX", "NASDAQ 100"},
                          {"RUT", "Russell 2000"},
                          {"VIX", "CBOE Volatility Index"},
                          {"NYA", "NYSE Composite"},
                          {"XAU", "Philadelphia Gold and Silver Index"},
                          {"RUI", "Russell 1000"},
                          {"RUA", "Russell 3000"},
                          {"FTSE", "FTSE 100"},
                      },
                  .desc = "Select the market index"},
          },
      .desc =
          "Load aggregate bars (OHLC) for popular market indices from Polygon.io. "
          "External loader extracts date range from input DataFrame and fetches data via "
          "/v2/aggs/ticker/{ticker}/range endpoint.",
      .inputs = {},
      .outputs =
          {
              {epoch_core::IODataType::Decimal, "open", "Open", true},
              {epoch_core::IODataType::Decimal, "high", "High", true},
              {epoch_core::IODataType::Decimal, "low", "Low", true},
              {epoch_core::IODataType::Decimal, "close", "Close", true},
              {epoch_core::IODataType::Decimal, "volume", "Volume", false},
              {epoch_core::IODataType::Decimal, "vw", "Volume Weighted Average Price", false},
              {epoch_core::IODataType::Integer, "n", "Number of Transactions", false},
              {epoch_core::IODataType::Integer, "timestamp", "Timestamp", true},
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
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
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
          },
      .desc =
          "Load aggregate bars (OHLC) for any market index from Polygon.io using a dynamic ticker symbol. "
          "External loader extracts date range from input DataFrame and fetches data via "
          "/v2/aggs/ticker/{ticker}/range endpoint.",
      .inputs = {},
      .outputs =
          {
              {epoch_core::IODataType::Decimal, "open", "Open", true},
              {epoch_core::IODataType::Decimal, "high", "High", true},
              {epoch_core::IODataType::Decimal, "low", "Low", true},
              {epoch_core::IODataType::Decimal, "close", "Close", true},
              {epoch_core::IODataType::Decimal, "volume", "Volume", false},
              {epoch_core::IODataType::Decimal, "vw", "Volume Weighted Average Price", false},
              {epoch_core::IODataType::Integer, "n", "Number of Transactions", false},
              {epoch_core::IODataType::Integer, "timestamp", "Timestamp", true},
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

}  // namespace epoch_metadata::transform

#endif  // EPOCH_METADATA_POLYGON_INDICES_METADATA_H
