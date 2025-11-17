#ifndef EPOCH_METADATA_POLYGON_INDICES_METADATA_H
#define EPOCH_METADATA_POLYGON_INDICES_METADATA_H

#include <epoch_script/transforms/core/metadata.h>
#include <epoch_data_sdk/dataloader/metadata_registry.hpp>
#include "data_category_mapper.h"
#include "metadata_helper.h"

namespace epoch_script::transform {

inline std::vector<epoch_script::transforms::TransformsMetaData> MakePolygonIndicesDataSources() {
  using namespace epoch_script::data_sources;
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // Get metadata from MetadataRegistry for market bars (indices use same schema as stocks)
  // Note: Indices can be either DailyBars or MinuteBars depending on timeframe
  auto dailyBarsMeta = data_sdk::dataloader::MetadataRegistry::GetMetadataForCategory(DataCategory::DailyBars);
  auto minuteBarsMeta = data_sdk::dataloader::MetadataRegistry::GetMetadataForCategory(DataCategory::MinuteBars);

  // Build outputs from SDK metadata
  auto outputs = BuildOutputsFromSDKMetadata(dailyBarsMeta);

  // Common Indices with SelectOption dropdown
  // Note: Uses DailyBars metadata by default, but actual category is determined at runtime based on timeframe
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
          },
      .desc = dailyBarsMeta.description,
      .inputs = {},
      .outputs = outputs,
      .requiresTimeFrame = true,
      .requiredDataSources = {"c"},  // Indices load data internally, "c" is just to get proper index
      .intradayOnly = false,
      .allowNullInputs = true,
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
  // Note: Uses DailyBars metadata by default, but actual category is determined at runtime based on timeframe
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
                  .desc = "Index ticker symbol (e.g., SPX, DJI, NDX, DAX, FTSE)"},
          },
      .desc = dailyBarsMeta.description,
      .inputs = {},
      .outputs = outputs,
      .requiresTimeFrame = true,
      .requiredDataSources = {"c"},  // Indices load data internally, "c" is just to get proper index
      .intradayOnly = false,
      .allowNullInputs = true,
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
