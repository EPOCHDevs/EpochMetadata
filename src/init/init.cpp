//
// Created by adesola on 4/24/25.
//
#include "init.h"

#include <epoch_core/macros.h>
#include <epoch_frame/factory/calendar_factory.h>
#include <epoch_data_sdk/model/asset/asset_database.hpp>
#include <epoch_script/transforms/core/registration.h>
#include <arrow/compute/api.h>

namespace epoch_script {

void InitEpochScript() {
  // Initialize Arrow Compute
  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok()) {
    std::stringstream errorMsg;
    errorMsg << "arrow compute initialized failed: " << arrowComputeStatus
             << std::endl;
    throw std::runtime_error(errorMsg.str());
  }

  // Initialize Calendar Factory
  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Register Assets MetaData from S3
  AssertFromFormat(
      data_sdk::asset::AssetSpecificationDatabase::GetInstance().IsInitialized(),
      "Failed to initialize Asset Specification Database.");

  // Register All Transform MetaData
  // For tests, we just initialize transforms without S3 loaders
  epoch_script::transform::InitializeTransforms();
}

} // namespace epoch_script
