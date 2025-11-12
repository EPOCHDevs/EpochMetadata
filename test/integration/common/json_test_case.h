#pragma once

#include <string>
#include <optional>
#include <map>
#include <glaze/glaze.hpp>
#include "transforms/compiler/ast_compiler.h"



namespace epoch_script::test {

/**
 * Column validation rules for runtime output validation.
 *
 * Validates specific columns in output dataframes/tearsheets/event markers.
 */
enum class ColumnValidation {
    at_least_one_valid,  // At least one non-null/valid value in column
    all_nulls            // All values must be null
};

/**
 * Runtime validation configuration.
 *
 * Specifies expected outputs and validation rules for runtime execution tests.
 * Tests can validate executor outputs (dataframes), tearsheets (reports),
 * and event markers by checking column-level properties.
 */
struct RuntimeValidation {
    /**
     * Column validation specification for a single output type.
     * Maps column name -> validation rule.
     */
    struct OutputColumnValidation {
        std::map<std::string, ColumnValidation> columns;
    };

    // Executor outputs (TimeFrameAssetDataFrameMap - dataframes with signals/positions)
    std::optional<OutputColumnValidation> executor_outputs;

    // Tearsheets (AssetReportMap - protobuf TearSheet messages with metrics)
    std::optional<OutputColumnValidation> tearsheets;

    // Event markers (AssetEventMarkerMap - UI interaction points with data)
    std::optional<OutputColumnValidation> event_markers;
};

/**
 * JSON-based test case structure.
 *
 * Replaces directory-based test structure with single JSON file containing:
 * - input: Inline EpochScript source code
 * - graph: Expected compilation output (AST nodes)
 * - runtime: Runtime validation rules (optional)
 * - error: Expected error message (for negative tests)
 *
 * Test types:
 * 1. Compilation-only: input + graph (runtime is null)
 * 2. Full integration: input + graph + runtime
 * 3. Error tests: input + error (graph and runtime are null)
 */
struct JsonTestCase {
    // Inline EpochScript source code
    std::string input;

    // Expected compilation output (graph.json equivalent)
    // Null for error tests
    std::optional<CompilationResult> graph;

    // Runtime validation rules (optional)
    // Null for compilation-only tests and error tests
    std::optional<RuntimeValidation> runtime;

    // Expected error message (for negative tests)
    // Null for successful compilation tests
    std::optional<std::string> error;
};

} // namespace epoch_script::test

// Glaze reflection bindings for JSON serialization/deserialization
template <>
struct glz::meta<epoch_script::test::ColumnValidation> {
    using enum epoch_script::test::ColumnValidation;
    static constexpr auto value = enumerate(
        "at_least_one_valid", at_least_one_valid,
        "all_nulls", all_nulls
    );
};

template <>
struct glz::meta<epoch_script::test::RuntimeValidation::OutputColumnValidation> {
    using T = epoch_script::test::RuntimeValidation::OutputColumnValidation;
    static constexpr auto value = object(
        "columns", &T::columns
    );
};

template <>
struct glz::meta<epoch_script::test::RuntimeValidation> {
    using T = epoch_script::test::RuntimeValidation;
    static constexpr auto value = object(
        "executor_outputs", &T::executor_outputs,
        "tearsheets", &T::tearsheets,
        "event_markers", &T::event_markers
    );
};

template <>
struct glz::meta<epoch_script::test::JsonTestCase> {
    using T = epoch_script::test::JsonTestCase;
    static constexpr auto value = object(
        "input", &T::input,
        "graph", &T::graph,
        "runtime", &T::runtime,
        "error", &T::error
    );
};
