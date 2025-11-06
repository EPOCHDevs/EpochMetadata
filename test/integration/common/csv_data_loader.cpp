#include "csv_data_loader.h"

#include <fstream>
#include <regex>
#include <spdlog/spdlog.h>

#include "epoch_frame/serialization.h"

namespace epoch_script::runtime::test {

CsvDataLoader::TimeFrameAssetDataFrameMap
CsvDataLoader::LoadFromDirectory(const std::filesystem::path& inputDir) {
    TimeFrameAssetDataFrameMap result;

    if (!std::filesystem::exists(inputDir)) {
        SPDLOG_WARN("Input directory does not exist: {}", inputDir.string());
        return result;
    }

    // Iterate over all CSV files in directory
    for (const auto& entry : std::filesystem::directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".csv") continue;

        const std::string filename = entry.path().filename().string();
        auto parts = ParseFilename(filename);

        if (!parts) {
            SPDLOG_WARN("Skipping file with invalid name format: {}", filename);
            continue;
        }

        try {
            auto df = LoadCsvFile(entry.path());
            SPDLOG_DEBUG("Loaded {}: {} rows, {} columns",
                        filename, df.num_rows(), df.num_cols());
            result[parts->timeframe][parts->assetId] = std::move(df);
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to load {}: {}", filename, e.what());
        }
    }

    return result;
}

epoch_frame::DataFrame CsvDataLoader::LoadCsvFile(const std::filesystem::path& csvPath) {
    // Read CSV using epoch_frame utilities
    // Don't specify index_column - let epoch_frame auto-detect or use first column
    epoch_frame::CSVReadOptions options;

    auto result = epoch_frame::read_csv_file(csvPath.string(), options);

    if (!result.ok()) {
        throw std::runtime_error("Failed to read CSV: " + result.status().ToString());
    }

    auto df = result.ValueOrDie();

    // Set the first column as index if it exists and is named "index"
    if (df.num_cols() > 0 && df.column_names()[0] == "index") {
        df = df.set_index("index");
    }

    return df;
}

std::optional<CsvDataLoader::FilenameParts>
CsvDataLoader::ParseFilename(const std::string& filename) {
    // Pattern: {timeframe}_{assetid}.csv
    // Examples:
    //   1D_AAPL-Stock.csv
    //   1Min_EURUSD-FX.csv
    //   15Min_ESH25-Futures.csv

    std::regex pattern(R"(^(\w+)_(.+)\.csv$)");
    std::smatch matches;

    if (!std::regex_match(filename, matches, pattern)) {
        return std::nullopt;
    }

    std::string timeframe = matches[1].str();
    std::string assetId = matches[2].str();

    return FilenameParts{timeframe, assetId};
}

void CsvDataLoader::WriteCsvFile(const epoch_frame::DataFrame& df,
                                const std::filesystem::path& csvPath,
                                bool includeIndex) {
    // Ensure parent directory exists
    std::filesystem::create_directories(csvPath.parent_path());

    // Optionally exclude an index-like column named "index"
    epoch_frame::DataFrame toWrite = df;
    if (!includeIndex) {
        auto cols = toWrite.column_names();
        if (!cols.empty() && cols.front() == std::string("index")) {
            toWrite = toWrite.drop(std::string("index"));
        }
    }

    // Write CSV using epoch_frame utilities
    auto status = epoch_frame::write_csv_file(toWrite, csvPath.string());

    if (!status.ok()) {
        throw std::runtime_error("Failed to write CSV: " + status.ToString());
    }

    SPDLOG_DEBUG("Wrote DataFrame to {}: {} rows, {} columns",
                csvPath.string(), df.num_rows(), df.num_cols());
}

} // namespace epoch_script::runtime::test
