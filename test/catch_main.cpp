//
// Created by adesola on 3/21/25.
//
#include "catch2/catch_session.hpp"
#include "common.h"
#include "epoch_frame/factory/calendar_factory.h"
#include "epoch_script/strategy/registration.h"
#include "../include/epoch_script/transforms/core/registration.h"
#include <arrow/compute/initialize.h>
#include <iostream>
#include <google/protobuf/stubs/common.h>
#include "absl/log/initialize.h"
#include <epoch_data_sdk/model/asset/asset_database.hpp>
#include <epoch_data_sdk/model/asset/index_constituents.hpp>
#include <epoch_core/macros.h>

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

  // Load asset specifications from S3
  AssertFromFormat(
      data_sdk::asset::AssetSpecificationDatabase::GetInstance().IsInitialized(),
      "Failed to initialize Asset Specification Database.");

  // Load index constituents from S3
  AssertFromFormat(
      data_sdk::asset::IndexConstituentsDatabase::GetInstance().IsInitialized(),
      "Failed to initialize Index Constituents Database.");

  // Register transform metadata
  epoch_script::transforms::RegisterTransformMetadata(
      epoch_script::DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epoch_script::transform::InitializeTransforms(
      epoch_script::DEFAULT_YAML_LOADER, {}, {});

    setenv("POLYGON_API_KEY", "ptMp4LUoa1sgSpTFS7v8diiVtnimqH46", 1);
    setenv("FRED_API_KEY", "b6561c96d3615458fcae0b57580664f3", 1);

  // your setup ...
  int result = Catch::Session().run(argc, argv);

  // your clean-up...
  google::protobuf::ShutdownProtobufLibrary();

  return result;
}
