add_executable(epoch_metadata_test catch_main.cpp metadata_options_test.cpp sql_statement_test.cpp timeframe_test.cpp)
target_link_libraries(epoch_metadata_test PRIVATE epoch_metadata Catch2::Catch2  yaml-cpp::yaml-cpp)
target_include_directories(epoch_metadata_test PRIVATE ../src . )

# Set runtime output directory to bin (where test resources are copied)
set_target_properties(epoch_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(strategy)
add_subdirectory(transforms)
add_subdirectory(epochflow)
add_subdirectory(selectors)

set(METADATA_FILES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/files DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_compile_definitions(epoch_metadata_test PUBLIC
    -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}"
    -DTRANSFORMS_TEST_CASES_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/transforms_test_cases")
MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")
