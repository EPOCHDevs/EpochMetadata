//
// Unit test for Tulip indicator default values
// Tests that common technical indicators work with default parameter values
//

#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include "transforms/compiler/ast_compiler.h"
#include <algorithm>

using namespace epoch_script;

// Test ATR with default period (should be 14)
TEST_CASE("ATR with default period", "[tulip][defaults][atr]")
{
    std::string script = R"(
atr_result = atr(timeframe="1D")()
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the ATR node
    auto atr_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "atr";
    });

    REQUIRE(atr_node != result.end());

    // Verify that period option was set to default value of 14
    REQUIRE(atr_node->options.contains("period"));
    auto period_value = atr_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 14.0);
}

// Test RSI with default period (should be 14)
TEST_CASE("RSI with default period", "[tulip][defaults][rsi]")
{
    std::string script = R"(
src = market_data_source(timeframe="1D")
rsi_result = rsi()(src.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the RSI node
    auto rsi_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "rsi";
    });

    REQUIRE(rsi_node != result.end());

    // Verify that period option was set to default value of 14
    REQUIRE(rsi_node->options.contains("period"));
    auto period_value = rsi_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 14.0);
}

// Test SMA with default period (should be 14)
TEST_CASE("SMA with default period", "[tulip][defaults][sma]")
{
    std::string script = R"(
src = market_data_source(timeframe="1D")
sma_result = sma()(src.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the SMA node
    auto sma_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "sma";
    });

    REQUIRE(sma_node != result.end());

    // Verify that period option was set to default value of 14
    REQUIRE(sma_node->options.contains("period"));
    auto period_value = sma_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 14.0);
}

// Test EMA with default period (should be 14)
TEST_CASE("EMA with default period", "[tulip][defaults][ema]")
{
    std::string script = R"(
src = market_data_source(timeframe="1D")
ema_result = ema()(src.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the EMA node
    auto ema_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "ema";
    });

    REQUIRE(ema_node != result.end());

    // Verify that period option was set to default value of 14
    REQUIRE(ema_node->options.contains("period"));
    auto period_value = ema_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 14.0);
}

// Test Bollinger Bands with default period (14) and stddev (2)
TEST_CASE("Bollinger Bands with defaults", "[tulip][defaults][bbands]")
{
    std::string script = R"(
src = market_data_source(timeframe="1D")
bbands_result = bbands()(src.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the bbands node
    auto bbands_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "bbands";
    });

    REQUIRE(bbands_node != result.end());

    // Verify that period option was set to default value of 14
    REQUIRE(bbands_node->options.contains("period"));
    auto period_value = bbands_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 14.0);

    // Verify that stddev option was set to default value of 2
    REQUIRE(bbands_node->options.contains("stddev"));
    auto stddev_value = bbands_node->options.at("stddev").GetDecimal();
    REQUIRE(stddev_value == 2.0);
}

// Test MACD with default periods (short=12, long=26, signal=9)
TEST_CASE("MACD with defaults", "[tulip][defaults][macd]")
{
    std::string script = R"(
src = market_data_source(timeframe="1D")
macd_result = macd()(src.c)
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the MACD node
    auto macd_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "macd";
    });

    REQUIRE(macd_node != result.end());

    // Verify short_period = 12
    REQUIRE(macd_node->options.contains("short_period"));
    auto short_period = macd_node->options.at("short_period").GetDecimal();
    REQUIRE(short_period == 12.0);

    // Verify long_period = 26
    REQUIRE(macd_node->options.contains("long_period"));
    auto long_period = macd_node->options.at("long_period").GetDecimal();
    REQUIRE(long_period == 26.0);

    // Verify signal_period = 9
    REQUIRE(macd_node->options.contains("signal_period"));
    auto signal_period = macd_node->options.at("signal_period").GetDecimal();
    REQUIRE(signal_period == 9.0);
}

// Test Stochastic Oscillator with default periods (%k=14, %d=3)
TEST_CASE("Stochastic Oscillator with defaults", "[tulip][defaults][stoch]")
{
    std::string script = R"(
stoch_result = stoch(timeframe="1D")()
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the stoch node
    auto stoch_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "stoch";
    });

    REQUIRE(stoch_node != result.end());

    // Verify k_period = 14
    REQUIRE(stoch_node->options.contains("k_period"));
    auto k_value = stoch_node->options.at("k_period").GetDecimal();
    REQUIRE(k_value == 14.0);

    // Verify d_period = 3
    REQUIRE(stoch_node->options.contains("d_period"));
    auto d_value = stoch_node->options.at("d_period").GetDecimal();
    REQUIRE(d_value == 3.0);
}

// Test that explicit parameters override defaults
TEST_CASE("Explicit parameters override defaults", "[tulip][defaults][override]")
{
    std::string script = R"(
atr_custom = atr(timeframe="1D", period=20)()
)";

    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(script, /*skip_sink_validation=*/true);

    // Find the ATR node
    auto atr_node = std::ranges::find_if(result, [](const auto& node) {
        return node.type == "atr";
    });

    REQUIRE(atr_node != result.end());

    // Verify that period option was set to explicit value of 20
    REQUIRE(atr_node->options.contains("period"));
    auto period_value = atr_node->options.at("period").GetDecimal();
    REQUIRE(period_value == 20.0);
}
