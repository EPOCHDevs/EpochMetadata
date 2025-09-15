cmake_minimum_required(VERSION 3.27 FATAL_ERROR) # 3.26 For now until XGBOOST Removes FindCUDA

########################################### INCLUDE RAPIDS CMAKE FILES #################################################
# Add option for shared vs static libraries (default to shared for development)
option(BUILD_SHARED_LIBS "Build libraries as shared instead of static" ON)
message(STATUS "Building SHARED libraries as ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "Building SHARED libraries")
else()
    message(STATUS "Building STATIC libraries")
endif()

# Set binary and library output directories to keep build files separate from source
set(PROJECT_VERSION 0.1.0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set PIC only when building shared libraries
if(BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
endif()

# Enable vcpkg manifest mode (uses vcpkg.json)
set(DEPENDENCY_DIR "/usr/local" CACHE STRING "Dependency directory")
set(CMAKE_TOOLCHAIN_FILE $ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_FEATURE_FLAGS "versions")

option(BUILD_TEST OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
########################################################################################################################

project(EpochMetadata LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (BUILD_TEST)
    # Find Catch2 for both tests and benchmarks
    find_package(Catch2 3 REQUIRED)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
    enable_testing()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/EpochFrame.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Tulip.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Graphviz.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/MPDecimal.cmake)

find_package(glaze REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(DataFrame CONFIG REQUIRED)
find_package(Armadillo CONFIG REQUIRED)


add_library(epoch_metadata "")
add_library(epoch::metadata ALIAS epoch_metadata)

# Configure version for shared libraries
set_target_properties(epoch_metadata PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# For shared libraries, export all symbols by default (Linux-friendly approach)
if(BUILD_SHARED_LIBS)
    set_target_properties(epoch_metadata PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN OFF
    )
endif()

target_compile_definitions(epoch_metadata PUBLIC -DSPDLOG_USE_STD_FORMAT)

target_include_directories(epoch_metadata
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_options(epoch_metadata PRIVATE -Wall -Wextra -Werror)
target_link_libraries(epoch_metadata PUBLIC
    armadillo
        DataFrame::DataFrame
       epoch::frame
       spdlog::spdlog_header_only
       yaml-cpp::yaml-cpp
       libmpdec++
       libmpdec
       tulip_indicators
       glaze::glaze
        cgraph
        gvc
        gvplugin_core
        gvplugin_dot_layout)

add_subdirectory(src)

# Add testing framework as a separate module
add_subdirectory(testing_framework)

if (BUILD_TEST)
    add_subdirectory(test)
endif()