//
// Created by adesola on 12/16/24.
//
#include "../registration.h"
#include <catch.hpp>


TEST_CASE("Strategy Component MetaData Total Count is Correct")
{
    using namespace stratifyx::metadata::strategy;
    REQUIRE(slippage::Registry::GetInstance().GetMetaData().size() == 3);
    REQUIRE(commission::Registry::GetInstance().GetMetaData().size() == 2);
    REQUIRE(futures_continuation::Registry::GetInstance().GetMetaData().size() == 1);
    REQUIRE(take_profit::Registry::GetInstance().GetMetaData().size() == 5);
    REQUIRE(stop_loss::Registry::GetInstance().GetMetaData().size() == 5);
    REQUIRE(position_sizer::Registry::GetInstance().GetMetaData().size() == 10);
    REQUIRE(trade_signal::Registry::GetInstance().GetMetaData().size() == 8);
}
