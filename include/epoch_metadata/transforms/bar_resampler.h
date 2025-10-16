#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_frame/dataframe.h"
#include "epoch_frame/factory/table_factory.h"
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/transforms/itransform.h"

#include <glaze/json/read.hpp>

namespace epoch_metadata::transform
{
  inline epoch_frame::DataFrame
  resample_ohlcv(epoch_frame::DataFrame const &df,
                 epoch_frame::DateOffsetHandlerPtr const &offset)
  {
    const auto C = epoch_metadata::EpochStratifyXConstants::instance();
    return df.resample_by_ohlcv({.freq = offset,
                                 .closed = epoch_core::GrouperClosedType::Right,
                                 .label = epoch_core::GrouperLabelType::Right},
                                {{"open", C.OPEN()},
                                 {"high", C.HIGH()},
                                 {"low", C.LOW()},
                                 {"close", C.CLOSE()},
                                 {"volume", C.VOLUME()}});
  }

  // TODO: Accept Option objects
  struct BarResampler
  {
    explicit BarResampler(
        const epoch_metadata::transform::TransformConfiguration &config)
    {
      auto interval = config.GetOptionValue("interval").GetInteger();
      glz::generic json;
      json["interval"] = interval;
      json["type"] = config.GetOptionValue("type").GetSelectOption();
      json["weekday"] = "Sunday";

      auto error = glz::read_json(m_timeframe, json.dump().value());
      if (error)
      {
        throw std::runtime_error(std::format("Failed to read timeframe: {}",
                                             glz::format_error(error)));
      }
    }

    [[nodiscard]] epoch_frame::DataFrame
    TransformData(epoch_frame::DataFrame const &bars) const
    {
      return resample_ohlcv(bars, m_timeframe->GetOffset());
    }

  private:
    std::optional<epoch_metadata::TimeFrame> m_timeframe;
  };
} // namespace epoch_metadata::transform
