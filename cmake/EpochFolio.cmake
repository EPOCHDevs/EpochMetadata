# EpochCore.cmake
#
# This is a helper file to include EpochCore
include(FetchContent)
    
set(EPOCH_FOLIO_REPOSITORY "https://github.com/EPOCHDevs/EpochFolio.git" CACHE STRING "EpochFolio repository URL")
set(EPOCH_FOLIO_TAG "master" CACHE STRING "EpochFolio Git tag to use")

FetchContent_Declare(
    EpochFolio
    GIT_REPOSITORY ${EPOCH_FOLIO_REPOSITORY}
    GIT_TAG ${EPOCH_FOLIO_TAG}
)

FetchContent_MakeAvailable(EpochFolio)

message(STATUS "EpochFolio fetched and built from source")