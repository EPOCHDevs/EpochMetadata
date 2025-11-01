#include "catch.hpp"
#include <epoch_metadata/model/builder/order_builder.h"
#include "epoch_metadata/decimal_utils.h"

using namespace epoch_flow;
using namespace epoch_metadata;
using namespace epoch_flow::order;
using namespace epoch_flow::asset;

TEST_CASE("Test Valid Arguments", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  epoch_frame::DateTime now = epoch_frame::DateTime::now();

  epoch_flow::order::OrderBuilder orderBuilder(asset, 10_dec, now);

  SECTION("Invalid Qty") {
    REQUIRE_THROWS(epoch_flow::order::OrderBuilder(asset, 0, now));
    REQUIRE_THROWS(epoch_flow::order::OrderBuilder(
        asset, epoch_metadata::EpochStratifyXConstants::instance().nan, now));
    REQUIRE_THROWS(epoch_flow::order::OrderBuilder(
        asset,
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        now));
    REQUIRE_THROWS(epoch_flow::order::OrderBuilder(
        asset,
        epoch_metadata::EpochStratifyXConstants::instance().negativeInfinity,
        now));
  }

  SECTION("Invalid Limit Price/ Trail Price") {
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        0, 0, epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        0, epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        0, epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().nan, 0,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity, 0,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.TrailingStopLimitBuy(
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_core::TimeInForceType::DAY));
  }

  SECTION("Invalid Stop Price") {
    REQUIRE_THROWS(orderBuilder.StopBuy(0, epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.StopBuy(
        epoch_metadata::EpochStratifyXConstants::instance().nan,
        epoch_core::TimeInForceType::DAY));
    REQUIRE_THROWS(orderBuilder.StopBuy(
        epoch_metadata::EpochStratifyXConstants::instance().positiveInfinity,
        epoch_core::TimeInForceType::DAY));
  }
}

TEST_CASE("Building orders", "[order_builder]") {
  //////////////// INITIALIZATION
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  decimal::Decimal qty(10);
  epoch_frame::DateTime now = epoch_frame::DateTime::now();
  decimal::Decimal limit_price(100);
  decimal::Decimal stop_price(90);
  decimal::Decimal trail_price(5);
  const epoch_core::TimeInForceType tif = epoch_core::TimeInForceType::GTC;
  epoch_flow::order::OrderBuilder orderBuilder(asset, qty, now);
  //////////////////////////////////////////////////////////////

  std::optional<Order> order;
  //////////////// CHECK UNIQUE_TEST TO EACH ORDER TYPE
  SECTION("Test Market") {
    SECTION("Market Buy") {
      order = orderBuilder.MarketBuy();
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
    }

    SECTION("Market Sell") {
      order = orderBuilder.MarketSell();
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
    }
    REQUIRE(order->GetType() == epoch_core::OrderType::Market);
    REQUIRE(order->GetLimitPrice().isnan());
    REQUIRE(order->GetStopPrice().isnan());
    REQUIRE(order->GetTrailAmount().isnan());
    REQUIRE(order->GetHWM().isnan());
  }

  SECTION("Test Limit") {
    SECTION("Limit Buy") {
      order = orderBuilder.LimitBuy(limit_price, tif);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
    }

    SECTION("Limit Sell") {
      order = orderBuilder.LimitSell(limit_price, tif);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
    }

    REQUIRE(order->GetType() == epoch_core::OrderType::Limit);
    REQUIRE(order->GetLimitPrice() == limit_price);
    REQUIRE(order->GetStopPrice().isnan());
    REQUIRE(order->GetTrailAmount().isnan());
    REQUIRE(order->GetHWM().isnan());
  }

  SECTION("Test Stop") {
    SECTION("Stop Buy") {
      order = orderBuilder.StopSell(stop_price, tif);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
    }

    SECTION("Stop Sell") {
      order = orderBuilder.StopBuy(stop_price, tif);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
    }

    REQUIRE(order->GetType() == epoch_core::OrderType::Stop);
    REQUIRE(order->GetStopPrice() == stop_price);
    REQUIRE(order->GetLimitPrice().isnan());
    REQUIRE(order->GetTrailAmount().isnan());
    REQUIRE(order->GetHWM().isnan());
  }

  SECTION("Test StopLimit") {
    SECTION("StopLimit Buy") {
      order = orderBuilder.StopLimitBuy(stop_price, tif, limit_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
    }

    SECTION("StopLimit Sell") {
      order = orderBuilder.StopLimitSell(stop_price, tif, limit_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
    }

    REQUIRE(order->GetType() == epoch_core::OrderType::StopLimit);
    REQUIRE(order->GetStopPrice() == stop_price);
    REQUIRE(order->GetLimitPrice() == limit_price);
    REQUIRE(order->GetTrailAmount().isnan());
    REQUIRE(order->GetHWM().isnan());
  }

  SECTION("Test TrailingStop") {
    SECTION("TrailingStop Buy") {
      order = orderBuilder.TrailingStopBuy(trail_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
      REQUIRE(order->GetHWM() == "+Inf"_dec);
      REQUIRE(order->GetStopPrice() == "+Inf"_dec);
    }

    SECTION("TrailingStop Sell") {
      order = orderBuilder.TrailingStopSell(trail_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
      REQUIRE(order->GetHWM() == "-Inf"_dec);
      REQUIRE(order->GetStopPrice() == "-Inf"_dec);
    }

    REQUIRE(order->GetType() == epoch_core::OrderType::TrailingStop);
    REQUIRE(order->GetTrailAmount() == trail_price);
    REQUIRE(order->GetLimitPrice().isnan());
  }

  SECTION("Test TrailingStopLimit") {
    SECTION("TrailingStopLimit Buy") {
      order = orderBuilder.TrailingStopLimitBuy(trail_price, limit_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Buy);
      REQUIRE(order->GetStopPrice() == "+Inf"_dec);
      REQUIRE(order->GetHWM() == "+Inf"_dec);
    }

    SECTION("TrailingStopLimit Sell") {
      order = orderBuilder.TrailingStopLimitSell(trail_price, limit_price);
      REQUIRE_FALSE(order->GetID() == epoch_metadata::INVALID_ID);
      REQUIRE(order->GetSide() == epoch_core::OrderSide::Sell);
      REQUIRE(order->GetStopPrice() == "-Inf"_dec);
      REQUIRE(order->GetHWM() == "-Inf"_dec);
    }

    REQUIRE(order->GetType() == epoch_core::OrderType::TrailingStopLimit);
    REQUIRE(order->GetTrailAmount() == trail_price);
    REQUIRE(order->GetLimitPrice() == limit_price);
  }
  //////////////////////////////////////////////////////////////

  REQUIRE(order->GetAsset() == asset);
  REQUIRE(order->GetQty() == qty);
  REQUIRE(order->IsGTC());
  REQUIRE_FALSE(order->HasOCAGroup());
  REQUIRE(order->GetParentID() == epoch_metadata::INVALID_ID);
  REQUIRE_FALSE(order->GetGTD().has_value());
  REQUIRE(order->IsPreSubmitted());
  REQUIRE(order->IsActive());
  REQUIRE(order->GetFilledPrice().isnan());
  REQUIRE(order->GetFilledQty().isnan());
  REQUIRE(order->GetCommission().iszero());
  REQUIRE(order->GetUpdateTime() == now);
}

TEST_CASE("Test MakeProfitTaker", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  decimal::Decimal qty(10);
  epoch_frame::DateTime now = epoch_frame::DateTime::now();
  decimal::Decimal limit_price(100);
  decimal::Decimal profitTarget(150);

  OrderBuilder parentOrderBuilder(asset, qty, now);
  Order parentOrder = parentOrderBuilder.LimitBuy(
      limit_price, epoch_core::TimeInForceType::GTC);
  SECTION("Valid Profit Target") {
    auto profitTaker = OrderBuilder(asset, qty, now)
                           .MakeProfitTaker(parentOrderBuilder, profitTarget,
                                            parentOrder.GetTIF())
                           .Build();

    REQUIRE(profitTaker.IsActive());
    REQUIRE(profitTaker.GetSide() == epoch_core::OrderSide::Sell);
    REQUIRE(profitTaker.GetLimitPrice() == profitTarget);
    REQUIRE(profitTaker.GetTIF() == epoch_core::TimeInForceType::GTC);
  }

  SECTION("Invalid Profit Target") {
    REQUIRE_THROWS(
        OrderBuilder(asset, qty, now)
            .MakeProfitTaker(
                parentOrderBuilder,
                epoch_metadata::EpochStratifyXConstants::instance().nan,
                parentOrder.GetTIF()));
    REQUIRE_THROWS(
        OrderBuilder(asset, qty, now)
            .MakeProfitTaker(parentOrderBuilder,
                             epoch_metadata::EpochStratifyXConstants::instance()
                                 .negativeInfinity,
                             parentOrder.GetTIF()));
    REQUIRE_THROWS(
        OrderBuilder(asset, qty, now)
            .MakeProfitTaker(parentOrderBuilder,
                             epoch_metadata::EpochStratifyXConstants::instance()
                                 .positiveInfinity,
                             parentOrder.GetTIF()));
    REQUIRE_THROWS(
        OrderBuilder(asset, qty, now)
            .MakeProfitTaker(parentOrderBuilder, 0, parentOrder.GetTIF()));
    REQUIRE_THROWS(OrderBuilder(asset, qty, now)
                       .MakeProfitTaker(parentOrderBuilder, -profitTarget,
                                        parentOrder.GetTIF()));
  }
}

TEST_CASE("Test MakeStopLoss", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  decimal::Decimal qty(10);
  epoch_frame::DateTime now = epoch_frame::DateTime::now();
  decimal::Decimal limit_price(100);
  decimal::Decimal stopPrice(90);
  decimal::Decimal trailAmt(5);

  OrderBuilder parentOrderBuilder(asset, qty, now);
  Order parentOrder = parentOrderBuilder.LimitBuy(
      limit_price, epoch_core::TimeInForceType::GTC);
  SECTION("Stop Loss with Stop Price") {
    auto stopLoss =
        OrderBuilder(asset, qty, now)
            .MakeStopLoss(parentOrderBuilder, parentOrder.GetTIF(), stopPrice)
            .Build();
    REQUIRE(stopLoss.IsActive());
    REQUIRE(stopLoss.GetSide() == epoch_core::OrderSide::Sell);
    REQUIRE(stopLoss.GetType() == epoch_core::OrderType::Stop);
    REQUIRE(stopLoss.GetLimitPrice().isnan());
    REQUIRE(stopLoss.GetStopPrice() == stopPrice);
    REQUIRE(stopLoss.GetTIF() == epoch_core::TimeInForceType::GTC);
  }

  SECTION("Stop Loss with Stop and Limit Price") {
    auto stopLoss = OrderBuilder(asset, qty, now)
                        .MakeStopLoss(parentOrderBuilder, parentOrder.GetTIF(),
                                      stopPrice, limit_price + 1)
                        .Build();
    REQUIRE(stopLoss.IsActive());
    REQUIRE(stopLoss.GetSide() == epoch_core::OrderSide::Sell);
    REQUIRE(stopLoss.GetType() == epoch_core::OrderType::StopLimit);
    REQUIRE(stopLoss.GetLimitPrice() == limit_price + 1);
    REQUIRE(stopLoss.GetStopPrice() == stopPrice);
    REQUIRE(stopLoss.GetTIF() == epoch_core::TimeInForceType::GTC);
  }

  SECTION("Stop Loss with Trailing Stop") {
    auto stopLoss =
        OrderBuilder(asset, qty, now)
            .MakeStopLoss(
                parentOrderBuilder, parentOrder.GetTIF(),
                epoch_metadata::EpochStratifyXConstants::instance().nan,
                epoch_metadata::EpochStratifyXConstants::instance().nan,
                trailAmt)
            .Build();
    REQUIRE(stopLoss.IsActive());
    REQUIRE(stopLoss.GetSide() == epoch_core::OrderSide::Sell);
    REQUIRE(stopLoss.GetType() == epoch_core::OrderType::TrailingStop);
    REQUIRE(stopLoss.GetLimitPrice().isnan());
    REQUIRE(stopLoss.GetStopPrice().isnan());
    REQUIRE(stopLoss.GetTrailAmount() == trailAmt);
    REQUIRE(stopLoss.GetTIF() == epoch_core::TimeInForceType::GTC);
  }

  SECTION("Stop Loss with Trailing Stop Limit") {
    auto stopLoss =
        OrderBuilder(asset, qty, now)
            .MakeStopLoss(
                parentOrderBuilder, parentOrder.GetTIF(),
                epoch_metadata::EpochStratifyXConstants::instance().nan,
                limit_price + 1, trailAmt)
            .Build();
    REQUIRE(stopLoss.IsActive());
    REQUIRE(stopLoss.GetSide() == epoch_core::OrderSide::Sell);
    REQUIRE(stopLoss.GetType() == epoch_core::OrderType::TrailingStopLimit);
    REQUIRE(stopLoss.GetLimitPrice() == limit_price + 1);
    REQUIRE_FALSE(stopLoss.GetLimitPrice() == limit_price);
    REQUIRE(stopLoss.GetStopPrice().isnan());
    REQUIRE(stopLoss.GetTrailAmount() == trailAmt);
    REQUIRE(stopLoss.GetTIF() == epoch_core::TimeInForceType::GTC);
  }

  SECTION("Invalid Stop Loss") {
    auto order = OrderBuilder(asset, qty, now)
                     .LimitBuy(limit_price, epoch_core::TimeInForceType::GTC);
    REQUIRE_THROWS(OrderBuilder(asset, qty, now)
                       .MakeStopLoss(parentOrderBuilder, parentOrder.GetTIF()));
    REQUIRE_THROWS(OrderBuilder(asset, qty, now)
                       .MakeStopLoss(parentOrderBuilder, parentOrder.GetTIF(),
                                     -stopPrice));
    REQUIRE_THROWS(OrderBuilder(asset, qty, now)
                       .MakeStopLoss(parentOrderBuilder, parentOrder.GetTIF(),
                                     stopPrice, limit_price, trailAmt));
  }
}

TEST_CASE("Test MakeOneTriggerAll", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  const decimal::Decimal qty(10);

  const decimal::Decimal limit_price(100);

  OrderBuilder parentOrderBuilder(asset, qty, epoch_frame::DateTime::now());
  Order parent = parentOrderBuilder.LimitBuy(limit_price,
                                             epoch_core::TimeInForceType::GTC);
  REQUIRE(parent.IsActive());

  std::vector children{parentOrderBuilder.Clone().MakeProfitTaker(
      parentOrderBuilder, 10, parent.GetTIF())};
  SECTION("Trigger Single Order") {
    REQUIRE_NOTHROW(OrderBuilder::MakeOneTriggerAll(parent.GetID(), children));
    const Order takeProfit = children.at(0).Build();

    REQUIRE(takeProfit.GetParentID() == parent.GetID());
    REQUIRE_FALSE(takeProfit.HasOCAGroup());
    REQUIRE_FALSE(takeProfit.IsActive());
  }

  SECTION("Trigger Multiple Orders") {
    children.emplace_back(parentOrderBuilder.Clone().MakeStopLoss(
        parentOrderBuilder, parent.GetTIF(), 20));
    REQUIRE_NOTHROW(OrderBuilder::MakeOneTriggerAll(parent.GetID(), children));
    const Order takeProfit = children.at(0).Build();
    const Order stopLoss = children.at(1).Build();

    REQUIRE(takeProfit.GetParentID() == parent.GetID());
    REQUIRE_FALSE(takeProfit.HasOCAGroup());
    REQUIRE_FALSE(takeProfit.IsActive());

    REQUIRE(stopLoss.GetParentID() == parent.GetID());
    REQUIRE_FALSE(stopLoss.HasOCAGroup());
    REQUIRE_FALSE(stopLoss.IsActive());
  }
}

TEST_CASE("Test MakeOneCancelAll", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  decimal::Decimal qty(10);

  decimal::Decimal limit_price(100);

  OrderBuilder parentOrderBuilder(asset, qty, epoch_frame::DateTime::now());
  auto parent = parentOrderBuilder.LimitBuy(limit_price,
                                            epoch_core::TimeInForceType::GTC);
  REQUIRE(parent.IsActive());

  std::vector children{parentOrderBuilder.Clone().MakeProfitTaker(
      parentOrderBuilder, 10, parent.GetTIF())};
  SECTION("Single Order Cancels Single Order") {
    REQUIRE_NOTHROW(OrderBuilder::MakeOneCancelAll(children));

    const Order takeProfit = children.at(0).Build();
    REQUIRE(takeProfit.GetParentID() == epoch_metadata::INVALID_ID);
    REQUIRE(takeProfit.HasOCAGroup());
    REQUIRE(takeProfit.IsActive());
  }

  SECTION("Single Order Cancels Multiple Order") {
    children.emplace_back(parentOrderBuilder.Clone().MakeStopLoss(
        parentOrderBuilder, parent.GetTIF(), 20));
    REQUIRE_NOTHROW(OrderBuilder::MakeOneCancelAll(children));
    const Order takeProfit = children.at(0).Build();
    const Order stopLoss = children.at(1).Build();

    REQUIRE(takeProfit.GetParentID() == epoch_metadata::INVALID_ID);
    REQUIRE_FALSE(takeProfit.GetOCAGroup() == epoch_metadata::INVALID_ID);
    REQUIRE(takeProfit.IsActive());

    REQUIRE(stopLoss.GetParentID() == epoch_metadata::INVALID_ID);
    REQUIRE_FALSE(stopLoss.GetOCAGroup() == epoch_metadata::INVALID_ID);
    REQUIRE(stopLoss.IsActive());
  }
}

TEST_CASE("Test Make Bracket Order", "[order_builder]") {
  asset::Asset asset = EpochStratifyXAssetConstants::instance().AAPL;
  decimal::Decimal qty(10);

  decimal::Decimal limit_price(100);

  OrderBuilder parentOrderBuilder(asset, qty, epoch_frame::DateTime::now());
  auto parent = parentOrderBuilder.LimitBuy(limit_price,
                                            epoch_core::TimeInForceType::GTC);
  REQUIRE(parent.IsActive());

  BracketOrder order = parentOrderBuilder.Clone().MakeBracketOrder(
      parentOrderBuilder, parent.GetTIF(), 5, parent.GetTIF(), 12);
  REQUIRE(order.takeProfit);
  REQUIRE(order.stopLoss);

  REQUIRE(order.takeProfit->GetParentID() == parent.GetID());
  REQUIRE_FALSE(order.takeProfit->GetOCAGroup() == epoch_metadata::INVALID_ID);
  REQUIRE(order.takeProfit->GetType() == epoch_core::OrderType::Limit);
  REQUIRE(order.takeProfit->GetLimitPrice() == 5);
  REQUIRE(order.takeProfit->GetSide() == epoch_core::OrderSide::Sell);
  REQUIRE_FALSE(order.takeProfit->IsActive());

  REQUIRE(order.stopLoss->GetParentID() == parent.GetID());
  REQUIRE_FALSE(order.stopLoss->GetOCAGroup() == epoch_metadata::INVALID_ID);
  REQUIRE(order.stopLoss->GetType() == epoch_core::OrderType::Stop);
  REQUIRE(order.stopLoss->GetStopPrice() == 12);
  REQUIRE(order.stopLoss->GetSide() == epoch_core::OrderSide::Sell);
  REQUIRE_FALSE(order.stopLoss->IsActive());
}