# Data Explorer Tool
add_executable(data_explorer data_explorer.cpp)

target_include_directories(data_explorer PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(data_explorer
    PRIVATE
        epoch::script
)

# Install the executable
install(TARGETS data_explorer
    RUNTIME DESTINATION bin
)

target_compile_definitions(data_explorer PUBLIC
        -DMETADATA_FILES_DIR="${PROJECT_SOURCE_DIR}/files")