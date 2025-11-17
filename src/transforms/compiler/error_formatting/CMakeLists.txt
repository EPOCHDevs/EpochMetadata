# Error Formatting sources
target_sources(epoch_script PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/error_formatter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/argument_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/type_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/component_error.cpp
)
