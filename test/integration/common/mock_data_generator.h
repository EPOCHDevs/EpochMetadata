#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <chrono>
#include <random>

#include "epoch_frame/dataframe.h"

namespace epoch_script::runtime::test {

/**
 * @brief Deterministic mock data generator for integration tests
 *
 * Generates realistic data for all EpochScript data sources:
 * - Market Data: OHLCV (Open, High, Low, Close, Volume, VWAP, Number of trades)
 * - FRED Data: Economic indicators (CPI, GDP, Fed Funds, etc.)
 * - Polygon Fundamentals: Balance sheets, income statements
 * - SEC Data: Form 13F holdings, insider trading
 *
 * Uses seeded random number generation to ensure reproducibility.
 *
 * Key features:
 * - Deterministic: same seed → same data
 * - Realistic patterns: trends, volatility, gaps
 * - Multi-asset support: stocks, crypto, forex, futures
 * - Multi-data source: market, economic, fundamental, institutional
 */
class MockDataGenerator {
public:
    enum class AssetClass {
        Stock,
        Crypto,
        Forex,
        Futures
    };

    enum class DataSourceType {
        MarketData,        // OHLCV price data
        FRED,              // Economic indicators
        BalanceSheet,      // Polygon balance sheet fundamentals
        IncomeStatement,   // Polygon income statement fundamentals
        Form13F,           // SEC institutional holdings
        InsiderTrading,    // SEC insider transactions
        MarketIndices      // Market index data (SPY, QQQ, etc.)
    };

    enum class MarketRegime {
        Trending,      // Persistent directional movement
        Ranging,       // Sideways movement
        Volatile,      // High volatility, no clear direction
        Mixed          // Combination of regimes
    };

    struct GenerationConfig {
        std::string ticker;
        AssetClass asset_class = AssetClass::Stock;
        DataSourceType data_source = DataSourceType::MarketData;
        std::string timeframe;      // e.g., "1D", "1H", "15m"
        size_t num_bars = 100;
        std::string start_date;     // ISO 8601 date (e.g., "2024-01-01")
        MarketRegime regime = MarketRegime::Mixed;
        uint64_t seed = 0;          // Use 0 for auto-seed from ticker+timeframe

        // Market Data parameters
        double initial_price = 100.0;
        double volatility = 0.02;   // Daily volatility (2%)
        double trend_strength = 0.0; // -1.0 to 1.0 (negative = downtrend)
        size_t base_volume = 1000000; // Base daily volume
        double volume_volatility = 0.3; // Volume variation

        // FRED parameters
        std::string indicator_name = "CPI";  // For FRED data
        double indicator_base_value = 100.0;  // Base indicator value

        // Fundamental data parameters
        double revenue_base = 10000000000.0;  // $10B revenue
        double assets_base = 50000000000.0;   // $50B assets
        double eps_base = 5.0;                // $5 EPS

        // SEC data parameters
        size_t num_institutions = 10;         // Number of institutions for 13F
        size_t num_insiders = 5;              // Number of insiders for trading
    };

    /**
     * @brief Generate data according to configuration (dispatches to appropriate generator)
     * @param config Generation parameters
     * @return DataFrame with appropriate columns for the data source type
     */
    static epoch_frame::DataFrame GenerateData(const GenerationConfig& config);

    /**
     * @brief Generate OHLCV market data
     * @param config Generation parameters
     * @return DataFrame with columns: index (timestamp), o, h, l, c, vw, n
     */
    static epoch_frame::DataFrame GenerateMarketData(const GenerationConfig& config);

    /**
     * @brief Generate FRED economic indicator data
     * @param config Generation parameters
     * @return DataFrame with columns: index (observation_date), value
     */
    static epoch_frame::DataFrame GenerateFREDData(const GenerationConfig& config);

    /**
     * @brief Generate Polygon balance sheet fundamental data
     * @param config Generation parameters
     * @return DataFrame with balance sheet columns (quarterly data)
     */
    static epoch_frame::DataFrame GenerateBalanceSheetData(const GenerationConfig& config);

    /**
     * @brief Generate Polygon income statement fundamental data
     * @param config Generation parameters
     * @return DataFrame with income statement columns (quarterly data)
     */
    static epoch_frame::DataFrame GenerateIncomeStatementData(const GenerationConfig& config);

    /**
     * @brief Generate SEC Form 13F institutional holdings data
     * @param config Generation parameters
     * @return DataFrame with 13F columns (quarterly reporting)
     */
    static epoch_frame::DataFrame GenerateForm13FData(const GenerationConfig& config);

    /**
     * @brief Generate SEC insider trading data
     * @param config Generation parameters
     * @return DataFrame with insider trading transaction columns
     */
    static epoch_frame::DataFrame GenerateInsiderTradingData(const GenerationConfig& config);

    /**
     * @brief Resample daily data to intraday timeframe
     * @param daily_data DataFrame with daily OHLCV data
     * @param target_timeframe Target timeframe (e.g., "1H", "15m")
     * @param seed Seed for deterministic resampling
     * @return DataFrame with intraday data
     */
    static epoch_frame::DataFrame ResampleToIntraday(
        const epoch_frame::DataFrame& daily_data,
        const std::string& target_timeframe,
        uint64_t seed
    );

    /**
     * @brief Parse timeframe string to minutes
     * @param timeframe Timeframe string (e.g., "1D", "1H", "15m")
     * @return Number of minutes in timeframe, or nullopt if invalid
     */
    static std::optional<int> ParseTimeframeMinutes(const std::string& timeframe);

    /**
     * @brief Generate CSV filename from config
     * @param config Generation config
     * @return Filename like "1D_AAPL-Stock.csv"
     */
    static std::string GenerateFilename(const GenerationConfig& config);

    /**
     * @brief Write generated data to CSV file
     * @param data DataFrame to write
     * @param output_path Path to write CSV
     */
    static void WriteToCSV(const epoch_frame::DataFrame& data,
                          const std::filesystem::path& output_path);

private:
    // Generate deterministic seed from string
    static uint64_t GenerateSeed(const std::string& input);

    // Generate single OHLCV bar
    struct OHLCVBar {
        std::chrono::system_clock::time_point timestamp;
        double open;
        double high;
        double low;
        double close;
        double vwap;
        size_t volume;
    };

    // Generate bar sequence with realistic patterns
    static std::vector<OHLCVBar> GenerateBars(const GenerationConfig& config);

    // Apply market regime patterns
    static void ApplyRegimePattern(std::vector<OHLCVBar>& bars,
                                  MarketRegime regime,
                                  std::mt19937_64& rng);

    // Add realistic gaps (for daily+ timeframes)
    static void AddGaps(std::vector<OHLCVBar>& bars,
                       const std::string& timeframe,
                       std::mt19937_64& rng);

    // Increment timestamp by timeframe
    static std::chrono::system_clock::time_point IncrementTimestamp(
        std::chrono::system_clock::time_point current,
        const std::string& timeframe,
        AssetClass asset_class
    );

    // Get asset class string
    static std::string AssetClassToString(AssetClass ac);
};

} // namespace epoch_script::runtime::test