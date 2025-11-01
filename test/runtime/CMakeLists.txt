# Runtime/Orchestrator tests - merged into main epoch_flow_test executable

target_sources(epoch_flow_test PRIVATE
    # Orchestrator tests
    orchestrator_initialization_test.cpp
    orchestrator_error_handling_test.cpp
    orchestrator_parallel_execution_test.cpp
    orchestrator_report_caching_test.cpp
    orchestrator_selector_caching_test.cpp
    orchestrator_cross_sectional_test.cpp
    orchestrator_graph_topologies_test.cpp

    # Integration tests
    multiple_reporters_integration_test.cpp
    tearsheet_merge_test.cpp
    transform_flow_graph_test.cpp
)

add_subdirectory(testing)

target_include_directories(epoch_flow_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Copy test data directories to runtime output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/flow_graph_test_cases
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/flow_source_test_cases
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
