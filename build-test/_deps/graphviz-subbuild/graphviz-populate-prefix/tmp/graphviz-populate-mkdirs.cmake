# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-src")
  file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-src")
endif()
file(MAKE_DIRECTORY
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-build"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/tmp"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/src/graphviz-populate-stamp"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/src"
  "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/src/graphviz-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/src/graphviz-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/adesola/EpochLab/EpochMetadata/build-test/_deps/graphviz-subbuild/graphviz-populate-prefix/src/graphviz-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
