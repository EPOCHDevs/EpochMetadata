//
// Utility to dump all transform metadata to JSON for documentation verification
//
#include <epoch_script/transforms/core/registry.h>
#include <epoch_script/transforms/core/registration.h>
#include <epoch_script/core/constants.h>
#include <glaze/glaze.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

using namespace epoch_script::transforms;

// Glaze JSON serialization for TransformsMetaData
template <>
struct glz::meta<IOMetaData> {
  using T = IOMetaData;
  static constexpr auto value = object(
    "type", &T::type,
    "id", &T::id,
    "name", &T::name,
    "allowMultipleConnections", &T::allowMultipleConnections,
    "isFilter", &T::isFilter
  );
};

template <>
struct glz::meta<TransformsMetaData> {
  using T = TransformsMetaData;
  static constexpr auto value = object(
    "id", &T::id,
    "name", &T::name,
    "category", &T::category,
    "plotKind", &T::plotKind,
    "desc", &T::desc,
    "inputs", &T::inputs,
    "outputs", &T::outputs,
    "options", &T::options,
    "tags", &T::tags,
    "isCrossSectional", &T::isCrossSectional,
    "atLeastOneInputRequired", &T::atLeastOneInputRequired,
    "requiresTimeFrame", &T::requiresTimeFrame,
    "requiredDataSources", &T::requiredDataSources,
    "intradayOnly", &T::intradayOnly,
    "allowNullInputs", &T::allowNullInputs,
    "strategyTypes", &T::strategyTypes,
    "relatedTransforms", &T::relatedTransforms,
    "assetRequirements", &T::assetRequirements,
    "usageContext", &T::usageContext,
    "limitations", &T::limitations
  );
};

int main(int argc, char* argv[]) {
  try {
    std::string outputFile = "transform_metadata.json";
    std::string metadataDir = "test/files";

    if (argc > 1) {
      outputFile = argv[1];
    }
    if (argc > 2) {
      metadataDir = argv[2];
    }

    // Initialize transform metadata registry
    const auto loader = [metadataDir](std::string const &path) {
      return YAML::LoadFile(std::filesystem::path{metadataDir} / path);
    };

    RegisterTransformMetadata(loader);

    // Get all metadata
    const auto& allMetadata = ITransformRegistry::GetInstance().GetMetaData();

    // Convert to vector for JSON serialization
    std::vector<TransformsMetaData> metadataList;
    metadataList.reserve(allMetadata.size());
    for (const auto& [id, metadata] : allMetadata) {
      metadataList.push_back(metadata);
    }

    // Serialize to JSON
    std::string json = glz::write_json(metadataList).value_or("{}");

    // Write to file
    std::ofstream outFile(outputFile);
    if (!outFile) {
      std::cerr << "Failed to open output file: " << outputFile << std::endl;
      return 1;
    }
    outFile << json;
    outFile.close();

    std::cout << "Successfully dumped " << metadataList.size()
              << " transforms to " << outputFile << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
