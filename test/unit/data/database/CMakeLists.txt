# Database Unit Tests

# Add database test sources to the main test executable
target_sources(epoch_script_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/database_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/database_impl_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resampler_test.cpp
)

# Add WebSocket test subdirectory
add_subdirectory(updates)

# Copy resample test data files to build directory
SET(RESAMPLE_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resample_test_files)
file(COPY ${RESAMPLE_TEST_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
