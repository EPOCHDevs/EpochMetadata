//
// Utility to dump all transform metadata to JSON
//
#include "common.h"
#include "catch2/catch_session.hpp"
#include "epoch_frame/factory/calendar_factory.h"
#include "epoch_script/strategy/registration.h"
#include "epoch_script/transforms/core/registration.h"
#include "epoch_script/transforms/core/registry.h"
#include "epoch_script/core/glaze_custom_types.h"
#include <arrow/compute/initialize.h>
#include <google/protobuf/stubs/common.h>
#include <absl/log/initialize.h>
#include <epoch_data_sdk/model/asset/asset_database.hpp>
#include <epoch_core/macros.h>
#include <glaze/glaze.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace epoch_script::transforms;

int main(int argc, char* argv[]) {
  try {
    // Parse arguments
    std::string outputFile = argc > 1 ? argv[1] : "transform_metadata.json";

    // Initialize (same as catch_main.cpp)
    absl::InitializeLog();
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    auto arrowComputeStatus = arrow::compute::Initialize();
    if (!arrowComputeStatus.ok()) {
      std::stringstream errorMsg;
      errorMsg << "arrow compute initialized failed: " << arrowComputeStatus << std::endl;
      throw std::runtime_error(errorMsg.str());
    }

    epoch_frame::calendar::CalendarFactory::instance().Init();

    AssertFromFormat(
        data_sdk::asset::AssetSpecificationDatabase::GetInstance().IsInitialized(),
        "Failed to initialize Asset Specification Database.");

    epoch_script::transforms::RegisterTransformMetadata(epoch_script::DEFAULT_YAML_LOADER);
    epoch_script::transform::InitializeTransforms(epoch_script::DEFAULT_YAML_LOADER, {}, {});

    setenv("POLYGON_API_KEY", "ptMp4LUoa1sgSpTFS7v8diiVtnimqH46", 1);
    setenv("FRED_API_KEY", "b6561c96d3615458fcae0b57580664f3", 1);

    // Get all metadata
    const auto& allMetadata = ITransformRegistry::GetInstance().GetMetaData();

    // Convert to vector
    std::vector<TransformsMetaData> metadataList;
    metadataList.reserve(allMetadata.size());
    for (const auto& [id, metadata] : allMetadata) {
      metadataList.push_back(metadata);
    }

    // Serialize to JSON
    std::string json;
    auto err = glz::write_file_json(metadataList, outputFile, json);

    if (!err) {
      std::cout << "✓ Dumped " << metadataList.size() << " transforms to " << outputFile << std::endl;
    }
    else {
      std::cerr << "Failed: " << glz::format_error(err, json) << std::endl;
    }


    // Cleanup
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    return 1;
  }
}
