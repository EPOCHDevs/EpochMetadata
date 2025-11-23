//
// Unit test for economic_indicator timeframe resolution bug
// Bug: Zero-input source nodes like economic_indicator() fail timeframe resolution
//

#include <catch2/catch_all.hpp>
#include "transforms/compiler/ast_compiler.h"

using namespace epoch_script;

TEST_CASE("Economic Indicator Timeframe Resolution", "[compiler][timeframe][regression]")
{
    SECTION("economic_indicator with attribute access should resolve timeframe")
    {
        std::string source = R"(
src = market_data_source(timeframe="1D")
fed_funds = economic_indicator(category="FedFunds")()
low_rates = fed_funds.value < 2.5
numeric_cards_report(agg="mean", category="Test", title="Low Rates")(low_rates)
)";

        AlgorithmAstCompiler compiler;
        
        // This should NOT throw - fed_funds should get timeframe from its dependent (lt_0)
        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);
            
            // Find fed_funds node
            auto fed_funds_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "fed_funds"; });
            
            REQUIRE(fed_funds_it != result.end());
            REQUIRE(fed_funds_it->timeframe.has_value());
            
            // Should inherit "1D" from the market_data_source base timeframe
            REQUIRE(fed_funds_it->timeframe->ToString() == "1D");
        }());
    }
    
    SECTION("indices() should also resolve timeframe")
    {
        std::string source = R"(
src = market_data_source(timeframe="1D")
vix = indices(ticker="VIX")()
low_fear = vix.c < 20
numeric_cards_report(agg="mean", category="Test", title="Low VIX")(low_fear)
)";

        AlgorithmAstCompiler compiler;
        
        REQUIRE_NOTHROW([&]() {
            auto result = compiler.compile(source);
            
            auto vix_it = std::find_if(result.begin(), result.end(),
                [](const auto& n) { return n.id == "vix"; });
            
            REQUIRE(vix_it != result.end());
            REQUIRE(vix_it->timeframe.has_value());
            REQUIRE(vix_it->timeframe->ToString() == "1D");
        }());
    }
}
