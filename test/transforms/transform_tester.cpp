#include "transform_tester.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace epoch {
namespace test {

std::vector<TestCase> TransformTester::loadTestsFromYAML(const std::string& filePath) {
    std::vector<TestCase> testCases;

    try {
        YAML::Node root = YAML::LoadFile(filePath);

        if (!root["tests"]) {
            throw std::runtime_error("YAML file must have a 'tests' root node");
        }

        for (const auto& testNode : root["tests"]) {
            TestCase test;

            if (testNode["title"]) {
                test.title = testNode["title"].as<std::string>();
            } else {
                test.title = "Unnamed Test";
            }

            if (testNode["input"]) {
                test.input = parseTable(testNode["input"]);
            }

            if (testNode["expect"]) {
                test.expect = parseTable(testNode["expect"]);
            }

            if (testNode["options"]) {
                test.options = parseOptions(testNode["options"]);
            }

            testCases.push_back(test);
        }
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML: " + std::string(e.what()));
    }

    return testCases;
}

Value TransformTester::parseValue(const YAML::Node& node) {
    if (!node.IsDefined() || node.IsNull()) {
        return std::monostate{};
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

    if (node.Type() == YAML::NodeType::Map || node.Type() == YAML::NodeType::Sequence) {
        throw std::runtime_error("Complex types not supported as values");
    }

    return std::monostate{};
}

Column TransformTester::parseColumn(const YAML::Node& node) {
    Column column;

    if (!node.IsSequence()) {
        throw std::runtime_error("Column must be a sequence/array");
    }

    for (const auto& item : node) {
        column.push_back(parseValue(item));
    }

    return column;
}

Table TransformTester::parseTable(const YAML::Node& node) {
    Table table;

    if (!node.IsMap()) {
        throw std::runtime_error("Table must be a map of column names to arrays");
    }

    for (const auto& pair : node) {
        std::string columnName = pair.first.as<std::string>();
        table[columnName] = parseColumn(pair.second);
    }

    return table;
}

Options TransformTester::parseOptions(const YAML::Node& node) {
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

TransformTester::TestResult TransformTester::runSingleTest(const TestCase& test,
                                                           TransformFunction transform) {
    TestResult result;
    result.testTitle = test.title;

    try {
        Table actualOutput = transform(test.input, test.options);
        result.actualOutput = actualOutput;

        std::string diffMessage;
        result.passed = compareTables(test.expect, actualOutput, diffMessage);

        if (!result.passed) {
            result.message = "Output mismatch: " + diffMessage;
        } else {
            result.message = "Test passed";
        }
    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception during transform: " + std::string(e.what());
    }

    return result;
}

std::vector<TransformTester::TestResult> TransformTester::runAllTests(
    const std::string& yamlFilePath,
    TransformFunction transform) {

    std::vector<TestResult> results;

    try {
        std::vector<TestCase> testCases = loadTestsFromYAML(yamlFilePath);

        for (const auto& test : testCases) {
            results.push_back(runSingleTest(test, transform));
        }
    } catch (const std::exception& e) {
        TestResult errorResult;
        errorResult.testTitle = "Failed to load tests";
        errorResult.passed = false;
        errorResult.message = e.what();
        results.push_back(errorResult);
    }

    return results;
}

bool TransformTester::compareTables(const Table& expected, const Table& actual,
                                    std::string& differenceMessage) {
    std::stringstream diff;
    bool matches = true;

    for (const auto& [colName, expectedCol] : expected) {
        auto actualIt = actual.find(colName);
        if (actualIt == actual.end()) {
            diff << "Missing column: " << colName << "\n";
            matches = false;
            continue;
        }

        const Column& actualCol = actualIt->second;

        if (expectedCol.size() != actualCol.size()) {
            diff << "Column '" << colName << "' size mismatch: expected "
                 << expectedCol.size() << ", got " << actualCol.size() << "\n";
            matches = false;
            continue;
        }

        for (size_t i = 0; i < expectedCol.size(); ++i) {
            bool valueMatches = false;

            if (expectedCol[i].index() != actualCol[i].index()) {
                valueMatches = false;
            } else {
                if (std::holds_alternative<double>(expectedCol[i])) {
                    double exp = std::get<double>(expectedCol[i]);
                    double act = std::get<double>(actualCol[i]);

                    if (std::isnan(exp) && std::isnan(act)) {
                        valueMatches = true;
                    } else if (std::abs(exp - act) < 1e-9) {
                        valueMatches = true;
                    }
                } else if (std::holds_alternative<bool>(expectedCol[i])) {
                    valueMatches = (std::get<bool>(expectedCol[i]) == std::get<bool>(actualCol[i]));
                } else if (std::holds_alternative<std::string>(expectedCol[i])) {
                    valueMatches = (std::get<std::string>(expectedCol[i]) == std::get<std::string>(actualCol[i]));
                } else if (std::holds_alternative<std::monostate>(expectedCol[i])) {
                    valueMatches = std::holds_alternative<std::monostate>(actualCol[i]);
                }
            }

            if (!valueMatches) {
                diff << "Column '" << colName << "' row " << i << " mismatch: expected "
                     << valueToString(expectedCol[i]) << ", got "
                     << valueToString(actualCol[i]) << "\n";
                matches = false;
            }
        }
    }

    for (const auto& [colName, _] : actual) {
        if (expected.find(colName) == expected.end()) {
            diff << "Unexpected column: " << colName << "\n";
            matches = false;
        }
    }

    differenceMessage = diff.str();
    return matches;
}

std::string TransformTester::valueToString(const Value& v) {
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
        return "\"" + std::get<std::string>(v) + "\"";
    } else {
        return "null";
    }
}

std::string TransformTester::tableToString(const Table& table) {
    std::stringstream ss;
    ss << "{\n";
    for (const auto& [colName, column] : table) {
        ss << "  " << colName << ": [";
        for (size_t i = 0; i < column.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << valueToString(column[i]);
        }
        ss << "]\n";
    }
    ss << "}";
    return ss.str();
}

void TransformTester::printResults(const std::vector<TestResult>& results) {
    int passed = 0;
    int failed = 0;

    for (const auto& result : results) {
        if (result.passed) {
            std::cout << "[PASS] " << result.testTitle << std::endl;
            passed++;
        } else {
            std::cout << "[FAIL] " << result.testTitle << std::endl;
            std::cout << "       " << result.message << std::endl;
            if (result.actualOutput) {
                std::cout << "       Actual output:\n";
                std::string tableStr = tableToString(*result.actualOutput);
                std::istringstream iss(tableStr);
                std::string line;
                while (std::getline(iss, line)) {
                    std::cout << "       " << line << std::endl;
                }
            }
            failed++;
        }
    }

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << (passed + failed) << std::endl;

    if (failed > 0) {
        std::cout << "Failed: " << failed << std::endl;
    }
}

} // namespace test
} // namespace epoch