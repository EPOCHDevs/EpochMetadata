target_sources(epoch_metadata_test PRIVATE hmm_test.cpp)

  file(COPY "${CMAKE_CURRENT_LIST_DIR}/hmm_python_gen/hmm_test_data/" 
         DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hmm_test_data"
         FILES_MATCHING PATTERN "*.csv")