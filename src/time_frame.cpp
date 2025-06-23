//
// Created by adesola on 1/4/25.
//

#include "epoch_metadata/time_frame.h"
#include <epoch_core/error_type.h>

#include "date_time/date_offsets.h"
#include "epoch_core/macros.h"
#include "epoch_frame/factory/date_offset_factory.h"
#include "epoch_frame/relative_delta_options.h"
#include <glaze/json/json_t.hpp>
#include <utility>

namespace epoch_metadata {
bool IsIntraday(epoch_core::EpochOffsetType type) {
  return (type == epoch_core::EpochOffsetType::Hour ||
          type == epoch_core::EpochOffsetType::Minute ||
          type == epoch_core::EpochOffsetType::Second ||
          type == epoch_core::EpochOffsetType::Milli ||
          type == epoch_core::EpochOffsetType::Micro ||
          type == epoch_core::EpochOffsetType::Nano);
}

TimeFrame::TimeFrame(epoch_frame::DateOffsetHandlerPtr offset)
    : m_offset(std::move(offset)) {
  AssertFalseFromStream(m_offset == nullptr, "TimeFrame offset cannot be "
                                             "nullptr");
}

bool TimeFrame::IsIntraDay() const {
  return IsIntraday(m_offset->type());
}

std::string TimeFrame::ToString() const {
  return m_offset ? m_offset->name() : "";
}

bool TimeFrame::operator==(TimeFrame const &other) const {
  return m_offset->name() == other.m_offset->name();
}

bool TimeFrame::operator!=(TimeFrame const &other) const {
  return !(*this == other);
}

std::string TimeFrame::Serialize() const {
  std::string result;
  const auto error = glz::write_json(m_offset, result);
  if (error) {
    const auto desc = format_error(error);
    throw std::runtime_error("Failed to serialize timeframe: " + desc);
  }
  return result;
}

epoch_core::EpochOffsetType toOffset(epoch_core::StratifyxTimeFrameType type) {
  switch (type) {
    case epoch_core::StratifyxTimeFrameType::minute:
      return epoch_core::EpochOffsetType::Minute;
    case epoch_core::StratifyxTimeFrameType::hour:
      return epoch_core::EpochOffsetType::Hour;
    case epoch_core::StratifyxTimeFrameType::day:
      return epoch_core::EpochOffsetType::Day;
    case epoch_core::StratifyxTimeFrameType::week:
      return epoch_core::EpochOffsetType::Week;
    case epoch_core::StratifyxTimeFrameType::month:
      return epoch_core::EpochOffsetType::Month;
    case epoch_core::StratifyxTimeFrameType::quarter:
      return epoch_core::EpochOffsetType::Quarter;
    case epoch_core::StratifyxTimeFrameType::year:
      return epoch_core::EpochOffsetType::Year;
    default:
      break;
  }
  std::unreachable();
}

epoch_core::StratifyxTimeFrameType
fromOffset(epoch_core::EpochOffsetType type) {
  switch (type) {
  case epoch_core::EpochOffsetType::Minute:
    return epoch_core::StratifyxTimeFrameType::minute;
  case epoch_core::EpochOffsetType::Hour:
    return epoch_core::StratifyxTimeFrameType::hour;
  case epoch_core::EpochOffsetType::Day:
    return epoch_core::StratifyxTimeFrameType::day;
  case epoch_core::EpochOffsetType::Week:
    return epoch_core::StratifyxTimeFrameType::week;
  case epoch_core::EpochOffsetType::Month:
  case epoch_core::EpochOffsetType::MonthEnd:
    return epoch_core::StratifyxTimeFrameType::month;
  case epoch_core::EpochOffsetType::Quarter:
  case epoch_core::EpochOffsetType::QuarterEnd:
    return epoch_core::StratifyxTimeFrameType::quarter;
  case epoch_core::EpochOffsetType::Year:
  case epoch_core::EpochOffsetType::YearEnd:
    return epoch_core::StratifyxTimeFrameType::year;
  default:
    break;
  }
  throw std::runtime_error("Invalid Timeframe Type: " +
                           epoch_core::EpochOffsetTypeWrapper::ToString(type));
}

template <typename Serializer>
epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandler(Serializer const &buffer) {
  auto type = epoch_core::StratifyxTimeFrameTypeWrapper::FromString(
      buffer["type"].template as<std::string>());
  auto interval = buffer["interval"].template as<int>();
  switch (type) {
    case epoch_core::StratifyxTimeFrameType::day:
      return epoch_frame::factory::offset::days(interval);
    case epoch_core::StratifyxTimeFrameType::hour:
      return epoch_frame::factory::offset::hours(interval);
    case epoch_core::StratifyxTimeFrameType::minute:
      return epoch_frame::factory::offset::minutes(interval);
    case epoch_core::StratifyxTimeFrameType::week:
      return epoch_frame::factory::offset::weeks(interval);
    case epoch_core::StratifyxTimeFrameType::month:
      return epoch_frame::factory::offset::month_end(interval);
    case epoch_core::StratifyxTimeFrameType::quarter:
      return epoch_frame::factory::offset::quarter_end(interval);
    case epoch_core::StratifyxTimeFrameType::year:
      return epoch_frame::factory::offset::year_end(interval);
    default:
      break;
  }
  std::unreachable();
}

bool TimeFrame::operator<(TimeFrame const &other) const {
  const auto m_type = fromOffset(m_offset->type());
  const auto other_type = fromOffset(other.m_offset->type());
  if (m_type == other_type)
    return m_offset->n() < other.m_offset->n();

  return epoch_core::StratifyxTimeFrameTypeWrapper::toNumber(m_type) <
         epoch_core::StratifyxTimeFrameTypeWrapper::toNumber(other_type);
}

epoch_frame::DateOffsetHandlerPtr
CreateDateOffsetHandlerFromJSON(glz::json_t const &buffer) {
  if (buffer.is_null()) {
    return nullptr;
  }
  return CreateDateOffsetHandler(buffer);
}

glz::json_t
CreateDateOffsetHandlerJSON(epoch_frame::DateOffsetHandlerPtr const &x) {
  glz::json_t result;
  if (x) {
    result["type"] = epoch_core::StratifyxTimeFrameTypeWrapper::ToString(
        fromOffset(x->type()));
    result["interval"] = x->n();
  }

  return result;
}

} // namespace epoch_metadata

namespace YAML {
bool convert<epoch_frame::DateOffsetHandlerPtr>::decode(
    const Node &node, epoch_frame::DateOffsetHandlerPtr &rhs) {
  rhs = epoch_metadata::CreateDateOffsetHandler(node);
  return true;
}
} // namespace YAML