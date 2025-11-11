cmake_minimum_required(VERSION 3.26 FATAL_ERROR) # 3.26 For now until XGBOOST Removes FindCUDA

# Add option for shared vs static libraries (default to shared for development)
set(GIT_TOKEN "" CACHE STRING "Default GIT TOKEN")
option(BUILD_SHARED_LIBS "Build libraries as shared instead of static" ON)
message(STATUS "Building SHARED libraries as ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "Building SHARED libraries")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message(STATUS "Building STATIC libraries")
    set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
endif()

# Set binary and library output directories to keep build files separate from source
set(PROJECT_VERSION 0.1.0)
set(PROJECT_VERSION_MAJOR 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Option to specify where script metadata files (transforms.yaml, etc.) should be copied
# Defaults to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files but can be overridden
set(SCRIPT_METADATA_FILES_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files" CACHE PATH
    "Directory where script metadata files (transforms.yaml, commission.yaml, etc.) will be copied")
message(STATUS "Script metadata files will be copied to: ${SCRIPT_METADATA_FILES_DIR}")

# Enable vcpkg manifest mode (uses vcpkg.json)
set(DEPENDENCY_DIR "/usr/local" CACHE STRING "Dependency directory")
set(CMAKE_TOOLCHAIN_FILE $ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_FEATURE_FLAGS "versions")

option(BUILD_TEST OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
########################################################################################################################

project(EpochScript LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (GIT_TOKEN)
    SET(REPO_URL "https://${GIT_TOKEN}:@github.com")
    message("REPO URL: https://************:@github.com")
else ()
    SET(REPO_URL "https://github.com")
    message("REPO URL: ${REPO_URL}")
endif ()


if (BUILD_TEST)
    # Find Catch2 for both tests and benchmarks
    find_package(Catch2 3 REQUIRED)
    find_package(trompeloeil CONFIG REQUIRED)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
    enable_testing()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/EpochDashboard.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Tulip.cmake)
# Tree-sitter setup via CPM
include(${PROJECT_SOURCE_DIR}/cmake/TreeSitter.cmake)

find_package(yaml-cpp CONFIG REQUIRED)
find_package(DataFrame CONFIG REQUIRED)
find_package(Armadillo CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

add_library(epoch_script "")
add_library(epoch::script ALIAS epoch_script)

# Configure version for shared libraries
set_target_properties(epoch_script PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# For shared libraries, export all symbols by default (Linux-friendly approach)
if(BUILD_SHARED_LIBS)
    set_target_properties(epoch_script PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN OFF
    )
endif()

target_include_directories(epoch_script
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_options(epoch_script PRIVATE -Wall -Wextra -Werror)
target_link_libraries(epoch_script PUBLIC
        cpp-tree-sitter
        tree-sitter-python
    armadillo
        DataFrame::DataFrame
       epoch::dashboard
       yaml-cpp::yaml-cpp
       libmpdec++
       libmpdec
       tulip_indicators
       TBB::tbb)


add_subdirectory(src)

# Copy test files and define METADATA_FILES_DIR early
# Use the configurable SCRIPT_METADATA_FILES_DIR option from main CMakeLists.txt
set(METADATA_FILES_DIR ${SCRIPT_METADATA_FILES_DIR})

# Ensure the target directory exists and copy files
file(MAKE_DIRECTORY ${METADATA_FILES_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/files DESTINATION ${METADATA_FILES_DIR})

if (BUILD_TEST)
    add_subdirectory(test)
    add_subdirectory(benchmark)
endif()

# Utility: Dump transform metadata to JSON for documentation verification
add_executable(dump_transform_metadata scripts/dump_transform_metadata.cpp)
target_link_libraries(dump_transform_metadata PRIVATE epoch::script)
target_include_directories(dump_transform_metadata PRIVATE ${PROJECT_SOURCE_DIR}/include)