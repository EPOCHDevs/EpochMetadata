//
// Created by adesola on 4/20/25.
//
#include <catch2/catch_all.hpp>
#include <common/epoch_thread_pool.h>
#include <data/database/updates/alpaca_websocket_manager.h>
#include <drogon/HttpAppFramework.h>
#include "data/model/common/constants.h"
#include "data/factory.h"


using namespace epoch_script::data;

namespace {
    struct TestObserver {
        void operator()(const BarList & bars) const {
            auto str = glz::write_json(bars).value();
            m_bars += (str + "\n");
            SPDLOG_INFO("Received BarList: {}", str);
        }
        mutable std::string m_bars;
    };

    void RunTest(const TestObserver& observer,
        AssetClass assetClass,
        data_sdk::asset::AssetHashSet const& assets1,
        data_sdk::asset::AssetHashSet const& assets2) {
        auto key = getenv("ALPACA_API_KEY");
        const auto secret = getenv("ALPACA_API_SECRET");

        if (key == nullptr || secret == nullptr) {
            SPDLOG_WARN("ALPACA_API_KEY or ALPACA_API_SECRET not set");
            return;
        }

        auto manager = WebSocketManagerSingleton::instance().GetWebSocketManager(assetClass);
        manager->Connect();

        manager->Subscribe(assets1);
        manager->HandleNewMessage(observer);

        drogon::app().getLoop()->runAfter(10, [=]() { manager->Subscribe(assets2); });

        if (auto all_bars = observer.m_bars; all_bars.empty()) {
            SPDLOG_WARN("No bars received");
        }
        else {
            SPDLOG_INFO("All bars: {}", all_bars);
        }
    }
}

TEST_CASE("Alpaca Websocket Test", "[.live]") {
    TestObserver observer1, observer2, observer3;

    RunTest(observer1,
        AssetClass::Stocks,
data_sdk::asset::AssetHashSet{
            epoch_script::EpochScriptAssetConstants::instance().AMZN,
            epoch_script::EpochScriptAssetConstants::instance().MSFT
        },
data_sdk::asset::AssetHashSet{
epoch_script::EpochScriptAssetConstants::instance().AAPL,
epoch_script::EpochScriptAssetConstants::instance().MSFT}
);

    RunTest(observer2,
        AssetClass::Crypto,
data_sdk::asset::AssetHashSet{
            epoch_script::EpochScriptAssetConstants::instance().BTC_USD,
            epoch_script::EpochScriptAssetConstants::instance().ETH_BTC
        },
data_sdk::asset::AssetHashSet{
epoch_script::EpochScriptAssetConstants::instance().ETH_USD,
});

    RunTest(observer3, AssetClass::Stocks,
data_sdk::asset::AssetHashSet{
        epoch_script::EpochScriptAssetConstants::instance().GOOG
    },
data_sdk::asset::AssetHashSet{
epoch_script::EpochScriptAssetConstants::instance().IBM}
);

    drogon::app().getLoop()->runAfter(60,[&] { drogon::app().quit(); });
    drogon::app().setLogLevel(trantor::Logger::kDebug);
    drogon::app().run();
}