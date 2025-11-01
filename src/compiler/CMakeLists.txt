# EpochFlow - Python Algorithm Compiler for StratifyX
# Compiles Python-like algorithm syntax to nodes/edges graphs

# Add sources to parent target (epoch_metadata)
target_sources(epoch_metadata PRIVATE
    parser/python_parser.cpp
    compiler/timeframe_resolver.cpp
    # Refactored compiler components
    compiler/type_checker.cpp
    compiler/option_validator.cpp
    compiler/special_parameter_handler.cpp
    compiler/constructor_parser.cpp
    compiler/expression_compiler.cpp
    compiler/node_builder.cpp
    compiler/ast_visitor.cpp
    compiler/ast_compiler.cpp
    compiler/constant_folder.cpp
    # Special node validators
    compiler/validators/special_node_validator.cpp
    compiler/validators/first_non_null_validator.cpp
    compiler/validators/conditional_select_validator.cpp
)

# Link tree-sitter and Python grammar
target_link_libraries(epoch_metadata PUBLIC cpp-tree-sitter tree-sitter-python)

# Include current directory for epochflow headers
target_include_directories(epoch_metadata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
