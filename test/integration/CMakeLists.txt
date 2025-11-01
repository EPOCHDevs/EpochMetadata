# Integration tests

# Main integration test runner
target_sources(epoch_flow_test PRIVATE
    flow_integration_test.cpp
)

# Copy integration test cases to runtime output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cases
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
