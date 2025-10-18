# TreeSitter.cmake - Tree-sitter grammar setup using CPM
# Sets up tree-sitter with Python grammar support for EpochFlow

include_guard()

# Download CPM if not already included
if(NOT COMMAND CPMAddPackage)
    include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
endif()

# Force static linking for tree-sitter (override BUILD_SHARED_LIBS)
# This ensures portable, self-contained binaries with no .so dependencies
set(BUILD_SHARED_LIBS_BACKUP ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static tree-sitter" FORCE)

# Downloads cpp-tree-sitter wrapper and tree-sitter
# Pass CMAKE_BUILD_TYPE=Release to prevent sanitizer flags that cpp-tree-sitter adds in DEBUG mode
CPMAddPackage(
  NAME cpp-tree-sitter
  GIT_REPOSITORY https://github.com/nsumner/cpp-tree-sitter.git
  GIT_TAG v0.0.3
  OPTIONS
    "CMAKE_BUILD_TYPE Release"
)

# Downloads tree-sitter-python grammar
# Using 0.23.6 for compatibility with cpp-tree-sitter's bundled tree-sitter (language version 14)
add_grammar_from_repo(tree-sitter-python
  https://github.com/tree-sitter/tree-sitter-python.git
  0.23.6
)

# Restore original BUILD_SHARED_LIBS setting
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BACKUP} CACHE BOOL "Restored after tree-sitter" FORCE)

message(STATUS "Tree-sitter with Python grammar configured via CPM (static libraries)")