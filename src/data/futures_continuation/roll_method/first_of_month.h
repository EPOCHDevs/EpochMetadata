#pragma once
//
// Created by dewe on 12/4/23.
//
#include "roll_method_base.h"

namespace epoch_script::futures {
class FirstOfMonthRollMethod : public RolloverMethodBase {
public:
  explicit FirstOfMonthRollMethod(int offset) : RolloverMethodBase(offset) {}

  bool IsRollDate(const Input &input) const final;

  inline epoch_core::RolloverType GetType() const final {
    return epoch_core::RolloverType::FirstOfMonth;
  }
};
} // namespace epoch_script::futures