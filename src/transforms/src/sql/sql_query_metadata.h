#pragma once

#include "epoch_metadata/transforms/metadata.h"
#include "sql_query_transform.h"

namespace epoch_metadata::transform {

// Function to create SQL Query metadata for all variants
inline std::vector<epoch_metadata::transforms::TransformsMetaData> MakeSQLQueryMetaData() {
  std::vector<epoch_metadata::transforms::TransformsMetaData> metadataList;

  // SQLQueryTransform1 - Single output
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
    .id = "sql_query_1",
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (1 Output)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::String,
       .isRequired = true,
       .desc = "SQL query to execute on timeseries data. "
               "Column names containing '#' will be automatically sanitized to '_' for SQL compatibility. "
               "IMPORTANT: Include the index_column_name (default: timestamp) in your SELECT for timeseries merging. "
               "Example: SELECT timestamp, close, volume FROM input WHERE close > 100"},
      {.id = "table_name",
       .name = "Table Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
       .isRequired = false,
       .desc = "Name to reference the input DataFrame in your SQL query. Default is 'input'. "
               "Use this name in your FROM clause (e.g., FROM input)"},
      {.id = "index_column_name",
       .name = "Index Column Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
       .isRequired = false,
       .desc = "Name of the column to use as the output DataFrame index for timeseries data. "
               "This column will be set as the index after SQL execution to enable proper "
               "merging with other timeseries. Default is 'timestamp'. Must be included in SQL SELECT."}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL queries on timeseries data. "
            "Single output variant - returns entire query result as one DataFrame. "
            "Result will be indexed by the specified index_column_name for timeseries merging.",
    .inputs = {{epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}},
    .outputs = {
        {epoch_core::IODataType::Any, "output0", "Output 0", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "single-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "filtering", "feature-engineering", "custom-logic"},
    .relatedTransforms = {"sql_query_2", "sql_query_3", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Powerful SQL interface for custom data transformations. Use for: complex filtering (WHERE clauses), calculated columns (SELECT expressions), aggregations (GROUP BY), joins between multiple inputs, and custom feature engineering. Entire result returned as single output. Must include timestamp in SELECT for timeseries continuity. Supports full DuckDB SQL syntax including window functions.",
    .limitations = "Requires SQL knowledge. Query errors only caught at runtime. Column '#' characters sanitized to '_'. Must explicitly SELECT index column (timestamp) for proper timeseries merging. Performance depends on query complexity. Single output only - use sql_query_2/3/4 for multiple outputs. No validation of output data types."
  });

  // SQLQueryTransform2 - Two outputs
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
    .id = "sql_query_2",
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (2 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::String,
       .isRequired = true,
       .desc = "SQL query that returns exactly: output0, output1, and index_column_name. "
               "Example: SELECT price AS output0, volume AS output1, timestamp FROM input"},
      {.id = "table_name",
       .name = "Table Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
       .isRequired = false,
       .desc = "Name to reference the input DataFrame in SQL query"},
      {.id = "index_column_name",
       .name = "Index Column Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
       .isRequired = false,
       .desc = "Column name to set as output index for timeseries continuity"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 2 separate output ports. "
            "SQL result MUST contain exactly these columns: output0, output1, and index_column_name. "
            "Each output port carries its respective result column with the index.",
    .inputs = {{epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output 0", true},
      {epoch_core::IODataType::Any, "output1", "Output 1", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "signal-splitting", "custom-logic"},
    .relatedTransforms = {"sql_query_1", "sql_query_3", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 2 separate output ports for parallel processing. Query MUST use aliases: SELECT calc1 AS output0, calc2 AS output1, timestamp FROM input. Each output routes to different downstream nodes. Use for: splitting calculated features, separating signal components (e.g., trend + noise), or creating complementary indicators from same calculation.",
    .limitations = "Must alias outputs as 'output0' and 'output1' exactly. Column count strictly enforced (output0, output1, index_column). Runtime errors if column names wrong. Both outputs share same index/timestamps. Cannot have different row counts per output. Use sql_query_1 if only need single combined result."
  });

  // SQLQueryTransform3 - Three outputs
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
    .id = "sql_query_3",
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (3 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::String,
       .isRequired = true,
       .desc = "SQL query that returns exactly: output0, output1, output2, and index_column_name. "
               "Example: SELECT open AS output0, high AS output1, low AS output2, timestamp FROM input"},
      {.id = "table_name",
       .name = "Table Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
       .isRequired = false,
       .desc = "Name to reference the input DataFrame in SQL query"},
      {.id = "index_column_name",
       .name = "Index Column Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
       .isRequired = false,
       .desc = "Column name to set as output index for timeseries continuity"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 3 separate output ports. "
            "SQL result MUST contain exactly: output0, output1, output2, and index_column_name. "
            "Each output port carries its own data series with the index.",
    .inputs = {{epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output 0", true},
      {epoch_core::IODataType::Any, "output1", "Output 1", true},
      {epoch_core::IODataType::Any, "output2", "Output 2", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "multi-signal-generation", "custom-logic"},
    .relatedTransforms = {"sql_query_1", "sql_query_2", "sql_query_4"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 3 separate output ports. Query must alias: SELECT calc1 AS output0, calc2 AS output1, calc3 AS output2, timestamp FROM input. Use for: decomposing data into components (e.g., trend/cycle/noise), generating multiple related signals, or creating Bollinger-style bands (upper/middle/lower). Each output independently routable.",
    .limitations = "Must alias outputs as 'output0', 'output1', 'output2' exactly. Strict column count (3 outputs + index). All outputs share same index/timestamps. Cannot have different row counts per output. More outputs = more complex query maintenance. Consider if sql_query_1 with downstream splits clearer."
  });

  // SQLQueryTransform4 - Four outputs
  metadataList.emplace_back(epoch_metadata::transforms::TransformsMetaData{
    .id = "sql_query_4",
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (4 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::String,
       .isRequired = true,
       .desc = "SQL query that returns exactly: output0, output1, output2, output3, and index_column_name. "
               "Example: SELECT open AS output0, high AS output1, low AS output2, close AS output3, timestamp FROM input"},
      {.id = "table_name",
       .name = "Table Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
       .isRequired = false,
       .desc = "Name to reference the input DataFrame in SQL query"},
      {.id = "index_column_name",
       .name = "Index Column Name",
       .type = epoch_core::MetaDataOptionType::String,
       .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
       .isRequired = false,
       .desc = "Column name to set as output index for timeseries continuity"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query producing 4 separate output ports. "
            "SQL result MUST contain exactly: output0, output1, output2, output3, and index_column_name. "
            "Each output port is a separate data stream for connecting to different downstream nodes.",
    .inputs = {{epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output 0", true},
      {epoch_core::IODataType::Any, "output1", "Output 1", true},
      {epoch_core::IODataType::Any, "output2", "Output 2", true},
      {epoch_core::IODataType::Any, "output3", "Output 3", true}
    },
    .atLeastOneInputRequired = true,
    .tags = {"sql", "query", "transform", "timeseries", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"data-transformation", "feature-engineering", "ohlc-reconstruction", "multi-signal-generation"},
    .relatedTransforms = {"sql_query_1", "sql_query_2", "sql_query_3"},
    .assetRequirements = {"single-asset"},
    .usageContext = "SQL with 4 output ports - maximum multi-output variant. Perfect for OHLC reconstruction or 4-component decomposition. Query must alias: SELECT val1 AS output0, val2 AS output1, val3 AS output2, val4 AS output3, timestamp FROM input. Use for: OHLC price streams, multi-factor models, quartile bands, or 4-regime signals. Each output independently routable to different logic branches.",
    .limitations = "Must alias outputs as 'output0' through 'output3' exactly. Strict column count (4 outputs + index). All outputs share same index/timestamps - no independent row filtering per output. Most complex SQL query variant - high maintenance overhead. Consider if sql_query_1 with downstream processing more maintainable. Need 5+ outputs? Chain multiple SQL queries."
  });

  return metadataList;
}

} // namespace epoch_metadata::transform