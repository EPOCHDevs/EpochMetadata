# EpochFlow - Python Algorithm Compiler for StratifyX
# Compiles Python-like algorithm syntax to nodes/edges graphs

# Add sources to parent target (epoch_metadata)
target_sources(epoch_metadata PRIVATE
    parser/python_parser.cpp
    compiler/ast_compiler.cpp
    compiler/timeframe_resolver.cpp
)

# Link tree-sitter and Python grammar
target_link_libraries(epoch_metadata PUBLIC cpp-tree-sitter tree-sitter-python)

# Include current directory for epochflow headers
target_include_directories(epoch_metadata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
