//
// EpochScript Standalone Test Runner
//
// Usage: epoch_test_runner "<output_dir>"
//
// This executable compiles EpochScript code and runs it, outputting results to structured directory.
// Reads code from code.epochscript and metadata from metadata.json.
// Used by the Python coverage agent for test generation.
//

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <glaze/glaze.hpp>

#include "common.h"
#include "transforms/compiler/ast_compiler.h"
#include "epoch_frame/factory/calendar_factory.h"
#include <epoch_script/strategy/registration.h>
#include <epoch_script/strategy/metadata.h>
#include <epoch_script/strategy/introspection.h>
#include <epoch_script/transforms/core/registration.h>
#include "data/factory.h"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/serialization.h>
#include <arrow/compute/initialize.h>
#include <google/protobuf/stubs/common.h>
#include <absl/log/initialize.h>
#include <epoch_data_sdk/model/asset/asset_database.hpp>
#include <epoch_core/macros.h>

namespace fs = std::filesystem;

namespace {

// Asset configurations for multi-asset testing
struct AssetConfiguration {
  std::string name;
  std::vector<std::string> assets;
};

static std::vector<AssetConfiguration> GetAssetConfigurations() {
  return {
      {"single_asset", {"AAPL-Stocks"}}
  };
}

// Metadata JSON structure for test case (code stored separately in code.epochscript)
struct MetadataJson {
  std::string name;
  std::string description;
  std::string role;
  std::string category;
};

// Helper: Normalize CompilationResult by sorting nodes by ID
epoch_script::CompilationResult NormalizeResult(
    epoch_script::CompilationResult result) {
  std::ranges::sort(result, [](const auto& a, const auto& b) {
    return a.id < b.id;
  });
  return result;
}


// Initialize EpochScript runtime
void InitializeRuntime() {
  absl::InitializeLog();
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok()) {
    std::stringstream errorMsg;
    errorMsg << "arrow compute initialized failed: " << arrowComputeStatus;
    throw std::runtime_error(errorMsg.str());
  }

  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Load asset specifications from S3
  AssertFromFormat(
      data_sdk::asset::AssetSpecificationDatabase::GetInstance().IsInitialized(),
      "Failed to initialize Asset Specification Database.");

  // Register transform metadata
  epoch_script::transforms::RegisterTransformMetadata(
      epoch_script::DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epoch_script::transform::InitializeTransforms(
      epoch_script::DEFAULT_YAML_LOADER, {}, {});

  setenv("POLYGON_API_KEY", "ptMp4LUoa1sgSpTFS7v8diiVtnimqH46", 1);
  setenv("FRED_API_KEY", "b6561c96d3615458fcae0b57580664f3", 1);
}

// Cleanup runtime
void ShutdownRuntime() {
  google::protobuf::ShutdownProtobufLibrary();
}

// Write string to file
void WriteToFile(const std::string& content, const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }
  file << content;
  file.close();
}

// Save graph.json
void SaveGraph(const epoch_script::CompilationResult& graph, const std::string& output_dir) {
  std::string graph_json;
  auto ec = glz::write_json(graph, graph_json);
  if (ec) {
    throw std::runtime_error("Failed to serialize graph to JSON");
  }

  std::string graph_path = output_dir + "/graph.json";
  WriteToFile(graph_json, graph_path);
}

// Save transformed data as parquet files
void SaveTransformedDataAsParquet(
    const std::string& output_dir,
    const std::string& profile_name,
    const auto& db_output_data
) {
  // For each timeframe in the transformed data
  for (const auto& [timeframe_key, asset_map] : db_output_data) {
    // Create directory: output_dir/{profile}/tables/{timeframe_key}/
    std::string timeframe_dir = output_dir + "/" + profile_name +
                                "/tables/" + timeframe_key + "/";
    fs::create_directories(timeframe_dir);

    // For each asset's dataframe
    for (const auto& [asset, dataframe] : asset_map) {
      std::string asset_filename = asset.GetID() + ".parquet.gzip";
      std::string output_path = std::filesystem::path(timeframe_dir) / asset_filename;

      // Configure parquet write options with gzip compression
      epoch_frame::ParquetWriteOptions options;
      options.compression = arrow::Compression::GZIP;
      options.include_index = true;

      // Write DataFrame as parquet using epoch_frame serialization
      auto status = epoch_frame::write_parquet(dataframe, output_path, options);
      if (!status.ok()) {
        throw std::runtime_error("Failed to write parquet for " + asset.GetID() +
                                 " at " + timeframe_key + ": " + status.ToString());
      }
    }
  }
}

// Run test on EpochScript source with output directory
void RunTest(const std::string& source, const std::string& output_dir) {
  // Compile EpochScript source
  auto compiler = std::make_unique<epoch_script::strategy::PythonSource>(source, false);

  // Extract compilation result
  auto compilation_result = compiler->GetCompilationResult();

  // Normalize by sorting nodes by ID
  auto normalized = NormalizeResult(compilation_result);

  // Save graph.json to output directory
  SaveGraph(normalized, output_dir);

  // Runtime execution with multiple asset configurations
  auto asset_configs = GetAssetConfigurations();

  for (const auto& asset_config : asset_configs) {
    // 1. Create StrategyConfig from test input
    epoch_script::strategy::StrategyConfig strategyConfig;
    strategyConfig.trade_signal.source = *compiler;
    strategyConfig.data.assets = asset_config.assets;

    // Determine date range based on timeframe (10 years for daily, 1 year for intraday)
    bool is_intraday = epoch_script::strategy::IsIntradayCampaign(strategyConfig);

    auto start_date = is_intraday
        ? epoch_frame::DateTime::from_str("2024-01-01", "UTC", "%Y-%m-%d").date()
        : epoch_frame::DateTime::from_str("2015-01-01", "UTC", "%Y-%m-%d").date();

    auto end_date = epoch_frame::DateTime::from_str("2025-01-01", "UTC", "%Y-%m-%d").date();

    // 2. Create database using strategy-aware factory
    auto dataModuleOption = epoch_script::data::factory::MakeDataModuleOptionFromStrategy(
        epoch_core::CountryCurrency::USD,
        epoch_script::strategy::DatePeriodConfig{start_date, end_date},
        strategyConfig
    );

    auto factory = epoch_script::data::factory::DataModuleFactory(dataModuleOption);
    auto database = factory.CreateDatabase();

    // 3. Run database pipeline (load + transform data)
    database->RunPipeline();

    // 4. Get outputs directly from database
    auto db_output_data = database->GetTransformedData();
    auto reports = database->GetGeneratedReports();
    auto event_markers = database->GetGeneratedEventMarkers();

    // 5. Validate that at least one output was generated
    bool has_output = !db_output_data.empty() || !reports.empty() || !event_markers.empty();

    if (!has_output) {
      throw std::runtime_error("Runtime execution produced no outputs for asset config: " + asset_config.name);
    }

    // 6. Save transformed data as parquet files
    SaveTransformedDataAsParquet(output_dir, asset_config.name, db_output_data);
  }
}

// Read code from code.epochscript file
std::string ReadCodeFromFile(const std::string& test_case_dir) {
  std::string code_path = test_case_dir + "/code.epochscript";

  std::ifstream file(code_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open code.epochscript: " + code_path);
  }

  // Read entire file into string
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: epoch_test_runner \"<output_dir>\"\n";
    std::cerr << "\n";
    std::cerr << "Arguments:\n";
    std::cerr << "  output_dir  Directory containing test case files and where outputs will be saved\n";
    std::cerr << "\n";
    std::cerr << "Expected input:\n";
    std::cerr << "  {output_dir}/code.epochscript  EpochScript source code to compile and run\n";
    std::cerr << "  {output_dir}/metadata.json     Test case metadata (name, description, category)\n";
    std::cerr << "\n";
    std::cerr << "Outputs created in output_dir:\n";
    std::cerr << "  graph.json          Compiled graph nodes\n";
    std::cerr << "  {profile}/tables/{timeframe}/{asset}.parquet.gzip  Transform outputs\n";
    std::cerr << "  error.txt           Error message (if compilation/runtime fails)\n";
    return 2;
  }

  const std::string output_dir = argv[1];

  try {
    // Read EpochScript code from code.epochscript file
    std::string epochscript_code = ReadCodeFromFile(output_dir);

    // Initialize runtime
    InitializeRuntime();

    // Run test with output directory (throws on error)
    RunTest(epochscript_code, output_dir);

    // Cleanup runtime
    ShutdownRuntime();

    return 0;

  } catch (const std::exception& e) {
    // Save error to error.txt
    std::string error_path = output_dir + "/error.txt";
    WriteToFile(e.what(), error_path);

    std::cerr << "Test failed: " << e.what() << "\n";
    return 1;
  }
}
