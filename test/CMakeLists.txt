add_executable(epoch_metadata_test catch_main.cpp)
target_link_libraries(epoch_metadata_test PRIVATE epoch_metadata Catch2::Catch2)
target_include_directories(epoch_metadata_test PRIVATE ../src )

add_subdirectory(strategy)
add_subdirectory(transforms)

set(METADATA_FILES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files)
if (NOT EXISTS "${METADATA_FILES_DIR}" OR NOT IS_DIRECTORY "${METADATA_FILES_DIR}")
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/files DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

target_compile_definitions(epoch_metadata_test PUBLIC -DMETADATA_FILES_DIR="${METADATA_FILES_DIR}")
MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")
