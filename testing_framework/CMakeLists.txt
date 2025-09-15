# Create the testing framework library
add_library(epoch_testing_framework STATIC
    src/transform_tester_base.cpp
    src/dataframe_tester.cpp
)

# Set include directories
target_include_directories(epoch_testing_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies (yaml-cpp is already found in parent)
target_link_libraries(epoch_testing_framework
    PUBLIC
        yaml-cpp::yaml-cpp
)