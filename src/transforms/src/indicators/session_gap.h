#pragma once
//
// Created by assistant on 9/1/24.
//

#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {

// Outputs exposed:
//   - gap_filled: Boolean - whether gap was filled during the trading session
//   - gap_retrace: Decimal - fraction of gap retraced (0..1+)
//   - gap_size: Decimal - gap size as percentage (signed: + up, - down)
//   - psc: Decimal - prior session close price
//   - psc_timestamp: Timestamp - prior session close timestamp
//
// Detects gaps at day boundaries (00:00 UTC) by comparing current day's open
// to prior day's close. Tracks fill behavior throughout the trading session.
// Uses OHLC from bars.
class SessionGap final : public ITransform {
public:
  explicit SessionGap(const TransformConfiguration &config)
      : ITransform(config),
        m_fillPercent(
            static_cast<double>(
                config
                    .GetOptionValue(
                        "fill_percent",
                        epoch_metadata::MetaDataOptionDefinition{100.0})
                    .GetInteger()) /
            100.0) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final;

private:
  double m_fillPercent;
};

} // namespace epoch_metadata::transform
