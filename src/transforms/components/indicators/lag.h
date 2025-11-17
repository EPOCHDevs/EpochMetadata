#pragma once
//
// Created by adesola on 1/20/25.
//
#include <epoch_script/transforms/core/itransform.h>
#include "../type_tags.h"

namespace epoch_script::transform {

// Generic typed Lag transform template - DRY principle

template <typename TypeTag>
class TypedLag : public ITransform {
public:
  explicit TypedLag(const TransformConfiguration &config)
      : ITransform(config),
        m_period(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    epoch_frame::Series result = input.shift(m_period);
    return MakeResult(result);
  }

private:
  int m_period;
};

// Explicit type aliases using clear naming convention: {transform}_{type}
using LagString = TypedLag<StringType>;
using LagNumber = TypedLag<NumberType>;
using LagBoolean = TypedLag<BooleanType>;
using LagTimestamp = TypedLag<TimestampType>;

// Extern template declarations to reduce compilation time
extern template class TypedLag<StringType>;
extern template class TypedLag<NumberType>;
extern template class TypedLag<BooleanType>;
extern template class TypedLag<TimestampType>;

} // namespace epoch_script::transform