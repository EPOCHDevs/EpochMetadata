#include "tearsheet_comparator.h"

#include <algorithm>
#include <fstream>
#include <google/protobuf/json/json.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace epoch_script::runtime::test {

std::string TearSheetComparator::ToJson(const epoch_proto::TearSheet& tearsheet, bool prettyPrint) {
    std::string jsonStr;

    google::protobuf::json::PrintOptions options;
    options.add_whitespace = prettyPrint;
    options.preserve_proto_field_names = true;

    auto status = google::protobuf::json::MessageToJsonString(tearsheet, &jsonStr, options);

    if (!status.ok()) {
        throw std::runtime_error("Failed to convert TearSheet to JSON: " + std::string(status.message()));
    }

    return jsonStr;
}

std::string TearSheetComparator::LoadJson(const std::filesystem::path& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + jsonPath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void TearSheetComparator::SaveJson(const epoch_proto::TearSheet& tearsheet,
                                   const std::filesystem::path& jsonPath) {
    // Ensure parent directory exists
    std::filesystem::create_directories(jsonPath.parent_path());

    std::string jsonStr = ToJson(tearsheet, true);

    std::ofstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + jsonPath.string());
    }

    file << jsonStr;
    file.close();

    SPDLOG_DEBUG("Saved TearSheet JSON to {}", jsonPath.string());
}

bool TearSheetComparator::Compare(const std::string& expectedJson,
                                  const std::string& actualJson,
                                  std::string& diff) {
    if (expectedJson == actualJson) {
        return true;
    }

    // Parse JSON to protobuf messages
    epoch_proto::TearSheet expectedProto;
    epoch_proto::TearSheet actualProto;

    auto expectedStatus = google::protobuf::json::JsonStringToMessage(expectedJson, &expectedProto);
    auto actualStatus = google::protobuf::json::JsonStringToMessage(actualJson, &actualProto);

    if (!expectedStatus.ok()) {
        diff = "Failed to parse expected JSON: " + std::string(expectedStatus.message());
        return false;
    }
    if (!actualStatus.ok()) {
        diff = "Failed to parse actual JSON: " + std::string(actualStatus.message());
        return false;
    }

    // Normalize (sort cards) before comparison
    NormalizeTearSheet(expectedProto);
    NormalizeTearSheet(actualProto);

    // Convert normalized protos back to JSON for comparison
    std::string normalizedExpected = ToJson(expectedProto, true);
    std::string normalizedActual = ToJson(actualProto, true);

    if (normalizedExpected == normalizedActual) {
        return true;
    }

    diff = GenerateDiff(normalizedExpected, normalizedActual);
    return false;
}

bool TearSheetComparator::Compare(const epoch_proto::TearSheet& expected,
                                  const epoch_proto::TearSheet& actual,
                                  std::string& diff) {
    // Normalize copies to ensure deterministic ordering
    epoch_proto::TearSheet exp = expected;
    epoch_proto::TearSheet act = actual;
    NormalizeTearSheet(exp);
    NormalizeTearSheet(act);

    // First try a deterministic binary comparison after normalization
    std::string exp_bin, act_bin;
    exp.SerializeToString(&exp_bin);
    act.SerializeToString(&act_bin);
    if (exp_bin == act_bin) {
        return true;
    }

    // Fall back to JSON diff for human-readable diagnostics; never throw
    try {
        std::string expectedJson = ToJson(exp, true);
        std::string actualJson = ToJson(act, true);
        return Compare(expectedJson, actualJson, diff);
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "TearSheet comparison failed to produce JSON diff: " << e.what();
        diff = oss.str();
        return false;
    }
}

void TearSheetComparator::NormalizeTearSheet(epoch_proto::TearSheet& tearsheet) {
    // Sort cards by a stable key to ensure deterministic ordering
    // We'll sort by: category, title, type (to handle multiple cards with same category)
    if (tearsheet.has_cards()) {
        auto* cards = tearsheet.mutable_cards();
        auto* cardsArray = cards->mutable_cards();

        std::sort(cardsArray->begin(), cardsArray->end(),
                  [](const epoch_proto::CardDef& a, const epoch_proto::CardDef& b) {
                      // First sort by category
                      if (a.category() != b.category()) {
                          return a.category() < b.category();
                      }

                      // Then by first data item's title (if exists)
                      std::string aTitle = a.data_size() > 0 ? a.data(0).title() : "";
                      std::string bTitle = b.data_size() > 0 ? b.data(0).title() : "";

                      return aTitle < bTitle;
                  });
    }
}

std::string TearSheetComparator::GenerateDiff(const std::string& expectedJson,
                                              const std::string& actualJson) {
    std::ostringstream diffStream;

    // Split into lines for comparison
    auto splitLines = [](const std::string& str) -> std::vector<std::string> {
        std::vector<std::string> lines;
        std::istringstream stream(str);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        return lines;
    };

    auto expectedLines = splitLines(expectedJson);
    auto actualLines = splitLines(actualJson);

    diffStream << "TearSheet JSON Diff:\n";
    diffStream << "-------------------\n\n";

    // Simple line-by-line diff
    size_t maxLines = std::max(expectedLines.size(), actualLines.size());

    for (size_t i = 0; i < maxLines; ++i) {
        std::string expectedLine = (i < expectedLines.size()) ? expectedLines[i] : "<missing>";
        std::string actualLine = (i < actualLines.size()) ? actualLines[i] : "<missing>";

        if (expectedLine != actualLine) {
            diffStream << "Line " << (i + 1) << ":\n";
            diffStream << "  Expected: " << expectedLine << "\n";
            diffStream << "  Actual:   " << actualLine << "\n";
            diffStream << "\n";
        }
    }

    diffStream << "\n=== Full Expected ===\n" << expectedJson << "\n\n";
    diffStream << "=== Full Actual ===\n" << actualJson << "\n";

    return diffStream.str();
}

} // namespace epoch_script::runtime::test
