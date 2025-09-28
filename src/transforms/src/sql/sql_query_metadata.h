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
    .allowNullInputs = false
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
    .allowNullInputs = false
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
    .allowNullInputs = false
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
    .allowNullInputs = false
  });

  return metadataList;
}

} // namespace epoch_metadata::transform