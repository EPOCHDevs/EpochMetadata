#pragma once

#include "epoch_metadata/transforms/metadata.h"

namespace epoch_metadata::transforms {

// Reusable SQL option definitions for transforms and reports
// These provide consistent SQL functionality across the codebase

/**
 * SQL Query Option (for Reports)
 *
 * SQL query to execute on the DataFrame.
 *
 * Column References:
 * - Data columns: input0, input1, input2, ... (positional, based on SLOT order)
 * - Index column: timestamp (if add_index=true)
 * - Table name: input (always, not configurable)
 *
 * Example: "SELECT timestamp, input0 as price, input1 as volume FROM input WHERE input0 > 100"
 */
inline const epoch_metadata::MetaDataOption SQL_OPTION{
    .id = "sql",
    .name = "SQL Query",
    .type = epoch_core::MetaDataOptionType::String,
    .defaultValue = std::nullopt,
    .isRequired = true,
    .desc = "SQL query to execute. Reference columns as input0, input1, etc. 'timestamp' available if add_index=true. Always use 'FROM input'."
};

/**
 * Timeseries SQL Query Option (for SQLQueryTransform)
 *
 * SQL query to execute on timeseries data.
 *
 * Column References:
 * - Data columns: input0, input1, input2, ... (positional, based on SLOT order)
 * - Index column: timestamp (ALWAYS available - index is always added)
 * - Table name: input (always, not configurable)
 * - Outputs: output0, output1, output2, output3 (for multi-output transforms)
 *
 * IMPORTANT: Must SELECT timestamp in output for timeseries continuity
 *
 * Example: "SELECT timestamp, input0 as output0, input0 - LAG(input0) OVER (ORDER BY timestamp) as output1 FROM input"
 */
inline const epoch_metadata::MetaDataOption TIMESERIES_SQL_OPTION{
    .id = "sql",
    .name = "Timeseries SQL Query",
    .type = epoch_core::MetaDataOptionType::String,
    .defaultValue = std::nullopt,
    .isRequired = true,
    .desc = "SQL query to execute on timeseries data. Reference columns as input0, input1, etc. 'timestamp' is ALWAYS available. Must SELECT timestamp in output. Always use 'FROM input'."
};

/**
 * Add Index Option
 *
 * If true, the DataFrame index is added as a column named 'timestamp' that can be
 * referenced in SQL queries. This is useful for time-series operations.
 *
 * When false, the index is not accessible in SQL.
 */
inline const epoch_metadata::MetaDataOption ADD_INDEX_OPTION{
    .id = "add_index",
    .name = "Add Index as Timestamp",
    .type = epoch_core::MetaDataOptionType::Boolean,
    .defaultValue = epoch_metadata::MetaDataOptionDefinition{false},
    .isRequired = false,
    .desc = "If true, DataFrame index is added as column named 'timestamp' accessible in SQL"
};

} // namespace epoch_metadata::transforms
