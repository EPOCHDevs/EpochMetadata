//
// Created by adesola on 7/20/24.
//

#include "exchange_calendar.h"

#include <epoch_data_sdk/common/glaze_custom_types.hpp>
#include <epoch_script/core/bar_attribute.h>

namespace epoch_script::calendar {
  epoch_frame::calendar::MarketCalendarPtr
  GetExchangeCalendarFromSpec(AssetSpecification const &asset) {
    auto const &factory = epoch_frame::calendar::CalendarFactory::instance();
    if (asset.GetAssetClass() == AssetClass::Futures) {
      switch (asset.GetExchange()) {
        case Exchange::ICEUS:
        case Exchange::ICESI:
          return factory.get_calendar("ICEUS");
        case epoch_core::Exchange::CME:
        case epoch_core::Exchange::CBOT:
        case epoch_core::Exchange::CBOTM:
        case epoch_core::Exchange::NYMEX:
        case epoch_core::Exchange::COMEX: {
          auto category = asset.GetCategory();
          if (category == epoch_script::FuturesConstants::Category::INDICES) {
            return factory.get_calendar("CME_Equity");
          }
          if (category == epoch_script::FuturesConstants::Category::FINANCIALS) {
            return factory.get_calendar("CME_Bond");
          }
          return factory.get_calendar("CME_Agriculture");
        }
        case Exchange::GBLX: {
          return factory.get_calendar("CME_Equity");
        }
        case Exchange::CBOE: {
          return factory.get_calendar("CBOE_Futures");
        }
        default: {
          throw std::runtime_error("Invalid Exchange for Futures:" +
                                   ExchangeWrapper::ToString(asset.GetExchange()));
        }
      }
    }

    switch (asset.GetExchange()) {
      case Exchange::NYSE:
      case Exchange::NASDAQ:
      case Exchange::AMEX:
        return factory.get_calendar("NYSE");
      case Exchange::COINBASE:
        return factory.get_calendar("Crypto");
      case Exchange::FX:
        return factory.get_calendar("FX");
      default: {
        break;
      }
    }

    SPDLOG_WARN("Failed to find exchange calendar for {}. Using NYSE.",
                    glz::prettify(asset.GetData()));
    return factory.get_calendar("NYSE");
  }

  epoch_frame::calendar::MarketCalendarPtr
  GetExchangeCalendar(Asset const &asset) {
    return GetExchangeCalendarFromSpec(asset.GetSpec());
  }

  std::unordered_set<epoch_frame::calendar::MarketCalendarPtr>
  ExtractExchangeCalendars(AssetHashSet const &assets) {
    if (std::ranges::any_of(assets, [](const Asset& asset) {
      return asset.GetAssetClass() == AssetClass::Crypto;
    })) {
      return {epoch_frame::calendar::CalendarFactory::instance().get_calendar("Crypto")};
    }
    if (std::ranges::any_of(assets, [](const Asset& asset) {
      return asset.GetAssetClass() == AssetClass::FX;
    })) {
      return {epoch_frame::calendar::CalendarFactory::instance().get_calendar("FX")};
    }

    auto isFutures = [](const Asset& asset) {
      return asset.GetAssetClass() == AssetClass::Futures;
    };

    // BANDAID FIX: For futures, use only the first calendar to avoid merge conflicts
    // TODO: Enhance calendar merge to support multiple futures exchanges properly
    if (std::ranges::any_of(assets, isFutures)) {
      auto firstFuture = std::ranges::find_if(assets, isFutures);
      return {GetExchangeCalendar(*firstFuture)};
    }

    // For stocks/options, transform all of them (they all use same NYSE calendar anyway)
    auto fn = [](auto&& assets) {
      std::unordered_set<epoch_frame::calendar::MarketCalendarPtr> result;
      std::ranges::transform(assets, std::inserter(result, result.end()),
                             GetExchangeCalendar);
      return result;
    };

    return fn(assets);
  }
} // namespace epoch_script::calendar
