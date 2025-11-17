#include <gtest/gtest.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/index.h>
#include <arrow/api.h>
#include <vector>

TEST(ReindexReproTest, DailyToMonthlyReindex) {
    std::cout << "\n=== Testing Daily -> Monthly Reindex ===" << std::endl;

    // Create daily timestamps (10 days)
    std::vector<int64_t> daily_timestamps;
    std::vector<bool> daily_booleans;

    for (int i = 0; i < 10; i++) {
        daily_timestamps.push_back(1704067200000 + i * 86400000); // 2024-01-01 + i days (milliseconds)
        daily_booleans.push_back(i % 3 == 0); // True every 3rd day
    }

    std::cout << "Daily data: " << daily_timestamps.size() << " rows" << std::endl;

    // Create monthly timestamps (month-end) - 3 months
    std::vector<int64_t> monthly_timestamps = {
        1706745600000,  // 2024-01-31
        1709251200000,  // 2024-02-29
        1711929600000   // 2024-03-31
    };

    std::cout << "Monthly index: " << monthly_timestamps.size() << " rows" << std::endl;

    // Create daily DataFrame
    auto daily_index = epoch_frame::make_datetime_index(daily_timestamps);

    arrow::BooleanBuilder bool_builder;
    for (auto val : daily_booleans) {
        ASSERT_TRUE(bool_builder.Append(val).ok());
    }
    auto bool_array_result = bool_builder.Finish();
    ASSERT_TRUE(bool_array_result.ok());

    auto daily_df = epoch_frame::make_dataframe(
        daily_index,
        {arrow::ChunkedArray::Make({bool_array_result.ValueOrDie()}).ValueOrDie()},
        {"calendar_event"}
    );

    std::cout << "Created daily DataFrame with " << daily_df.num_rows() << " rows" << std::endl;

    // Create monthly index (target for reindex)
    auto monthly_index = epoch_frame::make_datetime_index(monthly_timestamps);

    std::cout << "Attempting reindex to monthly index..." << std::endl;

    // Perform reindex
    auto reindexed_df = daily_df.reindex(monthly_index);

    std::cout << "Reindex result: " << reindexed_df.num_rows() << " rows" << std::endl;
    std::cout << "Expected: " << monthly_index->size() << " rows" << std::endl;

    // THE BUG: This assertion should pass but might fail
    EXPECT_EQ(reindexed_df.num_rows(), static_cast<int64_t>(monthly_index->size()))
        << "REINDEX BUG: Result size (" << reindexed_df.num_rows()
        << ") doesn't match target index size (" << monthly_index->size() << ")";

    if (reindexed_df.num_rows() != static_cast<int64_t>(monthly_index->size())) {
        std::cout << "\n✗ BUG REPRODUCED!" << std::endl;
        std::cout << "  Reindex returned wrong number of rows" << std::endl;
        std::cout << "  This matches the event_marker failure pattern" << std::endl;
    } else {
        std::cout << "\n✓ Reindex working correctly" << std::endl;
        std::cout << "  Result matches target index size" << std::endl;
    }
}
