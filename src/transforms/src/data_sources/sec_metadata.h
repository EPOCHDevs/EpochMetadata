#pragma once

#include "epoch_metadata/transforms/metadata.h"

namespace epoch_metadata::transform {

// Factory function to create metadata for all SEC data source transforms
inline std::vector<epoch_metadata::transforms::TransformsMetaData>
MakeSECDataSources() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // 1. Form 13F Institutional Holdings
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "form13f_holdings",
          .category = epoch_core::TransformCategory::DataSource,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Form 13F Holdings",
          .options = {},
          .isCrossSectional = false,
          .desc = "Load SEC Form 13F institutional holdings data. "
                  "Track holdings reported by investment managers with $100M+ AUM. "
                  "Form 13F-HR is filed quarterly (45 days after quarter end) and "
                  "discloses long positions in US equities and convertible debt.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::Decimal, "shares",
                   "Number of Shares Held", true},
                  {epoch_core::IODataType::Decimal, "value",
                   "Position Value (USD)", true},
                  {epoch_core::IODataType::String, "security_type",
                   "Security Type", true},
                  {epoch_core::IODataType::String, "investment_discretion",
                   "Investment Discretion", true},
                  {epoch_core::IODataType::String, "institution_name",
                   "Institution Name", true},
                  {epoch_core::IODataType::String, "filing_date",
                   "Filing Date", true},
                  {epoch_core::IODataType::String, "period_end",
                   "Reporting Period End", true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"sec", "13f", "institutional", "holdings", "smart-money",
                   "fundamentals"},
          .requiresTimeFrame = true,
          .requiredDataSources = {"c"},
          .strategyTypes = {"fundamental-analysis", "follow-smart-money",
                            "institutional-flow", "ownership-analysis"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Track institutional ownership changes for follow-the-smart-money "
              "strategies. Monitor hedge fund and institutional portfolio changes "
              "quarterly. Identify concentrated ownership positions and sector "
              "crowding. Use to detect institutional accumulation/distribution "
              "patterns. Combine with price data for ownership-momentum strategies.",
          .limitations =
              "Quarterly filing frequency only (Q1-Q4). 45-day reporting lag "
              "after quarter end means holdings data is stale. Only long positions "
              "disclosed - short positions and derivatives not included. $100M+ AUM "
              "threshold excludes smaller managers. Position changes may be "
              "partially attributed to price movements vs. actual buying/selling. "
              "Requires external SEC-API data loader with API key.",
      });

  // 2. Insider Trading
  metadataList.emplace_back(
      epoch_metadata::transforms::TransformsMetaData{
          .id = "insider_trading",
          .category = epoch_core::TransformCategory::DataSource,
          .plotKind = epoch_core::TransformPlotKind::Null,
          .name = "Insider Trading",
          .options =
              {
                  MetaDataOption{
                      .id = "transaction_code",
                      .name = "Transaction Type",
                      .type = epoch_core::MetaDataOptionType::Select,
                      .defaultValue = MetaDataOptionDefinition(std::string("All")),
                      .selectOption =
                          {
                              {"All Transactions", "All"},
                              {"Purchase (P)", "P"},
                              {"Sale (S)", "S"},
                              {"Award/Grant (A)", "A"},
                              {"Exercise/Conversion (M)", "M"},
                          },
                      .desc = "Filter by transaction type. P=Purchase (bullish), "
                              "S=Sale (bearish), A=Award/Grant, M=Exercise/Conversion"},
              },
          .isCrossSectional = false,
          .desc = "Load SEC insider trading data from Forms 3, 4, 5, and 144. "
                  "Track transactions made by company insiders (officers, directors, "
                  "10%+ owners). Form 4 filed within 2 business days of transaction. "
                  "Use for insider sentiment and smart-money signals.",
          .inputs = {},
          .outputs =
              {
                  {epoch_core::IODataType::String, "transaction_date",
                   "Transaction Date", true},
                  {epoch_core::IODataType::String, "owner_name",
                   "Insider Name", true},
                  {epoch_core::IODataType::String, "transaction_code",
                   "Transaction Code (P/S/A/M)", true},
                  {epoch_core::IODataType::Decimal, "shares",
                   "Number of Shares", true},
                  {epoch_core::IODataType::Decimal, "price",
                   "Price Per Share", true},
                  {epoch_core::IODataType::Decimal, "ownership_after",
                   "Ownership After Transaction", true},
                  {epoch_core::IODataType::String, "filing_date",
                   "Filing Date", true},
              },
          .atLeastOneInputRequired = false,
          .tags = {"sec", "insider", "trading", "form-4", "smart-money",
                   "sentiment"},
          .requiresTimeFrame = true,
          .requiredDataSources = {"c"},
          .strategyTypes = {"insider-sentiment", "smart-money",
                            "signal-generation", "event-driven"},
          .assetRequirements = {"single-asset"},
          .usageContext =
              "Track insider buying/selling for sentiment signals. Insider "
              "purchases are generally bullish signals (insiders buying on private "
              "information or confidence). Cluster of insider buys can signal "
              "undervaluation. Focus on open-market purchases (code P) vs. automatic "
              "sales (10b5-1 plans). Large purchases or director/CEO buys carry more "
              "weight. Aggregate multiple insider transactions for stronger signals. "
              "Combine with price momentum for confirmation.",
          .limitations =
              "2-day reporting lag for Form 4 means some timing delay. Doesn't "
              "capture all insider activity - derivatives and indirect holdings may "
              "be excluded. Pre-arranged trading plans (Rule 10b5-1) dilute signal "
              "quality as sales may be scheduled regardless of outlook. Sales can be "
              "for tax/diversification reasons, not bearish views. Transaction codes "
              "are complex - not all transactions are open-market buys/sells. "
              "Requires external SEC-API data loader with API key.",
      });

  return metadataList;
}

} // namespace epoch_metadata::transform
