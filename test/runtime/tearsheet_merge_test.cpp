#include <catch2/catch_test_macros.hpp>
#include "runtime/orchestrator.h"
#include "epoch_protos/tearsheet.pb.h"
#include <epoch_dashboard/tearsheet/card_builder.h>
#include <epoch_dashboard/tearsheet/lines_chart_builder.h>
#include <epoch_dashboard/tearsheet/table_builder.h>

#include "orchestrator.h"

using namespace epoch_flow::runtime;

// Helper to create a report with specific content using builders
epoch_proto::TearSheet CreateTestReport(int numCards, int numCharts, int numTables) {
    epoch_proto::TearSheet report;

    // Add cards using CardBuilder
    if (numCards > 0) {
        auto* cards = report.mutable_cards();
        for (int i = 0; i < numCards; ++i) {
            epoch_tearsheet::CardBuilder cardBuilder;
            cardBuilder.setCategory("test_category_" + std::to_string(i));

            // Add a simple card data entry
            epoch_tearsheet::CardDataBuilder dataBuilder;
            dataBuilder.setTitle("card_" + std::to_string(i));
            epoch_proto::Scalar scalar;
            scalar.set_string_value(std::to_string(i));
            dataBuilder.setValue(scalar);

            cardBuilder.addCardData(dataBuilder.build());
            auto card = cardBuilder.build();
            cards->add_cards()->CopyFrom(card);
        }
    }

    // Add charts using LinesChartBuilder
    if (numCharts > 0) {
        auto* charts = report.mutable_charts();
        for (int i = 0; i < numCharts; ++i) {
            epoch_tearsheet::LinesChartBuilder chartBuilder;
            chartBuilder.setTitle("test_chart_" + std::to_string(i))
                        .setCategory("test_category");

            auto chart = chartBuilder.build();
            charts->add_charts()->CopyFrom(chart);
        }
    }

    // Add tables using TableBuilder
    if (numTables > 0) {
        auto* tables = report.mutable_tables();
        for (int i = 0; i < numTables; ++i) {
            epoch_tearsheet::TableBuilder tableBuilder;
            tableBuilder.setTitle("test_table_" + std::to_string(i))
                       .setCategory("test_category");

            auto table = tableBuilder.build();
            tables->add_tables()->CopyFrom(table);
        }
    }

    return report;
}

TEST_CASE("Report merging functionality", "[report_merge]") {

    SECTION("Basic report merging") {
        auto report1 = CreateTestReport(2, 1, 0);  // 2 cards, 1 chart, 0 tables
        auto report2 = CreateTestReport(1, 2, 3);  // 1 card, 2 charts, 3 tables

        // Verify initial counts
        REQUIRE(report1.cards().cards_size() == 2);
        REQUIRE(report1.charts().charts_size() == 1);
        REQUIRE(report1.tables().tables_size() == 0);

        REQUIRE(report2.cards().cards_size() == 1);
        REQUIRE(report2.charts().charts_size() == 2);
        REQUIRE(report2.tables().tables_size() == 3);

        // Perform merge
        DataFlowRuntimeOrchestrator::MergeReportInPlace(report1, report2, "test_transform");

        // Verify merged counts (should be additive)
        REQUIRE(report1.cards().cards_size() == 3);  // 2 + 1
        REQUIRE(report1.charts().charts_size() == 3); // 1 + 2
        REQUIRE(report1.tables().tables_size() == 3); // 0 + 3
    }

    SECTION("Merging empty reports") {
        auto report1 = CreateTestReport(1, 1, 1);
        auto emptyReport = CreateTestReport(0, 0, 0);

        // Merge empty into non-empty
        DataFlowRuntimeOrchestrator::MergeReportInPlace(report1, emptyReport, "empty_transform");

        // Should remain unchanged
        REQUIRE(report1.cards().cards_size() == 1);
        REQUIRE(report1.charts().charts_size() == 1);
        REQUIRE(report1.tables().tables_size() == 1);

        // Merge non-empty into empty
        auto anotherEmpty = CreateTestReport(0, 0, 0);
        DataFlowRuntimeOrchestrator::MergeReportInPlace(anotherEmpty, report1, "non_empty_transform");

        // Should now have the merged content
        REQUIRE(anotherEmpty.cards().cards_size() == 1);
        REQUIRE(anotherEmpty.charts().charts_size() == 1);
        REQUIRE(anotherEmpty.tables().tables_size() == 1);
    }

    SECTION("Multiple successive merges") {
        auto baseReport = CreateTestReport(1, 0, 0);

        // Perform multiple merges
        for (int i = 0; i < 5; ++i) {
            auto additionalReport = CreateTestReport(1, 1, 1);
            DataFlowRuntimeOrchestrator::MergeReportInPlace(baseReport, additionalReport,
                                                          "transform_" + std::to_string(i));
        }

        // Should have accumulated content
        REQUIRE(baseReport.cards().cards_size() == 6);   // 1 + 5*1
        REQUIRE(baseReport.charts().charts_size() == 5); // 0 + 5*1
        REQUIRE(baseReport.tables().tables_size() == 5); // 0 + 5*1
    }

    SECTION("Byte size changes during merge") {
        auto report1 = CreateTestReport(5, 5, 5);
        auto report2 = CreateTestReport(3, 3, 3);

        size_t originalSize = report1.ByteSizeLong();
        size_t additionalSize = report2.ByteSizeLong();

        DataFlowRuntimeOrchestrator::MergeReportInPlace(report1, report2, "size_test");

        size_t mergedSize = report1.ByteSizeLong();

        // Merged size should be larger than either original size
        REQUIRE(mergedSize > originalSize);
        REQUIRE(mergedSize > additionalSize);

        // The merged size should be approximately the sum (allowing for protobuf overhead)
        REQUIRE(mergedSize >= (originalSize + additionalSize) * 0.8); // Allow 20% overhead tolerance
    }
}