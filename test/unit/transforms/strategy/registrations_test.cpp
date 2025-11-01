//
// Created by adesola on 12/16/24.
//
#include "../common.h"
#include "epochflow/strategy/registration.h"
#include "epochflow/strategy/registry.h"
#include <catch.hpp>

TEST_CASE("Strategy Component MetaData Total Count is Correct")
{
        using namespace epochflow::strategy;

        REQUIRE(slippage::Registry::GetInstance().GetMetaData().size() == 3);
        REQUIRE(commission::Registry::GetInstance().GetMetaData().size() == 2);
        REQUIRE(futures_continuation::Registry::GetInstance().GetMetaData().size() ==
                3);
        REQUIRE(take_profit::Registry::GetInstance().GetMetaData().size() == 4);
        REQUIRE(stop_loss::Registry::GetInstance().GetMetaData().size() == 4);
        REQUIRE(position_sizer::Registry::GetInstance().GetMetaData().size() == 4);
        REQUIRE(trade_signal::Registry::GetInstance().GetMetaData().size() == 0);
        REQUIRE(strategy_templates::Registry::GetInstance().GetMetaData().size() ==
                0);
}
