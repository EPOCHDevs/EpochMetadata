//
// Created by adesola on 1/14/25.
//

#include "epoch_metadata/epoch_folio/metadata.h"

namespace epoch_metadata::cppFolio {
std::vector<CategoryMetaData> GetCategoryMetaData() {
    return {
        CategoryMetaData{
            epoch_core::CppFolioCategory::StrategyBenchmark,
            "Strategy Benchmark",
            "Strategy and benchmark performance comparison"
        },
        CategoryMetaData{
            epoch_core::CppFolioCategory::RiskAnalysis,
            "Risk Analysis",
            "Analysis of portfolio risk metrics"
        },
        CategoryMetaData{
            epoch_core::CppFolioCategory::ReturnsDistribution,
            "Returns Distribution",
            "Distribution analysis of returns"
        },
        CategoryMetaData{
            epoch_core::CppFolioCategory::Positions,
            "Positions",
            "Portfolio position information"
        },
        CategoryMetaData{
            epoch_core::CppFolioCategory::Transactions,
            "Transactions",
            "Trading transaction details"
        },
        CategoryMetaData{
            epoch_core::CppFolioCategory::RoundTrip,
            "Round Trip",
            "Round trip trade analysis"
        }
    };
}
} // namespace epoch_metadata::cppFolio
