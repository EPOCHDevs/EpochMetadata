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
        common/coverage_tracker.cpp
        common/coverage_tracker.h
        common/json_test_case.h
        common/json_test_loader.cpp
        common/json_test_loader.h
        common/test_orchestrator.cpp
        common/test_orchestrator.h
)

# Main integration test runner
target_sources(epoch_script_test PRIVATE
        script_integration_test_json.cpp
)

# Find glaze for JSON serialization
find_package(glaze CONFIG REQUIRED)

# Migration tool for converting old tests to JSON
add_executable(migrate_to_json
        tools/migrate_to_json.cpp
        common/json_test_case.h
)

target_include_directories(migrate_to_json PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/test
)

target_link_libraries(migrate_to_json PRIVATE
        glaze::glaze
        epoch_script
)

set_target_properties(migrate_to_json PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy integration test cases to runtime output directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/test_cases_json
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
