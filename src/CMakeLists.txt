target_sources(epoch_metadata PRIVATE bar_attribute.cpp metadata_options.cpp sql_statement.cpp time_frame.cpp asset/symbol.cpp)

add_subdirectory(reports)
add_subdirectory(strategy)
# TODO: Asset module has StratifyX dependencies - disabled for now
# add_subdirectory(model)
add_subdirectory(transforms)
add_subdirectory(compiler)
add_subdirectory(runtime)
