//
// Created by Claude Code
//

#pragma once
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/series.h>

namespace epoch_script::transform {

// IsNull Transform - checks if values are null
class IsNull : public ITransform {
public:
  explicit IsNull(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];

    // Use epoch_frame's is_null() method
    epoch_frame::Series result = input.is_null();

    return MakeResult(result);
  }
};

// IsValid Transform - checks if values are valid (not null)
class IsValid : public ITransform {
public:
  explicit IsValid(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];

    // Use epoch_frame's is_valid() method
    epoch_frame::Series result = input.is_valid();

    return MakeResult(result);
  }
};

// IsZero Transform - checks if values equal zero
class IsZero : public ITransform {
public:
  explicit IsZero(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];

    // Compare with zero using epoch_frame's equality operator
    epoch_frame::Scalar zero_scalar(0.0);
    epoch_frame::Series result = input == zero_scalar;

    return MakeResult(result);
  }
};

// IsOne Transform - checks if values equal one
class IsOne : public ITransform {
public:
  explicit IsOne(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];

    // Compare with one using epoch_frame's equality operator
    epoch_frame::Scalar one_scalar(1.0);
    epoch_frame::Series result = input == one_scalar;

    return MakeResult(result);
  }
};

} // namespace epoch_script::transform
