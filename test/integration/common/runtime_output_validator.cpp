#include "runtime_output_validator.h"
#include "tearsheet_comparator.h"
#include "event_marker_comparator.h"
#include <epoch_frame/serialization.h>
#include <fstream>
#include <sstream>

namespace epoch_script::runtime::test {

RuntimeOutputValidator::ValidationResult
RuntimeOutputValidator::ValidateDataframes(
    const TimeFrameAssetDataFrameMap& actual_dataframes,
    const fs::path& expected_dir)
{
    // If expected_dir doesn't exist or is empty, skip validation
    if (!fs::exists(expected_dir) || fs::is_empty(expected_dir)) {
        return ValidationResult::Success();
    }

    // Load expected dataframes from CSV files
    // Expected format: {timeframe}_{asset}_{output_id}.csv
    for (const auto& entry : fs::directory_iterator(expected_dir))
    {
        if (entry.path().extension() != ".csv") {
            continue;
        }

        std::string filename = entry.path().stem().string();

        // Parse filename to extract timeframe, asset, and output_id
        // Format: timeframe_asset_outputid.csv (e.g., 1D_AAPL_rsi_result.csv)
        size_t first_underscore = filename.find('_');
        size_t last_underscore = filename.rfind('_');

        if (first_underscore == std::string::npos || last_underscore == std::string::npos) {
            return ValidationResult::Failure(
                "Invalid expected dataframe filename format: " + filename +
                " (expected: timeframe_asset_outputid.csv)");
        }

        std::string timeframe = filename.substr(0, first_underscore);
        std::string asset = filename.substr(first_underscore + 1,
                                           last_underscore - first_underscore - 1);
        std::string output_id = filename.substr(last_underscore + 1);

        // Load expected dataframe
        auto expected_df_result = epoch_frame::read_csv_file(
            entry.path().string(), epoch_frame::CSVReadOptions{});

        if (!expected_df_result.ok()) {
            return ValidationResult::Failure(
                "Failed to load expected dataframe from " + entry.path().string() +
                ": " + expected_df_result.status().ToString());
        }

        // Find corresponding actual dataframe
        if (!actual_dataframes.contains(timeframe)) {
            return ValidationResult::Failure(
                "Missing timeframe in actual output: " + timeframe);
        }

        const auto& asset_map = actual_dataframes.at(timeframe);
        if (!asset_map.contains(asset)) {
            return ValidationResult::Failure(
                "Missing asset in actual output: " + asset + " for timeframe " + timeframe);
        }

        auto actual_df = asset_map.at(asset);
        auto expected_df = expected_df_result.ValueOrDie();

        // Normalize: some CSV writers include an implicit 'index' column; ignore it
        auto drop_index = [](epoch_frame::DataFrame df) {
            auto names = df.column_names();
            if (!names.empty() && names.front() == std::string("index")) {
                return df.drop(std::string("index"));
            }
            return df;
        };
        expected_df = drop_index(std::move(expected_df));
        actual_df = drop_index(std::move(actual_df));

        // Compare dataframes
        // TODO: Implement robust dataframe comparison
        if (actual_df.num_rows() != expected_df.num_rows()) {
            return ValidationResult::Failure(
                "Row count mismatch for " + timeframe + "/" + asset + "/" + output_id +
                ": expected " + std::to_string(expected_df.num_rows()) +
                ", got " + std::to_string(actual_df.num_rows()));
        }

        if (actual_df.num_cols() != expected_df.num_cols()) {
            return ValidationResult::Failure(
                "Column count mismatch for " + timeframe + "/" + asset + "/" + output_id +
                ": expected " + std::to_string(expected_df.num_cols()) +
                ", got " + std::to_string(actual_df.num_cols()));
        }
    }

    return ValidationResult::Success();
}

RuntimeOutputValidator::ValidationResult
RuntimeOutputValidator::ValidateTearsheets(
    const AssetReportMap& actual_reports,
    const fs::path& expected_dir)
{
    // If expected_dir doesn't exist or is empty, skip validation
    if (!fs::exists(expected_dir) || fs::is_empty(expected_dir)) {
        return ValidationResult::Success();
    }

    // Load expected tearsheets from proto files
    // Expected format: {asset}.pb or {asset}.json
    for (const auto& entry : fs::directory_iterator(expected_dir))
    {
        std::string ext = entry.path().extension().string();
        if (ext != ".pb" && ext != ".json") {
            continue;
        }

        std::string asset = entry.path().stem().string();

        // Find corresponding actual report
        if (!actual_reports.contains(asset)) {
            return ValidationResult::Failure(
                "Missing report for asset: " + asset);
        }

        const auto& actual_report = actual_reports.at(asset);

        // Load expected report
        epoch_proto::TearSheet expected_report;

        if (ext == ".pb") {
            // Binary protobuf format
            std::ifstream input(entry.path(), std::ios::binary);
            if (!expected_report.ParseFromIstream(&input)) {
                return ValidationResult::Failure(
                    "Failed to parse expected report from " + entry.path().string());
            }
        } else {
            // JSON format (TODO: implement JSON parsing if needed)
            return ValidationResult::Failure(
                "JSON format for tearsheets not yet supported");
        }

        // Compare reports using TearSheetComparator
        std::string diff;
        if (!TearSheetComparator::Compare(expected_report, actual_report, diff)) {
            return ValidationResult::Failure(
                "Report mismatch for asset " + asset + ":\n" + diff);
        }
    }

    return ValidationResult::Success();
}

RuntimeOutputValidator::ValidationResult
RuntimeOutputValidator::ValidateEventMarkers(
    const AssetEventMarkerMap& actual_event_markers,
    const fs::path& expected_dir)
{
    // If expected_dir doesn't exist or is empty, skip validation
    if (!fs::exists(expected_dir) || fs::is_empty(expected_dir)) {
        return ValidationResult::Success();
    }

    // Load expected event markers from JSON files
    // Expected format: {asset}.json
    for (const auto& entry : fs::directory_iterator(expected_dir))
    {
        if (entry.path().extension() != ".json") {
            continue;
        }

        std::string asset = entry.path().stem().string();

        // Find corresponding actual event markers
        if (!actual_event_markers.contains(asset)) {
            return ValidationResult::Failure(
                "Missing event markers for asset: " + asset);
        }

        const auto& actual_markers = actual_event_markers.at(asset);

        // Load expected event markers from JSON
        std::string expected_json = SelectorComparator::LoadJson(entry.path());

        // Convert actual event markers to JSON
        std::string actual_json = SelectorComparator::ToJson(actual_markers);

        // Compare using SelectorComparator
        std::string diff;
        if (!SelectorComparator::Compare(expected_json, actual_json, diff)) {
            return ValidationResult::Failure(
                "Event marker mismatch for asset " + asset + ":\n" + diff);
        }
    }

    return ValidationResult::Success();
}

} // namespace epoch_script::runtime::test
