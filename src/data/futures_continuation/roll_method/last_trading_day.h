#pragma once
//
// Created by dewe on 12/4/23.
//
#include "roll_method_base.h"

namespace epoch_script::futures {
class LastTradingDayMethod : public RolloverMethodBase {
public:
  explicit LastTradingDayMethod(int offset) : RolloverMethodBase(offset) {}

  bool IsRollDate(const Input &input) const final;

  RolloverType GetType() const final {
    return epoch_core::RolloverType::LastTradingDay;
  }
};
} // namespace epoch_script::futures