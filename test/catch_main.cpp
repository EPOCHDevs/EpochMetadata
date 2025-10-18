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
  epoch_metadata::transforms::RegisterTransformMetadata(
      epoch_metadata::DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epoch_metadata::transform::InitializeTransforms(
      epoch_metadata::DEFAULT_YAML_LOADER, {}, {});

  // your setup ...
  int result = Catch::Session().run(argc, argv);

  // your clean-up...
  google::protobuf::ShutdownProtobufLibrary();

  return result;
}
