//
// Created by adesola on 8/13/24.
//
#include "first_of_month.h"
#include "last_trading_day.h"
#include "liquidity_based.h"
#include <epoch_script/data/model/exchange_calendar.h>
#include <epoch_frame/market_calendar.h>
#include <epoch_data_sdk/model/builder/asset_builder.hpp>

namespace epoch_script::futures {
namespace asset = data_sdk::asset;

epoch_frame::calendar::MarketCalendarPtr
GetCalendar(asset::ContractInfo const &contractInfo) {
  const auto contract = contractInfo.GetSymbol();  // Already returns std::string
  AssertFromStream(
      contract.size() >= 4,
      "invalid contract usually greater than 3 {symbol}{month-code}{Year}");
  const auto root = contract.substr(0, contract.size() - 3);
  const auto assetSpec = asset::MakeAssetSpec(
      {.required = std::pair{root,  // root is std::string, matches data_sdk::Symbol
                             epoch_core::AssetClass::Futures}});
  return calendar::GetExchangeCalendarFromSpec(assetSpec);
}

bool FirstOfMonthRollMethod::IsRollDate(const Input &input) const {
  using namespace epoch_frame;
  auto contractInfo = GetContract(input.frontData);
  epoch_frame::calendar::MarketCalendarPtr calendar = GetCalendar(contractInfo);

  auto currentTimestamp = Scalar{input.currentDate}.timestamp();
  const auto &baseOffset = calendar->holidays();

  auto expiry = contractInfo.GetExpirationDate();

  DateTime firstDate{Date{expiry.year, expiry.month, 1d}};

  auto expectedDateTime = baseOffset->rollforward(firstDate.timestamp());

  auto offset = GetOffset();
  if (offset != 0) {
    expectedDateTime = baseOffset->mul(offset)->add(expectedDateTime);
  }

  auto expectedDate = DateTime::fromtimestamp(expectedDateTime.value).date();
  return expectedDate == input.currentDate;
}

bool LastTradingDayMethod::IsRollDate(const Input &input) const {
  using namespace epoch_frame;

  auto contractInfo = GetContract(input.frontData);
  auto expirationDate = contractInfo.GetExpirationDate();
  epoch_frame::calendar::MarketCalendarPtr calendar = GetCalendar(contractInfo);

  auto currentTimestamp = Scalar{input.currentDate}.timestamp();
  const auto &baseOffset = calendar->holidays();
  auto expectedTimestamp = baseOffset->mul(GetOffset())->add(currentTimestamp);
  auto expectedDate = Scalar(expectedTimestamp).to_datetime().date();

  return expirationDate <= expectedDate;
}
} // namespace epoch_script::futures