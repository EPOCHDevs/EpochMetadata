#pragma once
//
// Simple VWAP extractor transform
// Returns provider-per-bar VWAP ('vw') as result
//
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class VWAPTransform final : public ITransform {
public:
  explicit VWAPTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    // Expect provider to supply 'vw' column when requiredDataSources contains "vw"
    auto series = bars["vw"];
    return MakeResult(series);
  }
};

} // namespace epoch_script::transform

