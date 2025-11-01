#pragma once
#include "date_time/date_offsets.h"
#include "epoch_script/core/glaze_custom_types.h"
#include <epoch_core/enum_wrapper.h>
#include <variant>

CREATE_ENUM(SessionType, Sydney, Tokyo, London, NewYork, AsianKillZone,
            LondonOpenKillZone, NewYorkKillZone, LondonCloseKillZone);

namespace epoch_script::strategy {
using SessionVariant =
    std::variant<epoch_frame::SessionRange, epoch_core::SessionType>;

inline bool operator==(const std::optional<SessionVariant> &lhs,
                       const std::optional<SessionVariant> &rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }
  if (!lhs.has_value() && !rhs.has_value()) {
    return true;
  }

  return std::visit(
      [&]<typename T1, typename T2>(const T1 &lhs_arg, const T2 &rhs_arg) {
        if constexpr (std::is_same_v<T1, T2>) {
          return lhs_arg == rhs_arg;
        } else {
          return false;
        }
      },
      *lhs, *rhs);
}
} // namespace epoch_script::strategy
