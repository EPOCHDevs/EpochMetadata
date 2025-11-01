target_sources(epoch_flow_test PRIVATE hmm_test.cpp)

target_compile_definitions(epoch_flow_test PRIVATE
    -DHMM_TEST_DATA_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hmm_test_data")

file(COPY "${CMAKE_CURRENT_LIST_DIR}/hmm_python_gen/hmm_test_data/"
       DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hmm_test_data"
       FILES_MATCHING PATTERN "*.csv")