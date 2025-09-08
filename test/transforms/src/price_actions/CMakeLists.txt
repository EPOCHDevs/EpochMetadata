target_sources(epoch_metadata_test PRIVATE 
    smc_test.cpp
)

# Create directory if it doesn't exist    
# Copy test data directory to runtime bin
file(COPY "${CMAKE_CURRENT_LIST_DIR}/test_data" 
        DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        PATTERN "*")
        
message(STATUS "Copied test data to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data")
