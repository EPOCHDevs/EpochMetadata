# Shared test utilities - header only library
# These utilities are used by both unit and integration tests

# Add all header files and source files to the test executable
target_sources(epoch_script_test PRIVATE
    json_transform_tester.hpp
    yaml_transform_tester.hpp
    event_marker_output.hpp
    test_constants.cpp
    test_constants.h
)

# Make utilities available to other test modules
target_include_directories(epoch_script_test PRIVATE .)
