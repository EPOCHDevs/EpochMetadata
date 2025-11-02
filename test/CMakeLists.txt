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

# Helper tool to generate graph.json from EpochScript source
add_executable(generate_graph ../generate_graph.cpp)
target_link_libraries(generate_graph PRIVATE epoch_script)
target_include_directories(generate_graph PRIVATE ../src)
set_target_properties(generate_graph PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set runtime output directory to bin (where test resources are copied)
set_target_properties(epoch_script_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add unit tests (includes shared utilities in unit/common)
add_subdirectory(unit)

# Add integration tests (includes mocks in integration/mocks)
add_subdirectory(integration)

# Copy test files
set(METADATA_FILES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/files DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_compile_definitions(epoch_script_test PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}"
    -DTRANSFORMS_TEST_CASES_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/transforms_test_cases")
MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")
