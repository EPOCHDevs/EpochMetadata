#pragma once

#include <epoch_core/enum_wrapper.h>

namespace epoch_core {
// Enum representing different Polygon.io data endpoint categories
CREATE_ENUM(PolygonDataType,
            BalanceSheet,     // Balance sheet fundamental data
            IncomeStatement,  // Income statement fundamental data
            CashFlow,         // Cash flow statement fundamental data
            FinancialRatios,  // Financial ratios and metrics
            Quotes,           // Quote (NBBO) data
            Trades,           // Trade tick data
            Aggregates);      // Aggregate bars (OHLCV)
} // namespace epoch_core
