#pragma once

#include <filesystem>
#include <string>
#include <epoch_script/transforms/runtime/types.h>
#include <epoch_protos/tearsheet.pb.h>
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::runtime::test {

namespace fs = std::filesystem;

/**
 * @brief Validates runtime execution outputs against expected results
 *
 * Validates all three types of outputs:
 * - Dataframes (output data from transforms)
 * - Tearsheets/Reports (generated reports)
 * - Event Markers (interactive UI markers)
 */
class RuntimeOutputValidator {
public:
    struct ValidationResult {
        bool passed = true;
        std::string message;

        ValidationResult() = default;
        ValidationResult(bool p, std::string msg) : passed(p), message(std::move(msg)) {}

        static ValidationResult Success() {
            return ValidationResult(true, "");
        }

        static ValidationResult Failure(std::string msg) {
            return ValidationResult(false, std::move(msg));
        }
    };

    /**
     * @brief Validate output dataframes against expected CSV files
     * @param actual_dataframes Actual output dataframes from pipeline execution
     * @param expected_dir Directory containing expected CSV files
     * @return Validation result
     */
    static ValidationResult ValidateDataframes(
        const TimeFrameAssetDataFrameMap& actual_dataframes,
        const fs::path& expected_dir);

    /**
     * @brief Validate generated reports/tearsheets against expected proto files
     * @param actual_reports Actual generated reports
     * @param expected_dir Directory containing expected proto files
     * @return Validation result
     */
    static ValidationResult ValidateTearsheets(
        const AssetReportMap& actual_reports,
        const fs::path& expected_dir);

    /**
     * @brief Validate generated event markers against expected JSON files
     * @param actual_event_markers Actual generated event markers
     * @param expected_dir Directory containing expected JSON files
     * @return Validation result
     */
    static ValidationResult ValidateEventMarkers(
        const AssetEventMarkerMap& actual_event_markers,
        const fs::path& expected_dir);
};

} // namespace epoch_script::runtime::test
