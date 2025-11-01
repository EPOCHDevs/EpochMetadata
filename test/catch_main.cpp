//
// Created by adesola on 3/21/25.
//
#include "catch2/catch_session.hpp"
#include "common.h"
#include "epoch_frame/factory/calendar_factory.h"
#include "epochflow/strategy/registration.h"
#include "../include/epochflow/transforms/core/registration.h"
#include <arrow/compute/initialize.h>
#include <iostream>
#include <google/protobuf/stubs/common.h>
#include "absl/log/initialize.h"

int main(int argc, char *argv[])
{
  absl::InitializeLog();          // optional, just to route logs
  GOOGLE_PROTOBUF_VERIFY_VERSION; // checks basic version compat

  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok())
  {
    std::stringstream errorMsg;
    errorMsg << "arrow compute initialized failed: " << arrowComputeStatus
             << std::endl;
    throw std::runtime_error(errorMsg.str());
  }
  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Register transform metadata
  epochflow::transforms::RegisterTransformMetadata(
      epochflow::DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epochflow::transform::InitializeTransforms(
      epochflow::DEFAULT_YAML_LOADER, {}, {});

  // your setup ...
  int result = Catch::Session().run(argc, argv);

  // your clean-up...
  google::protobuf::ShutdownProtobufLibrary();

  return result;
}
