add_executable(metadataTest metadata_test.cpp)
target_link_libraries(metadataTest PRIVATE metadata common Catch2::Catch2 Catch2::Catch2WithMain)
target_include_directories(metadataTest PRIVATE ..)

include(CTest)

include(Catch)
catch_discover_tests(metadataTest)