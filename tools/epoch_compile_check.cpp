//
// EpochScript Compile Check Tool
//
// Usage: epoch_compile_check "<epochscript_code>"
//
// This executable validates EpochScript code syntax by attempting compilation.
// Outputs JSON response: {"status": "ok"|"error", "message": "error message"}
// Always returns exit code 0 for easy shell scripting.
//

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <filesystem>

#include <epoch_script/strategy/metadata.h>
#include <epoch_script/strategy/registration.h>
#include <epoch_script/transforms/core/registration.h>
#include <epoch_frame/factory/calendar_factory.h>
#include <epoch_data_sdk/model/asset/asset_database.hpp>
#include <epoch_core/macros.h>
#include <arrow/compute/initialize.h>
#include <google/protobuf/stubs/common.h>
#include <absl/log/initialize.h>
#include <yaml-cpp/yaml.h>

namespace {

// YAML loader for metadata files
const auto DEFAULT_YAML_LOADER = [](std::string const &_path) {
    return YAML::LoadFile(std::filesystem::path{METADATA_FILES_DIR} / _path);
};

// Initialize EpochScript runtime (minimal version - no API keys needed)
void InitializeRuntime() {
  absl::InitializeLog();
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok()) {
    std::stringstream errorMsg;
    errorMsg << "Arrow compute initialization failed: " << arrowComputeStatus;
    throw std::runtime_error(errorMsg.str());
  }

  epoch_frame::calendar::CalendarFactory::instance().Init();

  // Load asset specifications
  AssertFromFormat(
      data_sdk::asset::AssetSpecificationDatabase::GetInstance().IsInitialized(),
      "Failed to initialize Asset Specification Database.");

  // Register transform metadata
  epoch_script::transforms::RegisterTransformMetadata(DEFAULT_YAML_LOADER);

  // Initialize transforms registry
  epoch_script::transform::InitializeTransforms(DEFAULT_YAML_LOADER, {}, {});
}

// Escape JSON string
std::string EscapeJson(const std::string& s) {
  std::ostringstream o;
  for (char c : s) {
    switch (c) {
      case '"':  o << "\\\""; break;
      case '\\': o << "\\\\"; break;
      case '\b': o << "\\b";  break;
      case '\f': o << "\\f";  break;
      case '\n': o << "\\n";  break;
      case '\r': o << "\\r";  break;
      case '\t': o << "\\t";  break;
      default:
        if ('\x00' <= c && c <= '\x1f') {
          o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
          o << c;
        }
    }
  }
  return o.str();
}

// Output JSON response
void OutputJson(const std::string& status, const std::string& message) {
  std::cout << "{\"status\": \"" << status << "\", \"message\": \""
            << EscapeJson(message) << "\"}" << std::endl;
}

} // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    OutputJson("error", "Usage: epoch_compile_check \"<epochscript_code>\"");
    return 0;
  }

  std::string code = argv[1];

  if (code.empty()) {
    OutputJson("error", "Empty code provided");
    return 0;
  }

  try {
    // Initialize runtime
    InitializeRuntime();

    // Attempt to compile code (skip_sink_validation=true for faster syntax-only check)
    auto compiler = std::make_unique<epoch_script::strategy::PythonSource>(code, true);

    // If we got here, compilation succeeded
    OutputJson("ok", "Compilation successful");
    return 0;

  } catch (const std::exception& e) {
    // Compilation or runtime error
    OutputJson("error", e.what());
    return 0;
  } catch (...) {
    // Unknown error
    OutputJson("error", "Unknown error during compilation");
    return 0;
  }
}
