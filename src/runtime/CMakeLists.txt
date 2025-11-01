# EpochFlow Runtime - Transform Orchestrator and Execution Engine
# Manages dataflow graph execution using TBB

# Add runtime sources to parent target (epoch_metadata)
target_sources(epoch_metadata PRIVATE
    orchestrator.cpp
    execution/execution_node.cpp
    execution/intermediate_storage.cpp
    transform_manager/transform_manager.cpp
)

# Runtime needs access to execution headers
target_include_directories(epoch_metadata PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
