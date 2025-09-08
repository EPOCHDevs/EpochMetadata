#pragma once
//
// Created by assistant on 9/1/24.
//

#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {

// Outputs exposed:
//   booleans: "up", "down", "up_filled", "down_filled"
//   decimals:
//     - "up_gap_size", "down_gap_size" (absolute price differences)
//     - "up_fill_fraction", "down_fill_fraction" (0..1 fraction on gap days,
//       NaN otherwise)
//     - "psc" (prior session close price when gap exists, NaN otherwise)
//   timestamps: "psc_timestamp" (timestamp of prior session close when gap
//   exists, null otherwise)
// No explicit options required. Uses OHLC from bars.
class GapClassify final : public ITransform {
public:
  explicit GapClassify(const TransformConfiguration &config)
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
