#pragma once

#include "metadata.h"

namespace epochflow::transforms {

// Reusable SQL option definitions for transforms and reports
// These provide consistent SQL functionality across the codebase

/**
 * SQL Query Option (for Reports)
 *
 * SQL query to execute on the DataFrame.
 *
 * Column References:
 * - Data columns: SLOT0, SLOT1, SLOT2, ... (positional, based on SLOT order)
 * - Index column: timestamp (if add_index=true)
 * - Table name: self (always, not configurable)
 *
 * Example: "SELECT timestamp, SLOT0 as price, SLOT1 as volume FROM self WHERE SLOT0 > 100"
 */
inline const epochflow::MetaDataOption SQL_OPTION{
    .id = "sql",
    .name = "SQL Query",
    .type = epoch_core::MetaDataOptionType::SqlStatement,
    .defaultValue = std::nullopt,
    .isRequired = true,
    .desc = "SQL query to execute. Reference columns as SLOT0, SLOT1, etc. 'timestamp' available if add_index=true. Always use 'FROM self'."
};

/**
 * Timeseries SQL Query Option (for SQLQueryTransform)
 *
 * SQL query to execute on timeseries data.
 *
 * Column References:
 * - Data columns: SLOT0, SLOT1, SLOT2, ... (positional, based on SLOT order)
 * - Index column: timestamp (ALWAYS available - index is always added)
 * - Table name: self (always, not configurable)
 * - Outputs: RESULT0, RESULT1, RESULT2, RESULT3 (for multi-output transforms)
 *
 * IMPORTANT: Must SELECT timestamp in output for timeseries continuity
 *
 * Example: "SELECT timestamp, SLOT0 as RESULT0, SLOT0 - LAG(SLOT0) OVER (ORDER BY timestamp) as RESULT1 FROM self"
 */
inline const epochflow::MetaDataOption TIMESERIES_SQL_OPTION{
    .id = "sql",
    .name = "Timeseries SQL Query",
    .type = epoch_core::MetaDataOptionType::SqlStatement,
    .defaultValue = std::nullopt,
    .isRequired = true,
    .desc = "SQL query to execute on timeseries data. Reference columns as SLOT0, SLOT1, etc. 'timestamp' is ALWAYS available. Must SELECT timestamp in output. Always use 'FROM self'. For multi-output transforms, columns must be named RESULT0, RESULT1, etc."
};

/**
 * Add Index Option
 *
 * If true, the DataFrame index is added as a column named 'timestamp' that can be
 * referenced in SQL queries. This is useful for time-series operations.
 *
 * When false, the index is not accessible in SQL.
 */
inline const epochflow::MetaDataOption ADD_INDEX_OPTION{
    .id = "add_index",
    .name = "Add Index as Timestamp",
    .type = epoch_core::MetaDataOptionType::Boolean,
    .defaultValue = epochflow::MetaDataOptionDefinition{false},
    .isRequired = false,
    .desc = "If true, DataFrame index is added as column named 'timestamp' accessible in SQL"
};

} // namespace epochflow::transforms
