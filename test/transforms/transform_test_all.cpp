//
// Generic transform tester that loads all YAML test cases
// Now uses the testing framework for better library separation
//
#include <epoch_testing/yaml_transform_tester.hpp>
#include <epoch_core/catch_defs.h>

using namespace epoch::test;

TEST_CASE("All Transform Tests - YAML Based", "[Transform][YAML]") {
    // Test only the predefined test_cases directory for this library
    YamlTransformTester::Config config("transforms_test_cases");
    config.recursive = true;
    config.requireTestCasesDir = false;

    // Run all tests using the registry-based transform runner
    YamlTransformTester::runTransformRegistryTests(config);
}