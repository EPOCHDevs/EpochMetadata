#pragma once

#include "epoch_metadata/transforms/metadata.h"
#include "polygon_data_source.h"

namespace epoch_metadata::transform {

// Factory function to create metadata for all Polygon data source transforms
inline std::vector<epoch_metadata::transforms::TransformsMetaData>
MakePolygonDataSources() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // 1. Balance Sheet Data
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "balance_sheet",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Balance Sheet",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load balance sheet fundamental data. "
                  "Provides assets, liabilities, equity, and other balance "
                  "sheet metrics over time.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "accounts_payable",
                   "Accounts Payable", true},
                  {epoch_core::IODataType::Decimal, "accrued_liabilities",
                   "Accrued & Other Current Liabilities", true},
                  {epoch_core::IODataType::Decimal, "aoci",
                   "Accumulated Other Comprehensive Income", true},
                  {epoch_core::IODataType::Decimal, "cash",
                   "Cash and Equivalents", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "current_debt",
                   "Current Debt", true},
                  {epoch_core::IODataType::Decimal, "deferred_revenue",
                   "Deferred Revenue (Current)", true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal, "inventories", "Inventories",
                   true},
                  {epoch_core::IODataType::Decimal, "long_term_debt",
                   "Long Term Debt & Capital Lease", true},
                  {epoch_core::IODataType::Decimal, "other_current_assets",
                   "Other Current Assets", true},
                  {epoch_core::IODataType::Decimal, "other_ltl",
                   "Other Non-Current Liabilities", true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal, "ppe_net",
                   "Property Plant Equipment Net", true},
                  {epoch_core::IODataType::Decimal, "receivables", "Receivables",
                   true},
                  {epoch_core::IODataType::Decimal, "retained_earnings",
                   "Retained Earnings (Deficit)", true},
                  {epoch_core::IODataType::String, "timeframe", "Timeframe",
                   true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"fundamentals", "balance-sheet", "financial-statements"},
          .requiresTimeFrame = false,
          .strategyTypes = {"fundamental-analysis", "value-investing"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access balance sheet data for fundamental analysis. Use to "
              "evaluate company financial health, leverage, liquidity. "
              "Combine with price data for value strategies. Data is "
              "quarterly/annual based on company filings.",
          .limitations =
              "Data availability depends on company filing schedules. "
              "Quarterly data has reporting lag. Only available for US "
              "equities with SEC filings. Requires external data loader.",
      });

  // 2. Income Statement Data
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "income_statement",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Income Statement",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load income statement fundamental data . "
                  "Provides revenue, expenses, earnings, and profitability "
                  "metrics over time.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "basic_eps",
                   "Basic EPS", true},
                  {epoch_core::IODataType::Decimal, "basic_shares",
                   "Basic Shares Outstanding", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "net_income",
                   "Consolidated Net Income", true},
                  {epoch_core::IODataType::Decimal, "cogs",
                   "Cost of Revenue", true},
                  {epoch_core::IODataType::Decimal, "diluted_eps",
                   "Diluted EPS", true},
                  {epoch_core::IODataType::Decimal, "diluted_shares",
                   "Diluted Shares Outstanding", true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal, "gross_profit",
                   "Gross Profit", true},
                  {epoch_core::IODataType::Decimal, "ebt",
                   "Income Before Taxes", true},
                  {epoch_core::IODataType::Decimal, "income_tax",
                   "Income Taxes", true},
                  {epoch_core::IODataType::Decimal, "ni_common",
                   "Net Income (Common Shareholders)", true},
                  {epoch_core::IODataType::Decimal, "operating_income",
                   "Operating Income", true},
                  {epoch_core::IODataType::Decimal, "other_income",
                   "Other Income/Expenses", true},
                  {epoch_core::IODataType::Decimal, "other_opex",
                   "Other Operating Expenses", true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal, "rnd",
                   "R&D Expenses", true},
                  {epoch_core::IODataType::Decimal, "revenue", "Revenue", true},
                  {epoch_core::IODataType::Decimal, "sga",
                   "SG&A Expenses", true},
                  {epoch_core::IODataType::String, "timeframe", "Timeframe",
                   true},
              },
          .atLeastOneInputRequired = false,
          .tags = { "fundamentals", "income-statement", "earnings",
                   "financial-statements"},
          .requiresTimeFrame = false,
          .strategyTypes = {"fundamental-analysis", "growth-investing",
                            "earnings-momentum"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access income statement data for profitability analysis. "
              "Track revenue growth, margin expansion, earnings quality. "
              "Essential for growth and earnings-based strategies. Compare "
              "quarter-over-quarter and year-over-year trends.",
          .limitations =
              "Data availability depends on company filing schedules. "
              "Quarterly data has reporting lag (typically 45+ days after "
              "quarter end). Only available for US equities with SEC filings. "
              "Requires external data loader.",
      });

  // 3. Cash Flow Statement Data
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "cash_flow",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Cash Flow",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load cash flow statement fundamental data . "
                  "Provides operating, investing, and financing cash flows to "
                  "analyze liquidity and capital allocation.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "cfo_cont",
                   "Operating Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "change_in_cash",
                   "Change in Cash & Equivalents", true},
                  {epoch_core::IODataType::Decimal, "change_in_wc",
                   "Change in Other Operating Assets/Liabilities", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "dna",
                   "D&A", true},
                  {epoch_core::IODataType::Decimal, "dividends", "Dividends",
                   true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal, "lt_debt_net",
                   "Long Term Debt Issuances/Repayments", true},
                  {epoch_core::IODataType::Decimal, "cff",
                   "Net Cash from Financing", true},
                  {epoch_core::IODataType::Decimal, "cff_cont",
                   "Net Cash from Financing (Continuing)", true},
                  {epoch_core::IODataType::Decimal, "cfi",
                   "Net Cash from Investing", true},
                  {epoch_core::IODataType::Decimal, "cfi_cont",
                   "Net Cash from Investing (Continuing)", true},
                  {epoch_core::IODataType::Decimal, "cfo",
                   "Net Cash from Operating", true},
                  {epoch_core::IODataType::Decimal, "net_income",
                   "Net Income", true},
                  {epoch_core::IODataType::Decimal, "other_cff",
                   "Other Financing Activities", true},
                  {epoch_core::IODataType::Decimal, "other_cfi",
                   "Other Investing Activities", true},
                  {epoch_core::IODataType::Decimal, "other_cfo",
                   "Other Operating Activities", true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal, "capex", "CapEx", true},
                  {epoch_core::IODataType::Decimal, "st_debt_net",
                   "Short Term Debt Issuances/Repayments", true},
                  {epoch_core::IODataType::String, "timeframe", "Timeframe",
                   true},
              },
          .atLeastOneInputRequired = false,
          .tags = { "fundamentals", "cash-flow", "financial-statements"},
          .requiresTimeFrame = false,
          .strategyTypes = {"fundamental-analysis", "cash-flow-analysis",
                            "quality-investing"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access cash flow data to assess company liquidity, capital "
              "allocation efficiency, and financial flexibility. Free cash "
              "flow (Operating CF - CapEx) is key metric. Essential for "
              "quality-focused fundamental strategies.",
          .limitations =
              "Data availability depends on company filing schedules. "
              "Quarterly data has reporting lag. Only available for US "
              "equities with SEC filings. Requires external data loader.",
      });

  // 4. Financial Ratios Data
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "financial_ratios",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Financial Ratios",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load financial ratios and valuation metrics . "
                  "Provides P/E, P/B, P/S, EV/EBITDA, and other key ratios for "
                  "fundamental screening and valuation analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "avg_volume",
                   "Average Volume", true},
                  {epoch_core::IODataType::Decimal, "cash", "Cash", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "current_ratio",
                   "Current Ratio", true},
                  {epoch_core::IODataType::String, "date", "Date", true},
                  {epoch_core::IODataType::Decimal, "debt_equity",
                   "Debt to Equity", true},
                  {epoch_core::IODataType::Decimal, "div_yield",
                   "Dividend Yield", true},
                  {epoch_core::IODataType::Decimal, "eps", "EPS", true},
                  {epoch_core::IODataType::Decimal, "ev",
                   "Enterprise Value", true},
                  {epoch_core::IODataType::Decimal, "ev_ebitda",
                   "EV/EBITDA", true},
                  {epoch_core::IODataType::Decimal, "ev_sales", "EV/Sales",
                   true},
                  {epoch_core::IODataType::Decimal, "fcf",
                   "Free Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "market_cap", "Market Cap",
                   true},
                  {epoch_core::IODataType::Decimal, "price", "Price", true},
                  {epoch_core::IODataType::Decimal, "pb",
                   "Price to Book", true},
                  {epoch_core::IODataType::Decimal, "pcf",
                   "Price to Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "pe",
                   "Price to Earnings", true},
                  {epoch_core::IODataType::Decimal, "pfcf",
                   "Price to Free Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "ps",
                   "Price to Sales", true},
                  {epoch_core::IODataType::Decimal, "quick_ratio",
                   "Quick Ratio", true},
                  {epoch_core::IODataType::Decimal, "roa", "ROA", true},
                  {epoch_core::IODataType::Decimal, "roe", "ROE", true},
                  {epoch_core::IODataType::String, "ticker", "Ticker", true},
              },
          .atLeastOneInputRequired = false,
          .tags = { "fundamentals", "ratios", "valuation", "screening"},
          .requiresTimeFrame = false,
          .strategyTypes = {"fundamental-analysis", "value-investing",
                            "screening", "factor-investing"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access pre-calculated financial ratios for valuation analysis. "
              "Use for fundamental screening (low P/E, high ROE), factor "
              "strategies, and relative value comparisons. Combine with price "
              "momentum for quality-value hybrids.",
          .limitations =
              "Ratios are calculated by Polygon based on most recent filings. "
              "Update frequency matches filing schedule (quarterly/annual). "
              "Only available for US equities. Cross-sectional comparisons "
              "require multiple node instances. Requires external data loader.",
      });

  // 5. Quote Data (NBBO)
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "quotes",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Quotes",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load quote (NBBO) tick data . Provides "
                  "best bid/ask prices and sizes with microsecond timestamps "
                  "for market microstructure analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "ticker", "Ticker", true},
                  {epoch_core::IODataType::Decimal, "ask", "Ask Price", true},
                  {epoch_core::IODataType::Decimal, "bid", "Bid Price", true},
                  {epoch_core::IODataType::Integer, "ask_size", "Ask Size", true},
                  {epoch_core::IODataType::Integer, "bid_size", "Bid Size", true},
                  {epoch_core::IODataType::Integer, "timestamp",
                   "Timestamp (ns)", true},
              },
          .atLeastOneInputRequired = false,
          .tags = { "market-data", "quotes", "nbbo", "bid-ask",
                   "microstructure"},
          .requiresTimeFrame = false,
          .strategyTypes = {"market-microstructure", "execution-analysis",
                            "liquidity-analysis"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access NBBO quote data for microstructure analysis, spread "
              "analysis, and execution quality measurement. Use to analyze "
              "bid-ask spread patterns, liquidity dynamics, and market depth. "
              "High-frequency data suitable for intraday analysis.",
          .limitations =
              "Very high data volume - use date ranges carefully. Nanosecond "
              "timestamps require careful handling. Historical data access "
              "limits based on subscription tier. Requires external "
              "data loader. Not suitable for daily/weekly strategies.",
      });

  // 6. Trade Tick Data
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "trades",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Trades",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load trade tick data . Provides individual "
                  "trade executions with price, size, and exchange information "
                  "for market microstructure and volume analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "ticker", "Ticker", true},
                  {epoch_core::IODataType::Decimal, "price", "Trade Price",
                   true},
                  {epoch_core::IODataType::Integer, "size", "Trade Size", true},
                  {epoch_core::IODataType::Integer, "exchange_id",
                   "Exchange ID", true},
                  {epoch_core::IODataType::Integer, "timestamp",
                   "Timestamp (ns)", true},

              },
          .atLeastOneInputRequired = false,
          .tags = { "market-data", "trades", "tick-data",
                   "microstructure"},
          .requiresTimeFrame = false,
          .strategyTypes = {"market-microstructure", "volume-analysis",
                            "vwap-execution"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Access individual trade executions for volume profile "
              "analysis, VWAP calculations, and trade flow studies. Use to "
              "analyze buying/selling pressure, large block trades, and "
              "intraday volume patterns. Aggregate to custom bars.",
          .limitations =
              "Extremely high data volume - use narrow date ranges. Nanosecond "
              "timestamps. Historical data access limits based on subscription. "
              "Requires external data loader. Not suitable for daily/weekly "
              "strategies - use Aggregates instead.",
      });

  // 7. OHLCV Aggregate Bars
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "aggregates",
          .category = epoch_core::TransformCategory::DataSource,
          .renderKind = epoch_core::TransformNodeRenderKind::Input,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Aggregates",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load OHLCV aggregate bars . Provides "
                  "open/high/low/close prices and volume at various timeframes "
                  "(1min to daily) for standard price-based technical analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "ticker", "Ticker", true},
                  {epoch_core::IODataType::Decimal, "o", "Open", true},
                  {epoch_core::IODataType::Decimal, "h", "High", true},
                  {epoch_core::IODataType::Decimal, "l", "Low", true},
                  {epoch_core::IODataType::Decimal, "c", "Close", true},
                  {epoch_core::IODataType::Decimal, "v", "Volume", true},
                  {epoch_core::IODataType::Decimal, "vw", "VWAP", true},
                  {epoch_core::IODataType::Integer, "n", "Trade Count", true},
                  {epoch_core::IODataType::Integer, "t", "Timestamp (ms)",
                   true},
              },
          .atLeastOneInputRequired = false,
          .tags = { "market-data", "ohlcv", "bars", "aggregates",
                   "price"},
          .requiresTimeFrame = true,
          .strategyTypes = {"technical-analysis", "trend-following",
                            "mean-reversion", "momentum", "any"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Primary data source for technical analysis strategies. Use "
              "for any OHLCV-based indicators and patterns. Supports multiple "
              "timeframes (1min, 5min, 1hour, 1day). Choose 'adjusted=true' "
              "for accurate historical comparisons. This is the most commonly "
              "used Polygon endpoint.",
          .limitations =
              "Daily bars only show regular trading hours. Intraday data "
              "availability depends on subscription tier. Adjust for splits/"
              "dividends to avoid false signals. Requires external data "
              "loader. Use market_data_source for basic strategies if Polygon "
              "not needed.",
      });

  return metadataList;
}

} // namespace epoch_metadata::transform
