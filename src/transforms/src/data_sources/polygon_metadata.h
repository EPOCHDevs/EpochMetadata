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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "cik",
                      .name = "CIK",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Central Index Key (optional filter)"},
                  MetaDataOption{
                      .id = "period_end",
                      .name = "Period End Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Filter by period end date (YYYY-MM-DD)"},
                  MetaDataOption{.id = "limit",
                                 .name = "Limit",
                                 .type = epoch_core::MetaDataOptionType::Integer,
                                 .defaultValue = MetaDataOptionDefinition(100.0),
                                 .min = 1,
                                 .max = 1000,
                                 .desc = "Maximum number of results to return"},
              },
          .isCrossSectional = false,
          .desc = "Load balance sheet fundamental data. "
                  "Provides assets, liabilities, equity, and other balance "
                  "sheet metrics over time.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "accounts_payable",
                   "Accounts Payable", true},
                  {epoch_core::IODataType::Decimal,
                   "accrued_and_other_current_liabilities",
                   "Accrued & Other Current Liabilities", true},
                  {epoch_core::IODataType::Decimal,
                   "accumulated_other_comprehensive_income",
                   "Accumulated Other Comprehensive Income", true},
                  {epoch_core::IODataType::Decimal, "cash_and_equivalents",
                   "Cash and Equivalents", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "debt_current",
                   "Current Debt", true},
                  {epoch_core::IODataType::Decimal, "deferred_revenue_current",
                   "Deferred Revenue (Current)", true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal, "inventories", "Inventories",
                   true},
                  {epoch_core::IODataType::Decimal,
                   "long_term_debt_and_capital_lease_obligations",
                   "Long Term Debt & Capital Lease", true},
                  {epoch_core::IODataType::Decimal, "other_current_assets",
                   "Other Current Assets", true},
                  {epoch_core::IODataType::Decimal,
                   "other_noncurrent_liabilities", "Other Non-Current Liabilities",
                   true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal,
                   "property_plant_equipment_net", "Property Plant Equipment Net",
                   true},
                  {epoch_core::IODataType::Decimal, "receivables", "Receivables",
                   true},
                  {epoch_core::IODataType::Decimal, "retained_earnings_deficit",
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "cik",
                      .name = "CIK",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Central Index Key (optional filter)"},
                  MetaDataOption{
                      .id = "period_end",
                      .name = "Period End Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Filter by period end date (YYYY-MM-DD)"},
                  MetaDataOption{.id = "limit",
                                 .name = "Limit",
                                 .type = epoch_core::MetaDataOptionType::Integer,
                                 .defaultValue = MetaDataOptionDefinition(100.0),
                                 .min = 1,
                                 .max = 1000,
                                 .desc = "Maximum number of results to return"},
              },
          .isCrossSectional = false,
          .desc = "Load income statement fundamental data . "
                  "Provides revenue, expenses, earnings, and profitability "
                  "metrics over time.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "basic_earnings_per_share",
                   "Basic EPS", true},
                  {epoch_core::IODataType::Decimal, "basic_shares_outstanding",
                   "Basic Shares Outstanding", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal,
                   "consolidated_net_income_loss", "Consolidated Net Income",
                   true},
                  {epoch_core::IODataType::Decimal, "cost_of_revenue",
                   "Cost of Revenue", true},
                  {epoch_core::IODataType::Decimal, "diluted_earnings_per_share",
                   "Diluted EPS", true},
                  {epoch_core::IODataType::Decimal, "diluted_shares_outstanding",
                   "Diluted Shares Outstanding", true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal, "gross_profit",
                   "Gross Profit", true},
                  {epoch_core::IODataType::Decimal, "income_before_income_taxes",
                   "Income Before Taxes", true},
                  {epoch_core::IODataType::Decimal, "income_taxes",
                   "Income Taxes", true},
                  {epoch_core::IODataType::Decimal,
                   "net_income_loss_attributable_to_common_shareholders",
                   "Net Income (Common Shareholders)", true},
                  {epoch_core::IODataType::Decimal, "operating_income",
                   "Operating Income", true},
                  {epoch_core::IODataType::Decimal, "other_income_expenses",
                   "Other Income/Expenses", true},
                  {epoch_core::IODataType::Decimal, "other_operating_expenses",
                   "Other Operating Expenses", true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal, "research_development",
                   "R&D Expenses", true},
                  {epoch_core::IODataType::Decimal, "revenue", "Revenue", true},
                  {epoch_core::IODataType::Decimal,
                   "selling_general_administrative", "SG&A Expenses", true},
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "cik",
                      .name = "CIK",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Central Index Key (optional filter)"},
                  MetaDataOption{
                      .id = "period_end",
                      .name = "Period End Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Filter by period end date (YYYY-MM-DD)"},
                  MetaDataOption{.id = "limit",
                                 .name = "Limit",
                                 .type = epoch_core::MetaDataOptionType::Integer,
                                 .defaultValue = MetaDataOptionDefinition(100.0),
                                 .min = 1,
                                 .max = 1000,
                                 .desc = "Maximum number of results to return"},
              },
          .isCrossSectional = false,
          .desc = "Load cash flow statement fundamental data . "
                  "Provides operating, investing, and financing cash flows to "
                  "analyze liquidity and capital allocation.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal,
                   "cash_from_operating_activities_continuing_operations",
                   "Operating Cash Flow", true},
                  {epoch_core::IODataType::Decimal,
                   "change_in_cash_and_equivalents",
                   "Change in Cash & Equivalents", true},
                  {epoch_core::IODataType::Decimal,
                   "change_in_other_operating_assets_and_liabilities_net",
                   "Change in Other Operating Assets/Liabilities", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal,
                   "depreciation_depletion_and_amortization", "D&A", true},
                  {epoch_core::IODataType::Decimal, "dividends", "Dividends",
                   true},
                  {epoch_core::IODataType::Integer, "fiscal_quarter",
                   "Fiscal Quarter", true},
                  {epoch_core::IODataType::Integer, "fiscal_year",
                   "Fiscal Year", true},
                  {epoch_core::IODataType::Decimal,
                   "long_term_debt_issuances_repayments",
                   "Long Term Debt Issuances/Repayments", true},
                  {epoch_core::IODataType::Decimal,
                   "net_cash_from_financing_activities",
                   "Net Cash from Financing", true},
                  {epoch_core::IODataType::Decimal,
                   "net_cash_from_financing_activities_continuing_operations",
                   "Net Cash from Financing (Continuing)", true},
                  {epoch_core::IODataType::Decimal,
                   "net_cash_from_investing_activities",
                   "Net Cash from Investing", true},
                  {epoch_core::IODataType::Decimal,
                   "net_cash_from_investing_activities_continuing_operations",
                   "Net Cash from Investing (Continuing)", true},
                  {epoch_core::IODataType::Decimal,
                   "net_cash_from_operating_activities",
                   "Net Cash from Operating", true},
                  {epoch_core::IODataType::Decimal, "net_income",
                   "Net Income", true},
                  {epoch_core::IODataType::Decimal, "other_financing_activities",
                   "Other Financing Activities", true},
                  {epoch_core::IODataType::Decimal, "other_investing_activities",
                   "Other Investing Activities", true},
                  {epoch_core::IODataType::Decimal, "other_operating_activities",
                   "Other Operating Activities", true},
                  {epoch_core::IODataType::String, "period_end", "Period End",
                   true},
                  {epoch_core::IODataType::Decimal,
                   "purchase_of_property_plant_and_equipment", "CapEx", true},
                  {epoch_core::IODataType::Decimal,
                   "short_term_debt_issuances_repayments",
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "cik",
                      .name = "CIK",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Central Index Key (optional filter)"},
                  MetaDataOption{.id = "limit",
                                 .name = "Limit",
                                 .type = epoch_core::MetaDataOptionType::Integer,
                                 .defaultValue = MetaDataOptionDefinition(100.0),
                                 .min = 1,
                                 .max = 1000,
                                 .desc = "Maximum number of results to return"},
              },
          .isCrossSectional = false,
          .desc = "Load financial ratios and valuation metrics . "
                  "Provides P/E, P/B, P/S, EV/EBITDA, and other key ratios for "
                  "fundamental screening and valuation analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "average_volume",
                   "Average Volume", true},
                  {epoch_core::IODataType::Decimal, "cash", "Cash", true},
                  {epoch_core::IODataType::String, "cik", "CIK", true},
                  {epoch_core::IODataType::Decimal, "current", "Current Ratio",
                   true},
                  {epoch_core::IODataType::String, "date", "Date", true},
                  {epoch_core::IODataType::Decimal, "debt_to_equity",
                   "Debt to Equity", true},
                  {epoch_core::IODataType::Decimal, "dividend_yield",
                   "Dividend Yield", true},
                  {epoch_core::IODataType::Decimal, "earnings_per_share", "EPS",
                   true},
                  {epoch_core::IODataType::Decimal, "enterprise_value",
                   "Enterprise Value", true},
                  {epoch_core::IODataType::Decimal, "ev_to_ebitda",
                   "EV/EBITDA", true},
                  {epoch_core::IODataType::Decimal, "ev_to_sales", "EV/Sales",
                   true},
                  {epoch_core::IODataType::Decimal, "free_cash_flow",
                   "Free Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "market_cap", "Market Cap",
                   true},
                  {epoch_core::IODataType::Decimal, "price", "Price", true},
                  {epoch_core::IODataType::Decimal, "price_to_book",
                   "Price to Book", true},
                  {epoch_core::IODataType::Decimal, "price_to_cash_flow",
                   "Price to Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "price_to_earnings",
                   "Price to Earnings", true},
                  {epoch_core::IODataType::Decimal, "price_to_free_cash_flow",
                   "Price to Free Cash Flow", true},
                  {epoch_core::IODataType::Decimal, "price_to_sales",
                   "Price to Sales", true},
                  {epoch_core::IODataType::Decimal, "quick", "Quick Ratio",
                   true},
                  {epoch_core::IODataType::Decimal, "return_on_assets", "ROA",
                   true},
                  {epoch_core::IODataType::Decimal, "return_on_equity", "ROE",
                   true},
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "from_date",
                      .name = "From Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Start date for quotes (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "to_date",
                      .name = "To Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date for quotes (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "limit",
                      .name = "Limit",
                      .type = epoch_core::MetaDataOptionType::Integer,
                      .defaultValue = MetaDataOptionDefinition(50000.0),
                      .min = 1,
                      .max = 50000,
                      .desc = "Maximum number of quotes to return"},
              },
          .isCrossSectional = false,
          .desc = "Load quote (NBBO) tick data . Provides "
                  "best bid/ask prices and sizes with microsecond timestamps "
                  "for market microstructure analysis.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "ticker", "Ticker", true},
                  {epoch_core::IODataType::Decimal, "ask_price", "Ask Price",
                   true},
                  {epoch_core::IODataType::Decimal, "bid_price", "Bid Price",
                   true},
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "from_date",
                      .name = "From Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Start date for trades (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "to_date",
                      .name = "To Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date for trades (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "limit",
                      .name = "Limit",
                      .type = epoch_core::MetaDataOptionType::Integer,
                      .defaultValue = MetaDataOptionDefinition(50000.0),
                      .min = 1,
                      .max = 50000,
                      .desc = "Maximum number of trades to return"},
              },
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
          .options =
              {
                  MetaDataOption{
                      .id = "ticker",
                      .name = "Ticker Symbol",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Stock ticker symbol (e.g., AAPL, TSLA)"},
                  MetaDataOption{
                      .id = "from_date",
                      .name = "From Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "Start date (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "to_date",
                      .name = "To Date",
                      .type = epoch_core::MetaDataOptionType::String,
                      .defaultValue = MetaDataOptionDefinition(std::string("")),
                      .desc = "End date (YYYY-MM-DD)"},
                  MetaDataOption{
                      .id = "is_eod",
                      .name = "End of Day Bars",
                      .type = epoch_core::MetaDataOptionType::Boolean,
                      .defaultValue = MetaDataOptionDefinition(true),
                      .desc = "True for daily bars, false for intraday"},
                  MetaDataOption{
                      .id = "adjusted",
                      .name = "Adjusted for Splits/Dividends",
                      .type = epoch_core::MetaDataOptionType::Boolean,
                      .defaultValue = MetaDataOptionDefinition(true),
                      .desc = "Adjust prices for corporate actions"},
              },
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
