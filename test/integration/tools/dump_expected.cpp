//
// CLI: Dump expected outputs for an integration test case
// - Compiles input.txt to expected/graph.json
// - Executes runtime with input_data/* and saves:
//     expected/dataframes/{timeframe}_{asset}_result.csv
//     expected/tearsheets/{asset}.pb (and {asset}.json for readability)
//     expected/event_markers/{asset}.json
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <filesystem>

#include <absl/log/initialize.h>
#include <google/protobuf/stubs/common.h>
#include <arrow/compute/initialize.h>

#include <glaze/glaze.hpp>

#include "transforms/compiler/ast_compiler.h"
#include <epoch_script/transforms/core/registration.h>
#include <epoch_script/strategy/registration.h>
#include <epoch_script/transforms/runtime/types.h>
#include "transforms/runtime/orchestrator.h"

#include "../common/csv_data_loader.h"
#include "../common/tearsheet_comparator.h"
#include "../common/event_marker_comparator.h"

#include "../../common.h"

namespace fs = std::filesystem;
using namespace epoch_script;

static std::string ReadFile(const fs::path& path) {
  std::ifstream f(path);
  if (!f.is_open()) throw std::runtime_error("Failed to open file: " + path.string());
  std::stringstream ss; ss << f.rdbuf();
  return ss.str();
}

static void WriteFile(const fs::path& path, const std::string& content) {
  fs::create_directories(path.parent_path());
  std::ofstream f(path);
  if (!f.is_open()) throw std::runtime_error("Failed to write file: " + path.string());
  f << content;
}

static transform::TransformConfigurationList ToConfigList(const std::vector<strategy::AlgorithmNode>& nodes) {
  transform::TransformConfigurationList configs;
  for (const auto& node : nodes) {
    TransformDefinition def(node, node.timeframe);
    configs.push_back(transform::TransformConfiguration{def});
  }
  return configs;
}

static std::set<std::string> ExtractAssets(const runtime::TimeFrameAssetDataFrameMap& input) {
  std::set<std::string> assets;
  for (const auto& [tf, assetMap] : input) {
    for (const auto& [asset, _] : assetMap) assets.insert(asset);
  }
  return assets;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <test_case_dir>\n";
    std::cerr << "Example: " << argv[0] << " test/integration/test_cases/basic/simple_literal\n";
    return 1;
  }

  try {
    absl::InitializeLog();
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    auto arrowStatus = arrow::compute::Initialize();
    if (!arrowStatus.ok()) {
      std::cerr << "Arrow compute initialization failed" << std::endl;
      return 1;
    }

    // Register transforms and metadata
    {
      fs::path metaDir{METADATA_FILES_DIR};
      auto tf = metaDir / "transforms.yaml";
      std::cerr << "METADATA dir: " << metaDir << "\n";
      std::cerr << "Looking for: " << tf << (fs::exists(tf) ? " [exists]" : " [missing]") << "\n";
    }
    transforms::RegisterTransformMetadata(DEFAULT_YAML_LOADER);
    transform::InitializeTransforms(DEFAULT_YAML_LOADER, {}, {});

    fs::path testDir = fs::path(argv[1]);
    fs::path inputScript = testDir / "input.txt";
    fs::path inputDataDir = testDir / "input_data";
    fs::path expectedDir = testDir / "expected";
    fs::path expectedGraph = expectedDir / "graph.json";
    fs::path expectedDfDir = expectedDir / "dataframes";
    fs::path expectedTearsheetsDir = expectedDir / "tearsheets";
    fs::path expectedMarkersDir = expectedDir / "event_markers";

    if (!fs::exists(inputScript)) {
      throw std::runtime_error("Missing input.txt at: " + inputScript.string());
    }

    // 1) Compile input.txt → expected/graph.json
    std::string source = ReadFile(inputScript);
    AlgorithmAstCompiler compiler;
    auto result = compiler.compile(source);

    // Preserve original node order for runtime orchestration
    auto nodes_for_runtime = result;

    // Sort a copy for stable graph.json output
    auto nodes_for_json = result;
    std::sort(nodes_for_json.begin(), nodes_for_json.end(), [](const auto& a, const auto& b) { return a.id < b.id; });

    auto json = glz::write<glz::opts{.prettify = true}>(nodes_for_json);
    if (!json.has_value()) throw std::runtime_error("Failed to serialize compilation result");
    WriteFile(expectedGraph, json.value());
    std::cerr << "✓ Wrote graph.json → " << expectedGraph << "\n";

    // 2) Load runtime input data
    auto inputData = runtime::test::CsvDataLoader::LoadFromDirectory(inputDataDir);
    // Normalize timeframe key to compiled graph timeframe if needed
    if (!nodes_for_runtime.empty() && nodes_for_runtime.front().timeframe.has_value()) {
      const std::string tf_key = nodes_for_runtime.front().timeframe->ToString();
      if (!inputData.contains(tf_key)) {
        if (inputData.size() == 1) {
          auto it = inputData.begin();
          inputData[tf_key] = std::move(it->second);
          inputData.erase(it->first);
        }
      }
    }
    auto assets = ExtractAssets(inputData);
    std::cerr << "Assets detected: " << assets.size() << "\n";

    // 3) Create orchestrator and execute
    auto configList = ToConfigList(nodes_for_runtime);
    auto orchestrator = runtime::CreateDataFlowRuntimeOrchestrator(assets, configList);
    if (!orchestrator) throw std::runtime_error("Failed to create orchestrator");

    auto outputs = orchestrator->ExecutePipeline(inputData);
    auto reports = orchestrator->GetGeneratedReports();
    auto markers = orchestrator->GetGeneratedEventMarkers();

    // 4) Save expected dataframes: one CSV per timeframe/asset
    fs::create_directories(expectedDfDir);
    for (const auto& [timeframe, assetMap] : outputs) {
      for (const auto& [asset, df] : assetMap) {
        // Filename: {timeframe}_{asset}_result.csv
        fs::path outPath = expectedDfDir / (timeframe + std::string("_") + asset + std::string("_result.csv"));
        runtime::test::CsvDataLoader::WriteCsvFile(df, outPath, true);
        std::cerr << "✓ Wrote dataframe → " << outPath << "\n";
      }
    }

    // 5) Save expected tearsheets: {asset}.pb
    fs::create_directories(expectedTearsheetsDir);
    for (const auto& [asset, ts] : reports) {
      fs::path pbPath = expectedTearsheetsDir / (asset + ".pb");
      {
        std::ofstream out(pbPath, std::ios::binary);
        if (!out.is_open()) throw std::runtime_error("Failed to write: " + pbPath.string());
        if (!ts.SerializeToOstream(&out)) throw std::runtime_error("Failed to serialize TearSheet to PB: " + pbPath.string());
      }
      std::cerr << "✓ Wrote tearsheet (pb) → " << pbPath << "\n";
    }

    // 6) Save expected event markers: {asset}.json
    fs::create_directories(expectedMarkersDir);
    for (const auto& [asset, sels] : markers) {
      fs::path jsonPath = expectedMarkersDir / (asset + ".json");
      runtime::test::SelectorComparator::SaveJson(sels, jsonPath);
      std::cerr << "✓ Wrote event markers → " << jsonPath << "\n";
    }

    std::cerr << "\nAll expected artifacts written under: " << expectedDir << "\n";
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }
}
