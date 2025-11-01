//
// Benchmark Main
// Custom main for Catch2 benchmarks with EpochMetadata initialization
//
#include "epoch_frame/serialization.h"
#include "epochflow/strategy/registration.h"
#include "../../include/epochflow/transforms/core/config_helper.h"
#include "../../include/epochflow/transforms/core/transform_registry.h"
#include "../../include/epochflow/transforms/core/registry.h"
#include <catch2/catch_session.hpp>
#include <../../include/epochflow/transforms/core/registration.h>
#include <epoch_frame/factory/calendar_factory.h>

#include <filesystem>

int main(const int argc, const char **argv)
{
  // Initialize Arrow compute library
  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok())
  {
    std::stringstream errorMsg;
    errorMsg << "Arrow compute initialization failed: " << arrowComputeStatus
             << std::endl;
    throw std::runtime_error(errorMsg.str());
  }

  // Initialize EpochMetadata transforms
  const auto DEFAULT_YAML_LOADER = [](std::string const &_path)
  {
    return YAML::LoadFile(std::filesystem::path{METADATA_FILES_DIR} / _path);
  };
  epochflow::transform::InitializeTransforms(DEFAULT_YAML_LOADER, {}, {});

  // Initialize calendar factory
  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Export transform registry as JSON for reference
  auto metadata = epochflow::transforms::ITransformRegistry::GetInstance().GetMetaData();
  std::vector<epochflow::transforms::TransformsMetaData> transforms;
  for (const auto &[_, v] : metadata)
  {
    transforms.push_back(v);
  }

  std::string buffer;
  auto ec = glz::write_file_json(transforms, "transform_registry.json", buffer);
  if (ec)
  {
    throw std::runtime_error("Failed to serialize Transform Registry to JSON: " + glz::format_error(ec, buffer));
  }

  // Run Catch2 session
  return Catch::Session().run(argc, argv);
}
