//
// Created by adesola on 7/20/24.
//

#pragma once
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_frame/factory/calendar_factory.h>

namespace epoch_script::calendar {
    using data_sdk::asset::Asset;
    using data_sdk::asset::AssetSpecification;
    using data_sdk::asset::AssetHashSet;

    epoch_frame::calendar::MarketCalendarPtr GetExchangeCalendar(Asset const &);

    epoch_frame::calendar::MarketCalendarPtr GetExchangeCalendarFromSpec(AssetSpecification const &);

    std::unordered_set<epoch_frame::calendar::MarketCalendarPtr>
    ExtractExchangeCalendars(AssetHashSet const &);
} // namespace epoch_script::calendar
