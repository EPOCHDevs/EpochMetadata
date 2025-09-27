# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-src")
  file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-src")
endif()
file(MAKE_DIRECTORY
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-build"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/tmp"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/src/epochcore-populate-stamp"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/src"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/src/epochcore-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/src/epochcore-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/epochcore-subbuild/epochcore-populate-prefix/src/epochcore-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
