target_sources(epoch_metadata_test PRIVATE
        registrations_test.cpp
        transforms_metadata_test.cpp)

target_compile_definitions(epoch_metadata_test PRIVATE
        -DGLZ_ENABLE_JSON_PRETTY_PRINT)


add_subdirectory(src)