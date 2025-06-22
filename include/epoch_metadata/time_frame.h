//
// Created by adesola on 1/4/25.
//

#pragma once
#include "date_time/date_offsets.h"
#include "epoch_frame/day_of_week.h"
#include "epoch_frame/time_delta.h"
#include <epoch_core/enum_wrapper.h>
#include <epoch_frame/factory/date_offset_factory.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

CREATE_ENUM(StratifyxTimeFrameType, minute, hour, day, week, month, quarter,
            year);
CREATE_ENUM(StratifyxBarType, TickBar, VolumeBar, DollarBar, TickImbalanceBar,
            VolumeImbalanceBar, DollarImbalanceBar, TimeBar);

CREATE_ENUM(AnchoredTimeFrameType, Start, End);

namespace epoch_metadata {

bool IsIntraday(epoch_core::EpochOffsetType);

class TimeFrame {
public:
  explicit TimeFrame(epoch_frame::DateOffsetHandlerPtr offset);

  bool IsIntraDay() const;

  std::string ToString() const;

  [[nodiscard]] epoch_frame::DateOffsetHandlerPtr GetOffset() const {
    return m_offset;
  }

  bool operator==(TimeFrame const &other) const;

  bool operator!=(TimeFrame const &other) const;

  bool operator<(TimeFrame const &other) const;

  std::string Serialize() const;

private:
  epoch_frame::DateOffsetHandlerPtr m_offset;
};

TimeFrame CreateTimeFrameFromYAML(YAML::Node const &);

epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandlerFromJSON(glz::json_t const &);
glz::json_t
CreateDateOffsetHandlerJSON(epoch_frame::DateOffsetHandlerPtr const &);

epoch_frame::RelativeDelta CreateRelativeDeltaFromJSON(glz::json_t const &);
glz::json_t CreateRelativeDeltaJSON(epoch_frame::RelativeDelta const &);

struct TimeFrameHash {
  size_t operator()(TimeFrame const &timeframe) const {
    return std::hash<std::string>()(timeframe.ToString());
  }
};

using TimeFrameSet = std::unordered_set<TimeFrame, TimeFrameHash>;

template <class T>
using TimeFrameHashMap = std::unordered_map<TimeFrame, T, TimeFrameHash>;
} // namespace epoch_metadata

namespace glz {
template <> struct meta<epoch_frame::DateOffsetHandlerPtr> {
  static constexpr auto read = [](epoch_frame::DateOffsetHandlerPtr &x,
                                  const json_t &input) {
    x = epoch_metadata::CreateDateOffsetHandlerFromJSON(input);
  };

  static constexpr auto write =
      [](const epoch_frame::DateOffsetHandlerPtr &x) -> auto {
    return epoch_metadata::CreateDateOffsetHandlerJSON(x);
  };

  static constexpr auto value = glz::custom<read, write>;
};

template <> struct meta<epoch_metadata::TimeFrame> {
  static constexpr auto read =
      [](epoch_metadata::TimeFrame &x,
         const epoch_frame::DateOffsetHandlerPtr &input) {
        if (input) {
          x = epoch_metadata::TimeFrame(input);
        }
      };

  static constexpr auto write = [](const epoch_metadata::TimeFrame &x) -> auto {
    return x.GetOffset();
  };

  static constexpr auto value = glz::custom<read, write>;
};

template <> struct meta<std::optional<epoch_metadata::TimeFrame>> {
  static constexpr auto read = [](std::optional<epoch_metadata::TimeFrame> &x,
                                  const glz::json_t &input) {
    if (input.is_null()) {
      x = std::nullopt;
    } else {
      auto offset = epoch_metadata::CreateDateOffsetHandlerFromJSON(input);
      if (offset) {
        x = epoch_metadata::TimeFrame(offset);
      } else {
        x = std::nullopt;
      }
    }
  };

  static constexpr auto write =
      [](const std::optional<epoch_metadata::TimeFrame> &x) -> auto {
    if (x) {
      return epoch_metadata::CreateDateOffsetHandlerJSON(x->GetOffset());
    }
    return glz::json_t{};
  };

  static constexpr auto value = glz::custom<read, write>;
};
} // namespace glz

namespace YAML {
template <> struct convert<epoch_frame::DateOffsetHandlerPtr> {
  static bool decode(const Node &node, epoch_frame::DateOffsetHandlerPtr &rhs);
};

template <> struct convert<epoch_metadata::TimeFrame> {
  static bool decode(const Node &node, epoch_metadata::TimeFrame &rhs) {
    rhs =
        epoch_metadata::TimeFrame(node.as<epoch_frame::DateOffsetHandlerPtr>());
    return true;
  }
};
} // namespace YAML