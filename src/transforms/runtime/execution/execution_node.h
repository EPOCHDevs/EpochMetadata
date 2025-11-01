//
// Created by adesola on 12/28/24.
//

#pragma once
#include "execution_context.h"
#include <epochflow/transforms/core/itransform.h>
#include <functional>
#include <memory>
#include <tbb/flow_graph.h>

namespace epoch_flow::runtime {
using execution_context_t = const tbb::flow::continue_msg &;

// Apply a regular transform
void ApplyDefaultTransform(const epochflow::transform::ITransformBase &transformer,
                           ExecutionContext &msg);

// Apply a cross-sectional transform
void ApplyCrossSectionTransform(const epochflow::transform::ITransformBase &transformer,
                                ExecutionContext &msg);

// Create a node function for a regular transform
// Pass the transformer, assets, and message by reference to avoid dangling
// references
template <bool is_cross_sectional>
std::function<void(execution_context_t)>
MakeExecutionNode(const epochflow::transform::ITransformBase &transformer,
                 ExecutionContext &msg) {

  return [&](execution_context_t /*unused*/) {
    if constexpr (is_cross_sectional) {
      ApplyCrossSectionTransform(transformer, msg);
    } else {
      ApplyDefaultTransform(transformer, msg);
    }
  };
}

} // namespace epoch_flow::runtime