#include "epoch_testing/transform_tester_base.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace epoch {
namespace test {

// Helper function implementations
static std::string valueToString(const Value& v) {
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (std::isnan(d)) {
            return "nan";
        }
        std::stringstream ss;
        ss << std::setprecision(10) << d;
        return ss.str();
    } else if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else {
        return "null";
    }
}

// Template implementations
template<typename InputType>
std::optional<Value> TransformTesterBase<InputType>::parseValue(const YAML::Node& node) {
    if (!node.IsDefined() || node.IsNull()) {
        return std::nullopt;
    }

    if (node.Type() == YAML::NodeType::Scalar) {
        const std::string str = node.as<std::string>();

        if (str == "nan" || str == "NaN" || str == "NAN") {
            return std::numeric_limits<double>::quiet_NaN();
        }

        if (str == "true" || str == "false") {
            return node.as<bool>();
        }

        try {
            size_t pos;
            double d = std::stod(str, &pos);
            if (pos == str.length()) {
                return d;
            }
        } catch (...) {}

        return str;
    }

    return std::nullopt;
}

template<typename InputType>
Column TransformTesterBase<InputType>::parseColumn(const YAML::Node& node) {
    Column column;

    if (!node.IsSequence()) {
        throw std::runtime_error("Column must be a sequence/array");
    }

    for (const auto& item : node) {
        column.push_back(parseValue(item));
    }

    return column;
}

template<typename InputType>
Table TransformTesterBase<InputType>::parseTable(const YAML::Node& node) {
    Table table;

    if (!node.IsMap()) {
        throw std::runtime_error("Table must be a map");
    }

    for (const auto& pair : node) {
        std::string columnName = pair.first.as<std::string>();
        table[columnName] = parseColumn(pair.second);
    }

    return table;
}

template<typename InputType>
Options TransformTesterBase<InputType>::parseOptions(const YAML::Node& node) {
    Options options;

    if (!node.IsMap()) {
        return options;
    }

    for (const auto& pair : node) {
        std::string key = pair.first.as<std::string>();

        if (pair.second.Type() == YAML::NodeType::Scalar) {
            const std::string str = pair.second.as<std::string>();

            if (str == "true" || str == "false") {
                options[key] = pair.second.as<bool>();
            } else {
                try {
                    size_t pos;
                    double d = std::stod(str, &pos);
                    if (pos == str.length()) {
                        options[key] = d;
                    } else {
                        options[key] = str;
                    }
                } catch (...) {
                    options[key] = str;
                }
            }
        }
    }

    return options;
}

// Specialization for Table input type
template<>
Table TransformTesterBase<Table>::parseInput(const YAML::Node& node) {
    return parseTable(node);
}

template<typename InputType>
typename TransformTesterBase<InputType>::OutputPtr
TransformTesterBase<InputType>::parseExpectedOutput(const YAML::Node& node) {
    if (!node["type"]) {
        // Default to dataframe if no type specified
        return OutputTypeRegistry::instance().create("dataframe", node);
    }

    std::string type = node["type"].as<std::string>();
    return OutputTypeRegistry::instance().create(type, node);
}

template<typename InputType>
std::vector<typename TransformTesterBase<InputType>::TestCaseType>
TransformTesterBase<InputType>::loadTestsFromYAML(const std::string& filePath) {
    std::vector<TestCaseType> testCases;

    try {
        YAML::Node root = YAML::LoadFile(filePath);

        if (!root["tests"]) {
            throw std::runtime_error("YAML file must have a 'tests' root node");
        }

        for (const auto& testNode : root["tests"]) {
            TestCaseType test;

            test.title = testNode["title"].as<std::string>("Unnamed Test");
            test.input = parseInput(testNode["input"]);
            test.expect = parseExpectedOutput(testNode["expect"]);

            if (testNode["options"]) {
                test.options = parseOptions(testNode["options"]);
            }

            // Parse timestamp_columns if present
            if (testNode["timestamp_columns"]) {
                for (const auto& col : testNode["timestamp_columns"]) {
                    test.timestamp_columns.push_back(col.as<std::string>());
                }
            }

            // Parse index_column if present
            if (testNode["index_column"]) {
                test.index_column = testNode["index_column"].as<std::string>();
            }

            testCases.push_back(std::move(test));
        }
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML: " + std::string(e.what()));
    }

    return testCases;
}

template<typename InputType>
typename TransformTesterBase<InputType>::TestResult
TransformTesterBase<InputType>::runSingleTest(TestCaseType& test, TransformFunction transform) {
    TestResult result;
    result.testTitle = test.title;

    try {
        result.actualOutput = transform(test.input, test.options);

        if (result.actualOutput && test.expect) {
            result.passed = result.actualOutput->equals(*test.expect);
        } else {
            result.passed = (!result.actualOutput && !test.expect);
        }

        if (!result.passed) {
            result.message = "Output mismatch:\nExpected: " +
                           (test.expect ? test.expect->toString() : "null") +
                           "\nActual: " +
                           (result.actualOutput ? result.actualOutput->toString() : "null");
        } else {
            result.message = "Test passed";
        }
    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

template<typename InputType>
std::vector<typename TransformTesterBase<InputType>::TestResult>
TransformTesterBase<InputType>::runAllTests(const std::string& yamlFilePath,
                                            TransformFunction transform) {
    std::vector<TestResult> results;

    try {
        auto testCases = loadTestsFromYAML(yamlFilePath);

        for (auto& test : testCases) {
            results.push_back(runSingleTest(test, transform));
        }
    } catch (const std::exception& e) {
        TestResult errorResult;
        errorResult.testTitle = "Failed to load tests";
        errorResult.passed = false;
        errorResult.message = e.what();
        results.push_back(std::move(errorResult));
    }

    return results;
}

template<typename InputType>
void TransformTesterBase<InputType>::printResults(const std::vector<TestResult>& results) {
    int passed = 0;
    int failed = 0;

    for (const auto& result : results) {
        if (result.passed) {
            std::cout << "[PASS] " << result.testTitle << std::endl;
            passed++;
        } else {
            std::cout << "[FAIL] " << result.testTitle << std::endl;
            std::cout << "       " << result.message << std::endl;
            failed++;
        }
    }

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << (passed + failed) << std::endl;

    if (failed > 0) {
        std::cout << "Failed: " << failed << std::endl;
    }
}

// Explicit instantiation for Table input type
template class TransformTesterBase<Table>;

} // namespace test
} // namespace epoch