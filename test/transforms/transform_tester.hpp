#ifndef TRANSFORM_TESTER_HPP
#define TRANSFORM_TESTER_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <functional>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace epoch {
namespace test {

using Value = std::variant<double, bool, std::string, std::monostate>;
using Column = std::vector<Value>;
using Table = std::map<std::string, Column>;
using Options = std::map<std::string, std::variant<bool, double, std::string>>;

struct TestCase {
    std::string title;
    Table input;
    Table expect;
    Options options;
};

class TransformTester {
public:
    using TransformFunction = std::function<Table(const Table&, const Options&)>;

    struct TestResult {
        std::string testTitle;
        bool passed;
        std::string message;
        std::optional<Table> actualOutput;
    };

    static std::vector<TestCase> loadTestsFromYAML(const std::string& filePath);

    static TestResult runSingleTest(const TestCase& test, TransformFunction transform);

    static std::vector<TestResult> runAllTests(const std::string& yamlFilePath,
                                               TransformFunction transform);

    static void printResults(const std::vector<TestResult>& results);

    static bool compareTables(const Table& expected, const Table& actual,
                             std::string& differenceMessage);

private:
    static Value parseValue(const YAML::Node& node);
    static Column parseColumn(const YAML::Node& node);
    static Table parseTable(const YAML::Node& node);
    static Options parseOptions(const YAML::Node& node);
    static std::string valueToString(const Value& v);
    static std::string tableToString(const Table& table);
};

} // namespace test
} // namespace epoch

#endif // TRANSFORM_TESTER_HPP