add_library(metadata STATIC metadata_options.cpp)
target_include_directories(metadata PUBLIC .. PRIVATE . ${EPOCH_LAB_COMMON_HEADERS})

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_options(metadata PRIVATE -Wall -Wextra -Werror)
endif ()

set(dependencies spdlog::spdlog_header_only
        fmt::fmt-header-only
        yaml-cpp::yaml-cpp
        tulipindicators
        glaze::glaze)

target_link_libraries(metadata PRIVATE ${dependencies})

INCLUDE_TEST()
#add_subdirectory(dlb)
add_subdirectory(strategy)
#add_subdirectory(ml_kit)
add_subdirectory(transforms)
