add_executable(epoch_metadata_test catch_main.cpp)
target_link_libraries(epoch_metadata_test PRIVATE epoch_metadata Catch2::Catch2)
target_include_directories(epoch_metadata_test PRIVATE ../src )

add_subdirectory(strategy)
add_subdirectory(transforms)