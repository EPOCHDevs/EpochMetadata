/**
 * Standalone reproduction test for reindex bug
 * Tests daily -> monthly reindex operation that's failing in event_marker
 */

#include <epoch_frame/dataframe.h>
#include <epoch_frame/index.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <arrow/api.h>
#include <arrow/compute/initialize.h>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    // Initialize Arrow compute functions
    auto arrow_status = arrow::compute::Initialize();
    if (!arrow_status.ok()) {
        std::cerr << "Failed to initialize Arrow compute: " << arrow_status.message() << "\n";
        return 1;
    }
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "  Reindex Bug Reproduction Test (EpochFrame)\n";
    std::cout << "================================================================\n";
    std::cout << "\n";

    try {
        // ============================================================
        // Setup: Create daily boolean data (like calendar events)
        // ============================================================

        std::cout << "Creating Daily Boolean DataFrame...\n";
        std::cout << "------------------------------------\n";

        std::vector<int64_t> daily_timestamps;
        std::vector<bool> daily_booleans;

        // Generate 10 daily values (simplified from real 182 days)
        int64_t start_ms = 1704067200000;  // 2024-01-01 00:00:00 UTC in milliseconds
        int64_t day_ms = 86400000;         // milliseconds in a day

        for (int i = 0; i < 10; i++) {
            daily_timestamps.push_back(start_ms + i * day_ms);
            daily_booleans.push_back(i % 3 == 0); // True every 3rd day
        }

        std::cout << "  Rows: " << daily_timestamps.size() << "\n";
        std::cout << "  Type: Boolean (calendar events)\n";
        std::cout << "  Range: 2024-01-01 to 2024-01-10\n";
        std::cout << "\n";

        // Create daily DataFrame
        auto daily_index = epoch_frame::factory::index::make_datetime_index(daily_timestamps);

        arrow::BooleanBuilder bool_builder;
        for (auto val : daily_booleans) {
            auto status = bool_builder.Append(val);
            if (!status.ok()) {
                std::cerr << "Error appending boolean: " << status.message() << "\n";
                return 1;
            }
        }

        auto bool_array_result = bool_builder.Finish();
        if (!bool_array_result.ok()) {
            std::cerr << "Error building boolean array: " << bool_array_result.status().message() << "\n";
            return 1;
        }

        auto daily_df = epoch_frame::make_dataframe(
            daily_index,
            {arrow::ChunkedArray::Make({bool_array_result.ValueOrDie()}).ValueOrDie()},
            {"calendar_event"}
        );

        std::cout << "✓ Created daily DataFrame with " << daily_df.num_rows() << " rows\n";
        std::cout << "\n";

        // ============================================================
        // Setup: Create monthly index (like CPI timeframe)
        // ============================================================

        std::cout << "Creating Monthly Target Index...\n";
        std::cout << "---------------------------------\n";

        // Month-end timestamps (simplified from real 122 months)
        std::vector<int64_t> monthly_timestamps = {
            1706745600000,  // 2024-01-31 00:00:00 UTC
            1709251200000,  // 2024-02-29 00:00:00 UTC
            1711929600000   // 2024-03-31 00:00:00 UTC
        };

        std::cout << "  Rows: " << monthly_timestamps.size() << "\n";
        std::cout << "  Type: Month-end timestamps\n";
        std::cout << "  Range: Jan 2024 to Mar 2024\n";
        std::cout << "\n";

        auto monthly_index = epoch_frame::factory::index::make_datetime_index(monthly_timestamps);

        // ============================================================
        // TEST: Perform reindex (daily -> monthly)
        // ============================================================

        std::cout << "Performing Reindex Operation...\n";
        std::cout << "--------------------------------\n";
        std::cout << "  Source:  Daily (10 rows)\n";
        std::cout << "  Target:  Monthly (3 rows)\n";
        std::cout << "  Expected Result: 3 rows (matching target index)\n";
        std::cout << "\n";

        auto reindexed_df = daily_df.reindex(monthly_index);

        std::cout << "Reindex completed.\n";
        std::cout << "\n";

        // ============================================================
        // VERIFY: Check if result matches expected size
        // ============================================================

        std::cout << "Results:\n";
        std::cout << "--------\n";
        std::cout << "  Result rows:       " << reindexed_df.num_rows() << "\n";
        std::cout << "  Target index size: " << monthly_index->size() << "\n";
        std::cout << "\n";

        bool size_matches = reindexed_df.num_rows() == static_cast<int64_t>(monthly_index->size());

        if (size_matches) {
            std::cout << "✓ SUCCESS: Row count MATCHES target index\n";
            std::cout << "\n";
            std::cout << "  Reindex is working correctly.\n";
            std::cout << "  Result has exactly " << monthly_index->size() << " rows as expected.\n";
            std::cout << "\n";
            return 0;
        } else {
            std::cout << "✗ FAILURE: Row count MISMATCH!\n";
            std::cout << "\n";
            std::cout << "  *** BUG REPRODUCED ***\n";
            std::cout << "\n";
            std::cout << "  Expected: " << monthly_index->size() << " rows\n";
            std::cout << "  Got:      " << reindexed_df.num_rows() << " rows\n";
            std::cout << "  Delta:    " << (reindexed_df.num_rows() - monthly_index->size()) << " extra rows\n";
            std::cout << "\n";
            std::cout << "  This matches the event_marker failure pattern:\n";
            std::cout << "  - Test case shows 182 rows vs 122 expected\n";
            std::cout << "  - RIGHT_OUTER join preserving wrong side\n";
            std::cout << "  - methods_helper.cpp:328 assertion fails\n";
            std::cout << "\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cout << "\n";
        std::cout << "✗ EXCEPTION: " << e.what() << "\n";
        std::cout << "\n";
        return 1;
    }
}
