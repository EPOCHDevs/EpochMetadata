# Runtime orchestrator unit tests
target_sources(epoch_script_test PRIVATE
    orchestrator_initialization_test.cpp
    orchestrator_error_handling_test.cpp
    orchestrator_parallel_execution_test.cpp
    orchestrator_report_caching_test.cpp
    orchestrator_selector_caching_test.cpp
    orchestrator_cross_sectional_test.cpp
    orchestrator_graph_topologies_test.cpp
)

target_include_directories(epoch_script_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
