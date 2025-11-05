#pragma once
//
// Simple Trade Count extractor transform
// Returns provider-per-bar trade count ('n') as result
//
#include <epoch_script/transforms/core/itransform.h>

namespace epoch_script::transform {

class TradeCountTransform final : public ITransform {
public:
  explicit TradeCountTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    auto series = bars["n"];
    return MakeResult(series);
  }
};

} // namespace epoch_script::transform

