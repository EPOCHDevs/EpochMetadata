target_sources(epoch_metadata_test PRIVATE
transform_metadata_factory.cpp
transforms_test.cpp trade_executor_test.cpp)


add_subdirectory(cummulative)
add_subdirectory(cross_sectional)
add_subdirectory(futures)
add_subdirectory(hosseinmoein)
add_subdirectory(indicators)
add_subdirectory(operators)
add_subdirectory(price_actions)
add_subdirectory(tulip)
add_subdirectory(volatility)
add_subdirectory(statistics)    


target_compile_definitions(epoch_metadata_test PUBLIC -DMETADATA_FILES_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/files")
MESSAGE("METADATA_FILES_DIR ${METADATA_FILES_DIR}")