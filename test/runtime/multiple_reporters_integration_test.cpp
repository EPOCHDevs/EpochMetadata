#include <catch2/catch_test_macros.hpp>
#include "runtime/orchestrator.h"
#include "epoch_protos/tearsheet.pb.h"
#include <epoch_dashboard/tearsheet/card_builder.h>
#include <epoch_dashboard/tearsheet/lines_chart_builder.h>
#include <epoch_dashboard/tearsheet/bar_chart_builder.h>
#include <epoch_dashboard/tearsheet/table_builder.h>

using namespace epoch_flow::runtime;

// Helper to create reports for testing using builders
namespace {
    // Helper to create Scalar from string
    epoch_proto::Scalar makeScalar(const std::string& value) {
        epoch_proto::Scalar scalar;
        scalar.set_string_value(value);
        return scalar;
    }

    epoch_proto::TearSheet CreatePerformanceReport() {
        epoch_proto::TearSheet ts;

        // Build cards using CardBuilder
        auto* cardsContainer = ts.mutable_cards();

        // Card 1: Total Return
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Total Return")
                       .setValue(makeScalar("25.5%"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Performance")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Card 2: Sharpe Ratio
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Sharpe Ratio")
                       .setValue(makeScalar("1.85"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Performance")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Build chart using LinesChartBuilder
        {
            epoch_tearsheet::LinesChartBuilder chartBuilder;
            chartBuilder.setTitle("Equity Curve")
                        .setCategory("Performance");

            auto chart = chartBuilder.build();
            ts.mutable_charts()->add_charts()->CopyFrom(chart);
        }

        return ts;
    }

    epoch_proto::TearSheet CreateRiskReport() {
        epoch_proto::TearSheet ts;

        // Build cards using CardBuilder
        auto* cardsContainer = ts.mutable_cards();

        // Card 1: Max Drawdown
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Max Drawdown")
                       .setValue(makeScalar("-12.3%"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Risk")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Card 2: Volatility
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Volatility")
                       .setValue(makeScalar("15.2%"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Risk")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Build table using TableBuilder
        {
            epoch_tearsheet::TableBuilder tableBuilder;
            tableBuilder.setTitle("Risk Metrics")
                       .setCategory("Risk");

            auto table = tableBuilder.build();
            ts.mutable_tables()->add_tables()->CopyFrom(table);
        }

        return ts;
    }

    epoch_proto::TearSheet CreateTradingReport() {
        epoch_proto::TearSheet ts;

        // Build cards using CardBuilder
        auto* cardsContainer = ts.mutable_cards();

        // Card 1: Total Trades
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Total Trades")
                       .setValue(makeScalar("127"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Trading")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Card 2: Win Rate
        {
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("Win Rate")
                       .setValue(makeScalar("58.3%"));

            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("Trading")
                       .addCardData(dataBuilder.build());

            cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
        }

        // Build chart using BarChartBuilder
        {
            epoch_tearsheet::BarChartBuilder chartBuilder;
            chartBuilder.setTitle("Trade Distribution")
                        .setCategory("Trading");

            auto chart = chartBuilder.build();
            ts.mutable_charts()->add_charts()->CopyFrom(chart);
        }

        return ts;
    }
}

TEST_CASE("Multiple Reporters Integration - Report Merging", "[multiple_reporters][integration]") {

    SECTION("Simulating multiple reporters for single asset - all content merged") {
        // Simulate what happens when multiple reporters contribute reports for the same asset
        auto performanceReport = CreatePerformanceReport();
        auto riskReport = CreateRiskReport();
        auto tradingReport = CreateTradingReport();

        // This is what the flow graph does: start with first report
        epoch_proto::TearSheet mergedReport = performanceReport;

        // Then merge in additional reports from other reporters
        DataFlowRuntimeOrchestrator::MergeReportInPlace(mergedReport, riskReport, "risk_reporter");
        DataFlowRuntimeOrchestrator::MergeReportInPlace(mergedReport, tradingReport, "trading_reporter");

        // Verify all content from all three reporters is present
        // Cards: 2 (perf) + 2 (risk) + 2 (trading) = 6
        REQUIRE(mergedReport.cards().cards_size() == 6);

        // Charts: 1 (perf) + 0 (risk) + 1 (trading) = 2
        REQUIRE(mergedReport.charts().charts_size() == 2);

        // Tables: 0 (perf) + 1 (risk) + 0 (trading) = 1
        REQUIRE(mergedReport.tables().tables_size() == 1);

        // Verify cards from all categories are present
        bool hasPerformance = false, hasRisk = false, hasTrading = false;
        for (const auto& card : mergedReport.cards().cards()) {
            if (card.category() == "Performance") hasPerformance = true;
            if (card.category() == "Risk") hasRisk = true;
            if (card.category() == "Trading") hasTrading = true;
        }
        REQUIRE(hasPerformance);
        REQUIRE(hasRisk);
        REQUIRE(hasTrading);

        // Verify charts are present - check chart types using oneof
        bool hasEquityCurve = false, hasTradeDistribution = false;
        for (const auto& chart : mergedReport.charts().charts()) {
            // Check if it's a lines chart with title "Equity Curve"
            if (chart.has_lines_def()) {
                const auto& chartDef = chart.lines_def().chart_def();
                if (chartDef.title() == "Equity Curve") {
                    hasEquityCurve = true;
                }
            }
            // Check if it's a bar chart with title "Trade Distribution"
            if (chart.has_bar_def()) {
                const auto& chartDef = chart.bar_def().chart_def();
                if (chartDef.title() == "Trade Distribution") {
                    hasTradeDistribution = true;
                }
            }
        }
        REQUIRE(hasEquityCurve);
        REQUIRE(hasTradeDistribution);
    }

    SECTION("Merging reports preserves all unique content") {
        auto report1 = CreatePerformanceReport();
        auto report2 = CreateRiskReport();

        // Verify initial state
        REQUIRE(report1.cards().cards_size() == 2);
        REQUIRE(report2.cards().cards_size() == 2);

        // Merge
        DataFlowRuntimeOrchestrator::MergeReportInPlace(report1, report2, "reporter2");

        // All cards should be present
        REQUIRE(report1.cards().cards_size() == 4);

        // Verify no duplicates by checking categories
        std::set<std::string> categories;
        for (const auto& card : report1.cards().cards()) {
            categories.insert(card.category());
        }
        REQUIRE(categories.size() == 2);  // "Performance" and "Risk"
    }

    SECTION("Stress test - many reporters") {
        epoch_proto::TearSheet baseReport;

        // Simulate 10 different reporters each contributing content
        for (int i = 0; i < 10; ++i) {
            epoch_proto::TearSheet additionalReport;

            // Each reporter contributes 2 cards
            auto* cardsContainer = additionalReport.mutable_cards();
            for (int j = 0; j < 2; ++j) {
                epoch_tearsheet::CardDataBuilder dataBuilder;
                dataBuilder.setTitle("Metric_A_" + std::to_string(i * 2 + j))
                           .setValue(makeScalar("Value_" + std::to_string(i * 2 + j)));

                epoch_tearsheet::CardBuilder cardBuilder;
                cardBuilder.setCategory("Category_" + std::to_string(i))
                           .addCardData(dataBuilder.build());

                cardsContainer->add_cards()->CopyFrom(cardBuilder.build());
            }

            // Each reporter contributes 1 chart
            {
                epoch_tearsheet::LinesChartBuilder chartBuilder;
                chartBuilder.setTitle("Chart_" + std::to_string(i))
                            .setCategory("Category_" + std::to_string(i));

                auto chart = chartBuilder.build();
                additionalReport.mutable_charts()->add_charts()->CopyFrom(chart);
            }

            // Merge this reporter's contributions
            DataFlowRuntimeOrchestrator::MergeReportInPlace(
                baseReport, additionalReport, "reporter_" + std::to_string(i));
        }

        // Verify all content accumulated
        REQUIRE(baseReport.cards().cards_size() == 20);  // 10 reporters * 2 cards
        REQUIRE(baseReport.charts().charts_size() == 10); // 10 reporters * 1 chart

        // Verify size is reasonable (not exponentially growing due to bugs)
        size_t finalSize = baseReport.ByteSizeLong();
        REQUIRE(finalSize > 0);
        REQUIRE(finalSize < 1000000);  // Reasonable upper bound
    }

    SECTION("Empty report merges gracefully") {
        auto report1 = CreatePerformanceReport();
        epoch_proto::TearSheet emptyReport;

        size_t originalSize = report1.ByteSizeLong();

        DataFlowRuntimeOrchestrator::MergeReportInPlace(report1, emptyReport, "empty_reporter");

        // Size should be unchanged or very similar
        REQUIRE(report1.ByteSizeLong() >= originalSize * 0.95);
        REQUIRE(report1.ByteSizeLong() <= originalSize * 1.05);

        // Content should be unchanged
        REQUIRE(report1.cards().cards_size() == 2);
        REQUIRE(report1.charts().charts_size() == 1);
    }

    SECTION("Merging into empty report") {
        epoch_proto::TearSheet emptyReport;
        auto report1 = CreateTradingReport();

        DataFlowRuntimeOrchestrator::MergeReportInPlace(emptyReport, report1, "first_reporter");

        // Empty report should now contain all content from report1
        REQUIRE(emptyReport.cards().cards_size() == 2);
        REQUIRE(emptyReport.charts().charts_size() == 1);
    }
}
