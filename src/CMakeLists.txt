target_sources(epoch_metadata PRIVATE bar_attribute.cpp metadata_options.cpp sql_statement.cpp time_frame.cpp asset/symbol.cpp)

#add_subdirectory(dlb)
add_subdirectory(reports)
add_subdirectory(strategy)
#add_subdirectory(ml_kit)
add_subdirectory(transforms)
add_subdirectory(epochflow)
