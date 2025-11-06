# EpochScript Test Suite
add_executable(epoch_script_test catch_main.cpp)
target_link_libraries(epoch_script_test PRIVATE
    epoch_script
    Catch2::Catch2
    yaml-cpp::yaml-cpp
    trompeloeil::trompeloeil
    protobuf::libprotobuf
)
target_include_directories(epoch_script_test PRIVATE
    ../src
    .
)

# Copy test files and define METADATA_FILES_DIR early
# Use the configurable SCRIPT_METADATA_FILES_DIR option from main CMakeLists.txt
set(METADATA_FILES_DIR ${SCRIPT_METADATA_FILES_DIR})

# Ensure the target directory exists and copy files
file(MAKE_DIRECTORY ${METADATA_FILES_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/files/ DESTINATION ${METADATA_FILES_DIR})

# Helper tool to generate graph.json from EpochScript source
add_executable(generate_graph integration/tools/generate_graph.cpp)
target_link_libraries(generate_graph PRIVATE epoch_script)
target_include_directories(generate_graph PRIVATE ../src .)
set_target_properties(generate_graph PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Helper tool to dump expected runtime artifacts for a test case
add_executable(dump_expected integration/tools/dump_expected.cpp)
target_link_libraries(dump_expected PRIVATE epoch_script)
target_include_directories(dump_expected PRIVATE ../src .)
set_target_properties(dump_expected PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Link integration common helpers (CSV loader, JSON/PB helpers)
target_sources(dump_expected PRIVATE
    integration/common/csv_data_loader.cpp
    integration/common/tearsheet_comparator.cpp
    integration/common/event_marker_comparator.cpp
)

# Provide METADATA_FILES_DIR define (used by DEFAULT_YAML_LOADER)
target_compile_definitions(dump_expected PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}")

# Set runtime output directory to bin (where test resources are copied)
set_target_properties(epoch_script_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add unit tests (includes shared utilities in unit/common)
add_subdirectory(unit)

# Add integration tests (includes mocks in integration/mocks)
add_subdirectory(integration)

target_compile_definitions(epoch_script_test PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}"
    -DTRANSFORMS_TEST_CASES_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/transforms_test_cases")

target_compile_definitions(generate_graph PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}")

MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")
