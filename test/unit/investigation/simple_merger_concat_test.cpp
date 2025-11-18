//
// Unit test to reproduce SimpleMerger concat behavior with corporate actions data
// This test loads cached Arrow files and merges them exactly like SimpleMerger does
//
#include "catch.hpp"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/serialization.h>
#include <epoch_frame/common.h>
#include <filesystem>
#include <iostream>

using namespace epoch_frame;

TEST_CASE("SimpleMerger concat reproduces alignment error", "[.investigation][simple_merger][concat]") {
    std::cout << "\n========================================\n";
    std::cout << "SIMPLE MERGER CONCAT TEST\n";
    std::cout << "========================================\n\n";

    // Use the cache directory that was just created by the failing test
    std::filesystem::path cache_dir = "/home/adesola/EpochLab/EpochScript/cache/data";

    // Pick one asset to test (e.g., AAPL)
    std::string asset = "AAPL-Stocks";

    std::cout << "[INFO] Testing asset: " << asset << "\n\n";

    // Load all 4 categories for this asset
    std::vector<std::string> categories = {"Dividends", "Splits", "ShortInterest", "ShortVolume"};
    std::vector<DataFrame> dataframes;

    for (const auto& cat : categories) {
        auto filepath = cache_dir / cat / "Stocks" / (asset + ".arrow");

        if (!std::filesystem::exists(filepath)) {
            std::cout << "[WARN] File not found: " << filepath << "\n";
            continue;
        }

        auto read_result = read_arrow(filepath.string());
        if (!read_result.ok()) {
            std::cout << "[ERROR] Failed to read " << filepath.filename() << ": "
                      << read_result.status().ToString() << "\n";
            continue;
        }

        auto df = read_result.MoveValueUnsafe();
        auto index = df.index();

        std::cout << "[LOADED] " << cat << ": "
                  << df.num_rows() << " rows, "
                  << (index ? index->size() : 0) << " index entries\n";

        // Print first few timestamps
        if (index && index->size() > 0) {
            auto index_array = index->as_chunked_array();
            std::cout << "         Index type: " << index_array->type()->ToString() << "\n";
            std::cout << "         First timestamp: ";

            auto first_chunk = index_array->chunk(0);
            if (first_chunk->length() > 0) {
                auto ts_array = std::static_pointer_cast<arrow::TimestampArray>(first_chunk);
                auto dt = DateTime::fromtimestamp(ts_array->Value(0), "UTC");
                std::cout << dt.repr() << "\n";
            }
        }

        dataframes.push_back(std::move(df));
    }

    std::cout << "\n[INFO] Loaded " << dataframes.size() << " categories\n";

    if (dataframes.empty()) {
        std::cout << "[ERROR] No dataframes loaded, skipping test\n";
        return;
    }

    // Now perform the EXACT concat operation that SimpleMerger does
    std::cout << "\n=== Performing SimpleMerger Concat ===\n";
    std::cout << "JoinType: Outer\n";
    std::cout << "Axis: Column\n";
    std::cout << "Sort: true\n\n";

    std::vector<FrameOrSeries> frames;
    for (auto& df : dataframes) {
        frames.push_back(std::move(df));
    }

    ConcatOptions options;
    options.frames = std::move(frames);
    options.joinType = JoinType::Outer;
    options.axis = AxisType::Column;
    options.sort = true;

    auto merged_df = concat(options);
    auto merged_index = merged_df.index();

    std::cout << "[RESULT] Merged DataFrame:\n";
    std::cout << "         Rows: " << merged_df.num_rows() << "\n";
    std::cout << "         Columns: " << merged_df.num_cols() << "\n";
    std::cout << "         Index size: " << (merged_index ? merged_index->size() : 0) << "\n";

    // Check for duplicate timestamps in result
    if (merged_index) {
        auto index_array = merged_index->as_chunked_array();
        auto vc_result = arrow::compute::ValueCounts(index_array);

        if (vc_result.ok()) {
            auto vc_struct = vc_result.ValueOrDie();
            auto counts = std::static_pointer_cast<arrow::Int64Array>(
                vc_struct->GetFieldByName("counts"));
            auto values = vc_struct->GetFieldByName("values");

            int64_t unique_count = counts->length();
            int64_t duplicate_count = 0;

            for (int64_t i = 0; i < counts->length(); i++) {
                if (counts->Value(i) > 1) {
                    duplicate_count++;

                    auto val_scalar = values->GetScalar(i);
                    if (val_scalar.ok()) {
                        auto ts_scalar = std::static_pointer_cast<arrow::TimestampScalar>(*val_scalar);
                        auto dt = DateTime::fromtimestamp(ts_scalar->value, "UTC");
                        std::cout << "[DUPLICATE] " << dt.repr()
                                  << " appears " << counts->Value(i) << "x\n";
                    }
                }
            }

            std::cout << "\n[SUMMARY] Unique timestamps: " << unique_count << "\n";
            std::cout << "[SUMMARY] Duplicate timestamps: " << duplicate_count << "\n";

            if (duplicate_count > 0) {
                std::cout << "\n❌ CONCAT CREATED DUPLICATES!\n";
            } else {
                std::cout << "\n✅ No duplicates in concat result\n";
            }

            // Check if num_rows matches index size
            if (merged_df.num_rows() != static_cast<size_t>(merged_index->size())) {
                std::cout << "\n❌ ALIGNMENT MISMATCH: num_rows (" << merged_df.num_rows()
                          << ") != index size (" << merged_index->size() << ")\n";
            } else {
                std::cout << "\n✅ Alignment OK: num_rows == index size\n";
            }
        }
    }

    // Save result for Python comparison
    std::filesystem::path output_path = "/tmp/cpp_merged_result.arrow";
    auto write_result = write_arrow(merged_df, output_path.string());

    if (write_result.ok()) {
        std::cout << "\n[INFO] Saved C++ result to: " << output_path << "\n";
    }

    std::cout << "\n========================================\n\n";
}
