//
// Created by dewe on 9/21/24.
//

#pragma once
#include "epoch_frame/factory/dataframe_factory.h"
#include <epochflow/transforms/core/itransform.h>

CREATE_ENUM(LogicalOperator, _or, _and, _not, _xor, _and_not);

namespace epochflow::transform {

template <epoch_core::LogicalOperator sign>
class LogicalTransform : public ITransform {
public:
  explicit LogicalTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    using namespace epoch_frame;
    Series lhs = bars[GetInputId(epochflow::ARG0)];
    Series rhs = bars[GetInputId(epochflow::ARG1)];

    std::string function = epoch_core::LogicalOperatorWrapper::ToString(sign);
    return make_dataframe(
        lhs.index(),
        {arrow_utils::call_compute_array({lhs.array(), rhs.array()}, function)},
        {GetOutputId()});
  }
};

class LogicalNot : public ITransform {
public:
  explicit LogicalNot(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    return MakeResult(!bars[GetInputId()]);
  }
};

using LogicalOR = LogicalTransform<epoch_core::LogicalOperator::_or>;
using LogicalAND = LogicalTransform<epoch_core::LogicalOperator::_and>;
using LogicalXOR = LogicalTransform<epoch_core::LogicalOperator::_xor>;
using LogicalAND_NOT = LogicalTransform<epoch_core::LogicalOperator::_and_not>;
} // namespace epochflow::transform