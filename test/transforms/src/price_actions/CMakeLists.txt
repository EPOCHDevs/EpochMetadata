target_sources(epoch_metadata_test PRIVATE
    smc_test.cpp
    chart_formations_test.cpp
)

# Copy test data directory to runtime bin
file(COPY "${CMAKE_CURRENT_LIST_DIR}/test_data"
        DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        PATTERN "*")

message(STATUS "Copied test data to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data")

# Define absolute path to test_data for TestMate compatibility
target_compile_definitions(epoch_metadata_test PRIVATE
    -DSMC_TEST_DATA_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data")
