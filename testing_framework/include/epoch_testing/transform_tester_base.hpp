#ifndef TRANSFORM_TESTER_BASE_HPP
#define TRANSFORM_TESTER_BASE_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <functional>
#include <optional>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace epoch {
namespace test {

// Base types for table data
using Value = std::variant<double, bool, std::string, std::monostate>;
using Column = std::vector<Value>;
using Table = std::map<std::string, Column>;
using Options = std::map<std::string, std::variant<bool, double, std::string>>;

// Base interface for output types - external clients can extend this
class IOutputType {
public:
    virtual ~IOutputType() = default;

    // Core methods that must be implemented
    virtual std::string getType() const = 0;
    virtual bool equals(const IOutputType& other) const = 0;
    virtual std::string toString() const = 0;

    // Factory method for creating output from YAML - must be implemented by derived classes
    static std::unique_ptr<IOutputType> fromYAML(const YAML::Node& node);
};

// Output type registry for extensibility
class OutputTypeRegistry {
public:
    using FactoryFunction = std::function<std::unique_ptr<IOutputType>(const YAML::Node&)>;

    static OutputTypeRegistry& instance() {
        static OutputTypeRegistry registry;
        return registry;
    }

    void registerType(const std::string& typeName, FactoryFunction factory) {
        factories_[typeName] = factory;
    }

    std::unique_ptr<IOutputType> create(const std::string& typeName, const YAML::Node& node) {
        auto it = factories_.find(typeName);
        if (it != factories_.end()) {
            return it->second(node);
        }
        throw std::runtime_error("Unknown output type: " + typeName);
    }

private:
    std::map<std::string, FactoryFunction> factories_;
};

// Test case structure
template<typename InputType = Table>
struct TestCase {
    std::string title;
    InputType input;
    std::unique_ptr<IOutputType> expect;
    Options options;
};

// Base transform tester
template<typename InputType = Table>
class TransformTesterBase {
public:
    using OutputPtr = std::unique_ptr<IOutputType>;
    using TransformFunction = std::function<OutputPtr(const InputType&, const Options&)>;
    using TestCaseType = TestCase<InputType>;

    struct TestResult {
        std::string testTitle;
        bool passed;
        std::string message;
        OutputPtr actualOutput;
    };

    // Core testing methods
    static std::vector<TestCaseType> loadTestsFromYAML(const std::string& filePath);
    static TestResult runSingleTest(TestCaseType& test, TransformFunction transform);
    static std::vector<TestResult> runAllTests(const std::string& yamlFilePath,
                                               TransformFunction transform);
    static void printResults(const std::vector<TestResult>& results);

protected:
    // Helper methods for parsing
    static InputType parseInput(const YAML::Node& node);
    static OutputPtr parseExpectedOutput(const YAML::Node& node);
    static Options parseOptions(const YAML::Node& node);
    static Value parseValue(const YAML::Node& node);
    static Column parseColumn(const YAML::Node& node);
    static Table parseTable(const YAML::Node& node);
};

} // namespace test
} // namespace epoch

#endif // TRANSFORM_TESTER_BASE_HPP