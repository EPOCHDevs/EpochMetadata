# EpochFlow AST Compiler Tests - add to main epoch_metadata_test

target_sources(epoch_metadata_test PRIVATE
    ast_compiler_test.cpp
)

# Copy test case directories to runtime output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cases DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
