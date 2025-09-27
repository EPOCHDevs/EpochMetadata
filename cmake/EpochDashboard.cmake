# EpochCore.cmake
#
# This is a helper file to include EpochCore
include(FetchContent)
    
set(EPOCH_DASHBOARD_REPOSITORY "${REPO_URL}/EPOCHDevs/EpochDashboard.git" CACHE STRING "EpochDashboard repository URL")
set(EPOCH_DASHBOARD_TAG "main" CACHE STRING "EpochDashboard Git tag to use")

FetchContent_Declare(
        EpochDashboard
    GIT_REPOSITORY ${EPOCH_DASHBOARD_REPOSITORY}
    GIT_TAG ${EPOCH_DASHBOARD_TAG}
    SOURCE_SUBDIR cpp
)

FetchContent_MakeAvailable(EpochDashboard)

message(STATUS "EpochDashboard fetched and built from source")