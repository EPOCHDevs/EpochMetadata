#pragma once

#include <epoch_script/transforms/core/metadata.h>
#include <epoch_script/transforms/core/sql_options.h>
#include "sql_query_transform.h"

namespace epoch_script::transform {

// Function to create SQL Query metadata for all variants
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeSQLQueryMetaData() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // SQLQueryTransform1 - Single output
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "sql_query_1",
    .category = epoch_core::TransformCategory::Utility,
    .name = "SQL Query (1 Output)",
    .options = {
      epoch_script::transforms::TIMESERIES_SQL_OPTION
    },
    .isCrossSectional = false,
    .desc = "Execute SQL queries on timeseries data. "
            "Single output variant - returns entire query result as one DataFrame. "
            "Result will be indexed by the specified index_column_name for timeseries merging.",
    .inputs = {{epoch_core::IODataType::Any, epoch_script::ARG, "", true}},
    .outputs = {
        {epoch_core::IODataType::Any, "RESULT0", "Output 0", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "single-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "filtering", "feature-engineering", "custom-logic"},
    .relatedTransforms = {"sql_query_2", "sql_query_3", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Powerful SQL interface for custom data transformations. IMPORTANT: Always use 'FROM self' as the table name. Input columns are SLOT0, SLOT1, SLOT2, etc. Use for: complex filtering (WHERE clauses), calculated columns (SELECT expressions), aggregations (GROUP BY), joins between multiple inputs, and custom feature engineering. Entire result returned as single output. Must include timestamp in SELECT for timeseries continuity. Supports full DuckDB SQL syntax including window functions.",
    .limitations = "Requires SQL knowledge. CRITICAL: Must use 'FROM self' as the table name (the DataFrame is registered as 'self' in DuckDB). Input columns are SLOT0, SLOT1, etc. Query errors only caught at runtime. Column '#' characters sanitized to '_'. Must explicitly SELECT index column (timestamp) for proper timeseries merging. Performance depends on query complexity. Single output only - use sql_query_2/3/4 for multiple outputs. No validation of output data types."
  });

  // SQLQueryTransform2 - Two outputs
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "sql_query_2",
    .category = epoch_core::TransformCategory::Utility,
    .name = "SQL Query (2 Outputs)",
    .options = {
      epoch_script::transforms::TIMESERIES_SQL_OPTION
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 2 separate output ports. "
            "SQL result MUST contain exactly these columns: RESULT0, RESULT1, and index_column_name. "
            "Each output port carries its respective result column with the index.",
    .inputs = {{epoch_core::IODataType::Any, epoch_script::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "RESULT0", "Output 0", true},
      {epoch_core::IODataType::Any, "RESULT1", "Output 1", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "signal-splitting", "custom-logic"},
    .relatedTransforms = {"sql_query_1", "sql_query_3", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 2 separate output ports for parallel processing. IMPORTANT: Always use 'FROM self' as the table name. Input columns are SLOT0, SLOT1, etc. Query MUST use aliases: SELECT calc1 AS RESULT0, calc2 AS RESULT1, timestamp FROM self. Each output routes to different downstream nodes. Use for: splitting calculated features, separating signal components (e.g., trend + noise), or creating complementary indicators from same calculation.",
    .limitations = "CRITICAL: Must use 'FROM self' as the table name. Must alias outputs as 'RESULT0' and 'RESULT1' exactly. Column count strictly enforced (RESULT0, RESULT1, index_column). Runtime errors if column names wrong. Both outputs share same index/timestamps. Cannot have different row counts per output. Use sql_query_1 if only need single combined result."
  });

  // SQLQueryTransform3 - Three outputs
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "sql_query_3",
    .category = epoch_core::TransformCategory::Utility,
    .name = "SQL Query (3 Outputs)",
    .options = {
      epoch_script::transforms::TIMESERIES_SQL_OPTION
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 3 separate output ports. "
            "SQL result MUST contain exactly: RESULT0, RESULT1, RESULT2, and index_column_name. "
            "Each output port carries its own data series with the index.",
    .inputs = {{epoch_core::IODataType::Any, epoch_script::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "RESULT0", "Output 0", true},
      {epoch_core::IODataType::Any, "RESULT1", "Output 1", true},
      {epoch_core::IODataType::Any, "RESULT2", "Output 2", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "multi-signal-generation", "custom-logic"},
    .relatedTransforms = {"sql_query_1", "sql_query_2", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 3 separate output ports. IMPORTANT: Always use 'FROM self' as the table name. Input columns are SLOT0, SLOT1, etc. Query must alias: SELECT calc1 AS RESULT0, calc2 AS RESULT1, calc3 AS RESULT2, timestamp FROM self. Use for: decomposing data into components (e.g., trend/cycle/noise), generating multiple related signals, or creating Bollinger-style bands (upper/middle/lower). Each output independently routable.",
    .limitations = "CRITICAL: Must use 'FROM self' as the table name. Must alias outputs as 'RESULT0', 'RESULT1', 'RESULT2' exactly. Strict column count (3 outputs + index). All outputs share same index/timestamps. Cannot have different row counts per output. More outputs = more complex query maintenance. Consider if sql_query_1 with downstream splits clearer."
  });

  // SQLQueryTransform4 - Four outputs
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "sql_query_4",
    .category = epoch_core::TransformCategory::Utility,
    .name = "SQL Query (4 Outputs)",
    .options = {
      epoch_script::transforms::TIMESERIES_SQL_OPTION
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 4 separate output ports. "
            "SQL result MUST contain exactly: RESULT0, RESULT1, RESULT2, RESULT3, and index_column_name. "
            "Each output port is a separate data stream for connecting to different downstream nodes.",
    .inputs = {{epoch_core::IODataType::Any, epoch_script::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "RESULT0", "Output 0", true},
      {epoch_core::IODataType::Any, "RESULT1", "Output 1", true},
      {epoch_core::IODataType::Any, "RESULT2", "Output 2", true},
      {epoch_core::IODataType::Any, "RESULT3", "Output 3", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "ohlc-reconstruction", "multi-signal-generation"},
    .relatedTransforms = {"sql_query_1", "sql_query_2", "sql_query_3"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 4 output ports - maximum multi-output variant. IMPORTANT: Always use 'FROM self' as the table name. Input columns are SLOT0, SLOT1, etc. Perfect for OHLC reconstruction or 4-component decomposition. Query must alias: SELECT val1 AS RESULT0, val2 AS RESULT1, val3 AS RESULT2, val4 AS RESULT3, timestamp FROM self. Use for: OHLC price streams, multi-factor models, quartile bands, or 4-regime signals. Each output independently routable to different logic branches.",
    .limitations = "CRITICAL: Must use 'FROM self' as the table name. Must alias outputs as 'RESULT0' through 'RESULT3' exactly. Strict column count (4 outputs + index). All outputs share same index/timestamps - no independent row filtering per output. Most complex SQL query variant - high maintenance overhead. Consider if sql_query_1 with downstream processing more maintainable. Need 5+ outputs? Chain multiple SQL queries."
  });

  return metadataList;
}

} // namespace epoch_script::transform