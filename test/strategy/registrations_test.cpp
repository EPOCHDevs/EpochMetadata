//
// Created by adesola on 12/16/24.
//
#include <catch.hpp>
#include "epoch_metadata/strategy/registry.h"
#include "epoch_metadata/strategy/registration.h"
#include "../common.h"

TEST_CASE("Strategy Component MetaData Total Count is Correct") {
    using namespace epoch_metadata::strategy;
    RegisterStrategyMetadata(epoch_metadata::DEFAULT_YAML_LOADER, epoch_metadata::LoadAIGeneratedStrategies());

    REQUIRE(slippage::Registry::GetInstance().GetMetaData().size() == 3);
    REQUIRE(commission::Registry::GetInstance().GetMetaData().size() == 2);
    REQUIRE(futures_continuation::Registry::GetInstance().GetMetaData().size() == 3);
    REQUIRE(take_profit::Registry::GetInstance().GetMetaData().size() == 4);
    REQUIRE(stop_loss::Registry::GetInstance().GetMetaData().size() == 4);
    REQUIRE(position_sizer::Registry::GetInstance().GetMetaData().size() == 9);
    REQUIRE(trade_signal::Registry::GetInstance().GetMetaData().size() == 800);
    REQUIRE(strategy_templates::Registry::GetInstance().GetMetaData().size() == 792);
}
