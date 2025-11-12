#pragma once
//
// Created by dewe on 12/4/23.
//
#include "roll_method_base.h"

namespace epoch_script::futures {
class LiquidityBasedMethod : public RolloverMethodBase {
public:
  LiquidityBasedMethod(int offset)
      : RolloverMethodBase(offset),
        m_liquidityRatio(1 + (GetOffset() / 100.0)) {}

  inline bool IsRollDate(const Input &input) const final {
    const double currentOI = GetOpenInterestsRatio(input);
    return currentOI >= m_liquidityRatio;
  }

  inline epoch_core::RolloverType GetType() const final {
    return epoch_core::RolloverType::LiquidityBased;
  }

private:
  const double m_liquidityRatio;
  static double GetOpenInterest(const epoch_frame::DataFrame &data) {
    return data
        .iloc(
            0,
            epoch_script::EpochStratifyXConstants::instance().OPEN_INTEREST())
        .as_double();
  }

  static double GetOpenInterestsRatio(const Input &input) {
    return GetOpenInterest(input.backData) / GetOpenInterest(input.frontData);
  }
};
} // namespace epoch_script::futures