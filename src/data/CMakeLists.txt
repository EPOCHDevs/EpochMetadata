# EpochScript Data Subsystem
# Minimal copy from StratifyX - only Database, Futures, Factory, WebSocket
# Uses epoch::data_sdk for ALL dataloader infrastructure (no duplication)

# Add data sources directly to epoch_script to avoid circular dependency
target_sources(epoch_script PRIVATE
    # Factory
    ${CMAKE_CURRENT_SOURCE_DIR}/factory.cpp

    # Model (calendar support)
    ${CMAKE_CURRENT_SOURCE_DIR}/model/exchange_calendar.cpp

    # Database
    ${CMAKE_CURRENT_SOURCE_DIR}/database/database_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/database/database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/database/resample.cpp

    # WebSocket
    ${CMAKE_CURRENT_SOURCE_DIR}/database/updates/alpaca_websocket_manager.cpp

    # Futures continuation
    ${CMAKE_CURRENT_SOURCE_DIR}/futures_continuation/continuations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/futures_continuation/constructed_bars.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/futures_continuation/adjustments/adjustment_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/futures_continuation/roll_method/roll_method_impl.cpp
)

# Add compile definitions for data subsystem
target_compile_definitions(epoch_script PUBLIC
    -DEPOCH_DB_S3="s3://epoch-db"
    -DDEFAULT_DATABASE_PATH="s3://epoch-db"
)
