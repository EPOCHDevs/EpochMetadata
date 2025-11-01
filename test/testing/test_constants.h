#pragma once
#include <string>

namespace epochflow {
    // Forward declaration - TimeFrame is defined in EpochMetadata
    class TimeFrame;
}

namespace epoch_flow::runtime::test {

/**
 * @brief Test constants for AssetIDs
 *
 * Format: TICKER-AssetClass
 * For testing, simple string identifiers are sufficient
 */
struct TestAssetConstants {
    static constexpr const char* AAPL = "AAPL-Stock";
    static constexpr const char* MSFT = "MSFT-Stock";
    static constexpr const char* GOOG = "GOOG-Stock";
    static constexpr const char* GOOGL = "GOOGL-Stock";
    static constexpr const char* AMZN = "AMZN-Stock";
    static constexpr const char* TSLA = "TSLA-Stock";
    static constexpr const char* SPY = "SPY-ETF";
    static constexpr const char* QQQ = "QQQ-ETF";

    // Futures
    static constexpr const char* ES = "ES-Futures";
    static constexpr const char* NQ = "NQ-Futures";
    static constexpr const char* CL = "CL-Futures";
};

/**
 * @brief Helper to create common TimeFrame objects for testing
 *
 * Note: TimeFrame constructors and factory methods are defined in EpochMetadata.
 * Simply reuse whatever methods exist there for Daily, Hourly, Minute.
 */
struct TestTimeFrames {
    // These are just simple inline constructors
    // TimeFrame likely has string constructor or static factory methods
    static epochflow::TimeFrame Daily();
    static epochflow::TimeFrame Hourly();
    static epochflow::TimeFrame Minute();
};

} // namespace epoch_flow::runtime::test
