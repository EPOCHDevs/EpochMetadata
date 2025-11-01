// mock_transforms.h
#pragma once

#include "../itransform.h"
#include <memory>
#include <trompeloeil.hpp>

namespace epoch_flow::runtime {

// Mock for ITransformBase (Pair Transform)
struct MockTransformBase : public ITransform {
  MockTransformBase(TransformConfiguration config)
      : ITransform(std::move(config)) {}

  MAKE_CONST_MOCK1(ExecutePipeline,
                   epoch_frame::DataFrame(const epoch_frame::DataFrame &),
                   override);
};

// Mock for ICrossSectionalTransform (Cross-Sectional Transform)
struct MockCrossSectionalTransform : public ICrossSectionalTransform {
  explicit MockCrossSectionalTransform(TransformConfiguration config)
      : ICrossSectionalTransform(std::move(config)) {}

  MAKE_CONST_MOCK1(
      ExecutePipeline,
      epoch_frame::DataFrame(const std::vector<epoch_frame::DataFrame> &),
      override);
};

} // namespace epoch_flow::runtime
