//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epoch_frame/datetime.h>

namespace epochflow::strategy {
struct DatePeriodConfig {
  epoch_frame::Date from, to;

  bool operator==(const DatePeriodConfig &other) const {
    return from == other.from && to == other.to;
  }
};
} // namespace epochflow::strategy