#include "test_orchestrator.h"
#include "csv_data_loader.h"
#include <algorithm>
#include <sstream>

namespace epoch_script::test {

runtime::TimeFrameAssetDataFrameMap TestOrchestrator::ProvideTestData(
    const std::string& timeframe)
{
    // TODO: Implement dataloader factory integration
    // For now, return empty map - this will be filled in when you integrate
    // your dataloader factory that provides AAPL/DJI/SP500 data
    runtime::TimeFrameAssetDataFrameMap data_map;

    // Placeholder: This is where you'll call your dataloader factory
    // Example:
    // auto loader = DataLoaderFactory::Create("historical");
    // data_map[timeframe] = loader->LoadAssets({"AAPL-Stock", "DJI-Index", "SP500-Index"});

    return data_map;
}

std::set<std::string> TestOrchestrator::GetDefaultTestAssets()
{
    return {"AAPL-Stock", "DJI-Index", "SP500-Index"};
}

ValidationResult TestOrchestrator::ValidateExecutorOutputs(
    const runtime::TimeFrameAssetDataFrameMap& outputs,
    const RuntimeValidation::OutputColumnValidation& validation)
{
    // Iterate through all timeframes and assets
    for (const auto& [timeframe, asset_map] : outputs)
    {
        for (const auto& [asset, df] : asset_map)
        {
            // Check each expected column
            for (const auto& [column_name, rule] : validation.columns)
            {
                // Check if column exists
                auto col_names = df.column_names();
                if (std::find(col_names.begin(), col_names.end(), column_name) == col_names.end()) {
                    return ValidationResult::Failure(
                        "Executor output missing column '" + column_name +
                        "' for asset '" + asset + "' at timeframe '" + timeframe + "'");
                }

                // Validate column against rule
                if (!ValidateColumn(df, column_name, rule)) {
                    std::string rule_name = (rule == ColumnValidation::at_least_one_valid)
                        ? "at_least_one_valid" : "all_nulls";
                    return ValidationResult::Failure(
                        "Executor output column '" + column_name +
                        "' failed validation '" + rule_name +
                        "' for asset '" + asset + "' at timeframe '" + timeframe + "'");
                }
            }
        }
    }

    return ValidationResult::Success();
}

ValidationResult TestOrchestrator::ValidateTearsheets(
    const runtime::AssetReportMap& reports,
    const RuntimeValidation::OutputColumnValidation& validation)
{
    // For each asset report
    for (const auto& report_pair : reports)
    {
        const auto& asset = report_pair.first;
        // const auto& tearsheet = report_pair.second;

        // Tearsheets contain cards, charts, and tables
        // TODO: Implement detailed tearsheet column validation based on protobuf structure

        // For now, just check that reports map is not empty
        // This is a placeholder - actual validation will check card/chart/table columns
        (void)asset;  // Suppress unused variable warning
    }

    return ValidationResult::Success();
}

ValidationResult TestOrchestrator::ValidateEventMarkers(
    const runtime::AssetEventMarkerMap& event_markers,
    const RuntimeValidation::OutputColumnValidation& validation)
{
    // For each asset's event markers
    for (const auto& [asset, marker_list] : event_markers)
    {
        for (const auto& marker : marker_list)
        {
            // Each EventMarkerData has a DataFrame
            const auto& df = marker.data;

            // Check each expected column
            for (const auto& [column_name, rule] : validation.columns)
            {
                // Check if column exists
                auto col_names = df.column_names();
                if (std::find(col_names.begin(), col_names.end(), column_name) == col_names.end()) {
                    return ValidationResult::Failure(
                        "Event marker missing column '" + column_name +
                        "' for asset '" + asset + "'");
                }

                // Validate column against rule
                if (!ValidateColumn(df, column_name, rule)) {
                    std::string rule_name = (rule == ColumnValidation::at_least_one_valid)
                        ? "at_least_one_valid" : "all_nulls";
                    return ValidationResult::Failure(
                        "Event marker column '" + column_name +
                        "' failed validation '" + rule_name +
                        "' for asset '" + asset + "'");
                }
            }
        }
    }

    return ValidationResult::Success();
}

bool TestOrchestrator::ValidateColumn(
    const epoch_frame::DataFrame& df,
    const std::string& column_name,
    ColumnValidation rule)
{
    switch (rule) {
        case ColumnValidation::at_least_one_valid:
            return HasAtLeastOneValid(df, column_name);
        case ColumnValidation::all_nulls:
            return AllNulls(df, column_name);
        default:
            return false;
    }
}

bool TestOrchestrator::HasAtLeastOneValid(
    const epoch_frame::DataFrame& df,
    const std::string& column_name)
{
    // Get column size
    size_t row_count = df.num_rows();
    if (row_count == 0) {
        return false;
    }

    // Check if column has at least one non-null value
    // This is a simplified check - actual implementation depends on DataFrame API
    // TODO: Implement proper null checking based on epoch_frame::DataFrame API

    // For now, assume if column exists and has rows, it has valid data
    // You'll need to enhance this based on your DataFrame's null handling
    return row_count > 0;
}

bool TestOrchestrator::AllNulls(
    const epoch_frame::DataFrame& df,
    const std::string& column_name)
{
    // Check if all values in column are null
    // This is a placeholder - actual implementation depends on DataFrame API
    // TODO: Implement proper null checking based on epoch_frame::DataFrame API

    // For now, return false (not all nulls) if column has any rows
    return df.num_rows() == 0;
}

} // namespace epoch_script::test
