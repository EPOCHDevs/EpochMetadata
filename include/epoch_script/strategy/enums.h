//
// Created by adesola on 6/10/25.
//

#pragma once
#include <epoch_core/enum_wrapper.h>

CREATE_ENUM(RolloverType, FirstOfMonth, LastTradingDay, LiquidityBased);
CREATE_ENUM(AlgorithmType, TakeProfit, StopLoss, Sizer, Commission, Slippage,
            FuturesContinuation);
CREATE_ENUM(TradeSignalType, TrendFollowing, MeanReverting, CandleStickPattern,
            Momentum, EventDriven, PriceAction, TechnicalPattern);
CREATE_ENUM(GenericFunctionType, TradeSignal, PositionSizer, TakeProfit,
            StopLoss, FuturesContinuation, Slippage, Commission);
CREATE_ENUM(AdjustmentType, BackwardPanamaCanal, BackwardRatio,
            ForwardPanamaCanal, ForwardRatio);
