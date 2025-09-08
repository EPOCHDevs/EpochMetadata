//
// Created by adesola on 3/21/25.
//
#include "catch2/catch_session.hpp"
#include "common.h"
#include "epoch_frame/factory/calendar_factory.h"
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/transforms/registration.h"
#include <arrow/compute/initialize.h>
#include <iostream>

int main(int argc, char *argv[]) {
  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok()) {
    std::stringstream errorMsg;
    errorMsg << "arrow compute initialized failed: " << arrowComputeStatus
             << std::endl;
    throw std::runtime_error(errorMsg.str());
  }
  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Register transform metadata
  epoch_metadata::transforms::RegisterTransformMetadata(
      epoch_metadata::DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epoch_metadata::transform::InitializeTransforms(
      epoch_metadata::DEFAULT_YAML_LOADER,
      epoch_metadata::LoadAIGeneratedResources(
          epoch_metadata::AI_GENERATED_ALGORITHMS_DIR),
      epoch_metadata::LoadAIGeneratedResources(
          epoch_metadata::AI_GENERATED_STRATEGIES_DIR));

  // your setup ...
  int result = Catch::Session().run(argc, argv);

  // your clean-up...

  return result;
}
