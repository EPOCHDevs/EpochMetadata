# Data Explorer Tool
add_executable(data_explorer data_explorer.cpp)

target_link_libraries(data_explorer
    PRIVATE
        epoch::script
)

# Install the executable
install(TARGETS data_explorer
    RUNTIME DESTINATION bin
)
