#ifndef EPOCH_METADATA_REFERENCE_STOCKS_METADATA_H
#define EPOCH_METADATA_REFERENCE_STOCKS_METADATA_H

#include <epoch_script/transforms/core/metadata.h>

namespace epoch_script::transform {

inline std::vector<epoch_script::transforms::TransformsMetaData> MakeReferenceStocksDataSources() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
      .id = "us_reference_stocks",
      .category = epoch_core::TransformCategory::DataSource,
      .plotKind = epoch_core::TransformPlotKind::Null,
      .name = "US Reference Stocks",
      .options =
          {
              MetaDataOption{
                  .id = "ticker",
                  .name = "Reference Ticker",
                  .type = epoch_core::MetaDataOptionType::String,
                  .defaultValue = MetaDataOptionDefinition(std::string("SPY")),
                  .desc = "Reference stock ticker symbol (e.g., SPY, QQQ, DIA, IWM)"},
          },
      .isCrossSectional = false,
      .desc =
          "Load OHLCV pricing data for US reference stocks (ETFs and equities) to use as "
          "benchmarks or comparison assets against the main strategy asset. "
          "Provides open, high, low, close, volume, volume-weighted price, and trade count. "
          "Commonly used for pairs trading, relative strength, or beta-hedging strategies.",
      .inputs = {},
      .outputs =
          {
              {epoch_core::IODataType::Decimal, "o", "Open", true},
              {epoch_core::IODataType::Decimal, "h", "High", true},
              {epoch_core::IODataType::Decimal, "l", "Low", true},
              {epoch_core::IODataType::Decimal, "c", "Close", true},
              {epoch_core::IODataType::Decimal, "v", "Volume", true},
              {epoch_core::IODataType::Decimal, "vw", "Volume Weighted Price", true},
              {epoch_core::IODataType::Integer, "n", "Trade Count", true},
          },
      .atLeastOneInputRequired = false,
      .tags = {"reference", "comparison", "benchmark", "data", "source", "etf"},
      .requiresTimeFrame = true,
      .requiredDataSources = {"o", "h", "l", "c", "v", "vw", "n"},
      .strategyTypes = {"pairs-trading", "relative-strength", "beta-hedging", "correlation"},
      .assetRequirements = {"multi-asset"},
      .usageContext =
          "Use this node to load reference stock data for comparison against your main asset. "
          "Date range automatically aligns with the strategy's main market_data_source. "
          "Common use cases: comparing stock performance to SPY, pairs trading, calculating beta, "
          "or building market-neutral strategies. The is_eod parameter is automatically determined "
          "from the timeframe (intraday vs daily/higher).",
      .limitations =
          "Data availability depends on Polygon.io subscription level. "
          "Date range is determined by the main market_data_source node in the strategy. "
          "External loader must handle API authentication, rate limiting, and date alignment.",
  });

  return metadataList;
}

}  // namespace epoch_script::transform

#endif  // EPOCH_METADATA_REFERENCE_STOCKS_METADATA_H
