#pragma once

#include <set>
#include <string>
#include <epoch_script/transforms/runtime/types.h>
#include <epoch_protos/tearsheet.pb.h>
#include "json_test_case.h"

namespace epoch_script::test {

/**
 * Validation result for runtime output checks.
 */
struct ValidationResult {
    bool passed;
    std::string message;

    static ValidationResult Success() {
        return {true, ""};
    }

    static ValidationResult Failure(const std::string& msg) {
        return {false, msg};
    }
};

/**
 * Test orchestrator for integration test runtime execution and validation.
 *
 * Responsibilities:
 * 1. Provide test input data (AAPL, DJI, SP500 via dataloader factory)
 * 2. Validate runtime outputs against expected column validation rules
 * 3. Support column-level validation (at_least_one_valid, all_nulls)
 */
class TestOrchestrator {
public:
    /**
     * Provide test input data for given timeframe.
     *
     * Uses dataloader factory to load historical data for:
     * - AAPL (stock)
     * - DJI (index)
     * - SP500 (index)
     *
     * @param timeframe Timeframe string (e.g., "1D", "1H")
     * @return TimeFrameAssetDataFrameMap with test data
     */
    static runtime::TimeFrameAssetDataFrameMap ProvideTestData(
        const std::string& timeframe);

    /**
     * Get default test assets.
     *
     * @return Set of asset identifiers: {"AAPL-Stock", "DJI-Index", "SP500-Index"}
     */
    static std::set<std::string> GetDefaultTestAssets();

    /**
     * Validate executor outputs (dataframes) against expected column rules.
     *
     * Checks each column in the validation spec exists in output dataframes
     * and satisfies the validation rule (at_least_one_valid or all_nulls).
     *
     * @param outputs TimeFrameAssetDataFrameMap from orchestrator execution
     * @param validation Expected column validation rules
     * @return ValidationResult with pass/fail and detailed message
     */
    static ValidationResult ValidateExecutorOutputs(
        const runtime::TimeFrameAssetDataFrameMap& outputs,
        const RuntimeValidation::OutputColumnValidation& validation);

    /**
     * Validate tearsheets (reports) against expected column rules.
     *
     * Tearsheets are protobuf messages (epoch_proto::TearSheet) containing
     * cards, charts, and tables. This validates the data columns in those
     * nested structures.
     *
     * @param reports AssetReportMap from orchestrator->GetGeneratedReports()
     * @param validation Expected column validation rules
     * @return ValidationResult with pass/fail and detailed message
     */
    static ValidationResult ValidateTearsheets(
        const runtime::AssetReportMap& reports,
        const RuntimeValidation::OutputColumnValidation& validation);

    /**
     * Validate event markers against expected column rules.
     *
     * Event markers contain EventMarkerData structures with dataframes.
     * This validates columns in those dataframes.
     *
     * @param event_markers AssetEventMarkerMap from orchestrator->GetGeneratedEventMarkers()
     * @param validation Expected column validation rules
     * @return ValidationResult with pass/fail and detailed message
     */
    static ValidationResult ValidateEventMarkers(
        const runtime::AssetEventMarkerMap& event_markers,
        const RuntimeValidation::OutputColumnValidation& validation);

private:
    /**
     * Check if a column satisfies the validation rule.
     *
     * @param df DataFrame containing the column
     * @param column_name Column name to check
     * @param rule Validation rule to apply
     * @return true if validation passes
     */
    static bool ValidateColumn(
        const epoch_frame::DataFrame& df,
        const std::string& column_name,
        ColumnValidation rule);

    /**
     * Check if column has at least one valid (non-null) value.
     *
     * @param df DataFrame containing the column
     * @param column_name Column name to check
     * @return true if at least one valid value exists
     */
    static bool HasAtLeastOneValid(
        const epoch_frame::DataFrame& df,
        const std::string& column_name);

    /**
     * Check if all values in column are null.
     *
     * @param df DataFrame containing the column
     * @param column_name Column name to check
     * @return true if all values are null
     */
    static bool AllNulls(
        const epoch_frame::DataFrame& df,
        const std::string& column_name);
};

} // namespace epoch_script::test
