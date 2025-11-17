# EpochScript Data Subsystem
# Minimal copy from StratifyX - only Database, Futures, Factory, WebSocket
# Uses epoch::data_sdk for ALL dataloader infrastructure (no duplication)

# Add data sources directly to epoch_script to avoid circular dependency
target_sources(epoch_script PRIVATE
    # Factory
    factory.cpp
)

add_subdirectory(database)
add_subdirectory(futures_continuation)
add_subdirectory(model)
# Add compile definitions for data subsystem
target_compile_definitions(epoch_script PUBLIC
    -DEPOCH_DB_S3="s3://epoch-db"
    -DDEFAULT_DATABASE_PATH="s3://epoch-db"
)
