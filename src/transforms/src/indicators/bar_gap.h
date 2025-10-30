#pragma once
//
// Created by assistant on 10/30/25.
//

#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {

// Outputs exposed (same as session_gap for compatibility):
//   - gap_filled: Boolean - whether gap fills in current bar
//   - gap_retrace: Decimal - fraction of gap filled (0..1)
//   - gap_size: Decimal - gap size as percentage (signed: + up, - down)
//   - psc: Decimal - previous bar's close price (for reference)
//   - psc_timestamp: Timestamp - previous bar's timestamp
//
// Key difference from session_gap:
//   - Detects gaps between ANY consecutive bars (no day boundary check)
//   - No multi-bar state tracking (each bar independent)
//   - psc = previous bar close (not "prior session close")
//   - Suitable for intraday gaps: trading halts, liquidity gaps, pip gaps, etc.
//
// Options:
//   - fill_percent: Minimum percentage of gap that must be retraced (default 100)
//   - min_gap_size: Minimum gap size to detect, as percentage (default 0.0)
//                   Example: 0.04 for 4-pip gaps on EUR/USD (~0.0004/1.0 * 100)
class BarGap final : public ITransform {
public:
  explicit BarGap(const TransformConfiguration &config)
      : ITransform(config),
        m_fillPercent(
            static_cast<double>(
                config
                    .GetOptionValue(
                        "fill_percent",
                        epoch_metadata::MetaDataOptionDefinition{100.0})
                    .GetInteger()) /
            100.0),
        m_minGapSize(
            static_cast<double>(
                config
                    .GetOptionValue(
                        "min_gap_size",
                        epoch_metadata::MetaDataOptionDefinition{0.0})
                    .GetDecimal())) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const final;

private:
  double m_fillPercent;
  double m_minGapSize;  // Minimum gap size (percentage) to detect
};

} // namespace epoch_metadata::transform
