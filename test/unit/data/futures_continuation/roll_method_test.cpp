//
// Created by dewe on 12/5/23.
//
#include "catch.hpp"
#include "data/futures_continuation/roll_method/first_of_month.h"
#include "data/futures_continuation/roll_method/last_trading_day.h"
#include "data/futures_continuation/roll_method/liquidity_based.h"
#include <epoch_frame/datetime.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/index.h>

using namespace epoch_script;
using namespace epoch_script::futures;
using namespace epoch_frame;

DataFrame MakeDataFromContract(Date const &t, std::string const &symbol,
                               const double oi = 10) {
  return make_dataframe(
      factory::index::make_datetime_index({DateTime{t}}),
      std::vector{std::vector{Scalar{symbol}}, {Scalar{oi}}},
      {arrow::field(
           epoch_script::EpochStratifyXConstants::instance().CONTRACT(),
           arrow::utf8()),
       arrow::field(
           epoch_script::EpochStratifyXConstants::instance().OPEN_INTEREST(),
           arrow::float64())});
}

TEST_CASE("FirstOfMonthRollMethod Initialization", "[RolloverMethod]") {
  FirstOfMonthRollMethod method(5); // Assuming an 'offset' parameter
  REQUIRE(method.GetOffset() == 5);
}

TEST_CASE("FirstOfMonthRollMethod On Rollover Date", "[RolloverMethod]") {
  const FirstOfMonthRollMethod method(0); // No offset
  Date currentTime = "2023-12-01"__date.date();
  REQUIRE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));

  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESX23"),
       .currentDate = currentTime}));

  const FirstOfMonthRollMethod firstOfMonthRollMethodWithLead(3);
  currentTime = "2023-12-06"__date.date();
  REQUIRE(firstOfMonthRollMethodWithLead.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));

  const FirstOfMonthRollMethod firstOfMonthRollMethodWithLag(-3);
  currentTime = "2023-11-28"__date.date();
  REQUIRE(firstOfMonthRollMethodWithLag.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));
}

TEST_CASE("FirstOfMonthRollMethod Near Rollover Date", "[RolloverMethod]") {
  FirstOfMonthRollMethod method(0); // No offset
  Date currentTime = "2023-12-06"__date.date();
  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));

  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESF24"),
       .currentDate = currentTime}));
}

TEST_CASE("LastTradingDayMethod Initialization", "[RolloverMethod]") {
  LastTradingDayMethod method(5); // Assuming an 'offset' parameter
  REQUIRE(method.GetOffset() == 5);
}

TEST_CASE("LastTradingDayMethod On Rollover Date", "[RolloverMethod]") {
  const LastTradingDayMethod method(0); // No offset
  Date currentTime = GetContractExpiration("ESZ23");
  REQUIRE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));

  const LastTradingDayMethod methodWithOffset(3);
  currentTime = GetContractExpiration("ESZ23");
  REQUIRE(methodWithOffset.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));
}

TEST_CASE("LastTradingDayMethod Near Rollover Date", "[RolloverMethod]") {
  LastTradingDayMethod method(0); // No offset
  Date ESZ23ExpiryDate = GetContractExpiration("ESZ23");

  Date currentTime = ESZ23ExpiryDate - days(3);
  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23"),
       .currentDate = currentTime}));

  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESF24"),
       .currentDate = currentTime}));
}

TEST_CASE("LiquidityBasedMethod Initialization", "[RolloverMethod]") {
  LiquidityBasedMethod method(5); // Assuming an 'offset' parameter
  REQUIRE(method.GetOffset() == 5);
}

TEST_CASE("LiquidityBasedMethod On Rollover Date", "[RolloverMethod]") {
  LiquidityBasedMethod method(10); // Assuming a liquidity threshold parameter
  Date currentTime = GetContractExpiration("ESZ23");
  // Assuming liquidity is higher for back contract than front contract
  REQUIRE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23", 8),
       .backData = MakeDataFromContract(currentTime, "ESF24", 12),
       .currentDate = currentTime}));
}

TEST_CASE("LiquidityBasedMethod Not On Rollover Date", "[RolloverMethod]") {
  LiquidityBasedMethod method(10);
  Date currentTime = GetContractExpiration("ESZ23") - days(1);
  // Assuming liquidity is lower for back contract than front contract
  REQUIRE_FALSE(method.IsRollDate(
      {.frontData = MakeDataFromContract(currentTime, "ESZ23", 12),
       .backData = MakeDataFromContract(currentTime, "ESF24", 8),
       .currentDate = currentTime}));
}