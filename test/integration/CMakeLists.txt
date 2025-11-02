# Integration tests

# Integration test common utilities
target_sources(epoch_script_test PRIVATE
        common/csv_data_loader.cpp
        common/csv_data_loader.h
        common/event_marker_comparator.cpp
        common/event_marker_comparator.h
        common/tearsheet_comparator.cpp
        common/tearsheet_comparator.h
        common/runtime_output_validator.cpp
        common/runtime_output_validator.h
)

# Main integration test runner
target_sources(epoch_script_test PRIVATE
        script_integration_test.cpp
)

# Copy integration test cases to runtime output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cases
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
