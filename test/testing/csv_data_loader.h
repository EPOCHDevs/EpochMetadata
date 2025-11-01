#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "epoch_frame/dataframe.h"

namespace epoch_flow::runtime::test {

/**
 * @brief Utility for loading test data from CSV files
 *
 * CSV files are named with pattern: {timeframe}_{assetid}.csv
 * Examples:
 *   - 1D_AAPL-Stock.csv
 *   - 1Min_EURUSD-FX.csv
 *   - 15Min_ESH25-Futures.csv
 */
class CsvDataLoader {
public:
    using AssetDataFrameMap = std::unordered_map<std::string, epoch_frame::DataFrame>;
    using TimeFrameAssetDataFrameMap = std::unordered_map<std::string, AssetDataFrameMap>;

    struct FilenameParts {
        std::string timeframe;
        std::string assetId;
    };

    /**
     * @brief Load all CSV files from a directory into TimeFrameAssetDataFrameMap
     * @param inputDir Directory containing CSV files
     * @return Map of timeframe -> asset -> DataFrame
     */
    static TimeFrameAssetDataFrameMap LoadFromDirectory(const std::filesystem::path& inputDir);

    /**
     * @brief Load a single CSV file into a DataFrame
     * @param csvPath Path to CSV file
     * @return DataFrame with timestamp index
     */
    static epoch_frame::DataFrame LoadCsvFile(const std::filesystem::path& csvPath);

    /**
     * @brief Parse timeframe and asset from CSV filename
     * @param filename Filename (e.g., "1D_AAPL-Stocks.csv")
     * @return FilenameParts with timeframe and asset, or nullopt if invalid
     */
    static std::optional<FilenameParts> ParseFilename(const std::string& filename);

    /**
     * @brief Write DataFrame to CSV file
     * @param df DataFrame to write
     * @param csvPath Output file path
     * @param includeIndex Whether to include timestamp index column
     */
    static void WriteCsvFile(const epoch_frame::DataFrame& df,
                            const std::filesystem::path& csvPath,
                            bool includeIndex = true);
};

} // namespace epoch_flow::runtime::test
