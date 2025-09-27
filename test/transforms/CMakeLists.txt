target_sources(epoch_metadata_test PRIVATE
        dataframe_tester.cpp
        registrations_test.cpp
        tearsheet_output.cpp
        transform_test_all.cpp
        transform_tester.cpp
        transform_tester_base.cpp
        transforms_metadata_test.cpp)

target_compile_definitions(epoch_metadata_test PRIVATE
        -DGLZ_ENABLE_JSON_PRETTY_PRINT)

# Copy test case YAML files to runtime output directory
file(GLOB_RECURSE TEST_CASE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/transforms_test_cases/*.yaml)
foreach(TEST_FILE ${TEST_CASE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_FILE})
    configure_file(${TEST_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${REL_PATH} COPYONLY)
endforeach()

add_subdirectory(src)