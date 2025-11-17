add_subdirectory(data_explorer)

# EpochScript Compile Check Tool
add_executable(epoch_compile_check epoch_compile_check.cpp)

target_include_directories(epoch_compile_check PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(epoch_compile_check
    PRIVATE
        epoch::script
)

# Install the executable
install(TARGETS epoch_compile_check
    RUNTIME DESTINATION bin
)

target_compile_definitions(epoch_compile_check PUBLIC
        -DMETADATA_FILES_DIR="${PROJECT_SOURCE_DIR}/files")

set_target_properties(epoch_compile_check PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")