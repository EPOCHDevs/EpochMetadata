cmake_minimum_required(VERSION 3.26 FATAL_ERROR) # 3.26 For now until XGBOOST Removes FindCUDA

########################################### INCLUDE RAPIDS CMAKE FILES #################################################
set(EPOCH_LAB "$ENV{HOME}/EpochLab"  CACHE PATH "EpochLab FOLDER")
include(${EPOCH_LAB}/shared_header.cmake)
########################################################################################################################

project(metadata LANGUAGES C CXX)

rapids_cpm_init()
include(${EPOCH_CMAKE}/include_test.cmake)
include(${EPOCH_CMAKE}/get_tulip.cmake)
include(${EPOCH_CMAKE}/get_glaze.cmake)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_subdirectory(src)

target_compile_definitions(metadata PUBLIC -DMETADATA_FILES_LOC="${FILES_DIR}/metadata")
MESSAGE("METADATA_FILES_LOC ${FILES_DIR}/metadata")
