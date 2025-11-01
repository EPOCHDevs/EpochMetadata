# EpochFlow Test Suite
add_executable(epoch_flow_test catch_main.cpp)
target_link_libraries(epoch_flow_test PRIVATE
    epochflow
    Catch2::Catch2
    yaml-cpp::yaml-cpp
    trompeloeil::trompeloeil
    protobuf::libprotobuf
)
target_include_directories(epoch_flow_test PRIVATE
    ../src
    .
)

# Set runtime output directory to bin (where test resources are copied)
set_target_properties(epoch_flow_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add shared testing utilities and mocks
add_subdirectory(testing)
add_subdirectory(mocks)

# Add unit tests
add_subdirectory(unit)

# Add integration tests
add_subdirectory(integration)

# Copy test files
set(METADATA_FILES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/files DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_compile_definitions(epoch_flow_test PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}"
    -DTRANSFORMS_TEST_CASES_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/transforms_test_cases")
MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")
