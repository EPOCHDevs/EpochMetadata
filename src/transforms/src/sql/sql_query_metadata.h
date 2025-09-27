#pragma once

#include "epoch_metadata/transforms/metadata.h"
#include "sql_query_transform.h"

namespace epoch_metadata::transform {

// Metadata for SQLQueryTransform1 (single output)
inline TransformsMetaData CreateSQLQueryTransform1Metadata(const std::string& name = "sql_query_1") {
  return {
    .id = name,
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (1 Output)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::Text,
       .defaultValue = MetaDataOptionDefinition{"SELECT * FROM input0"},
       .isRequired = true,
       .desc = "SQL query to execute. Reference input DataFrames by their input IDs or as input0, input1, etc."}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query on input DataFrames producing a single output table. "
            "Supports full DuckDB SQL syntax including JOINs, aggregations, window functions, etc.",
    .inputs = {},  // Variadic inputs handled by GetInputIds()
    .outputs = {IOMetaDataConstants::ANY_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,  // Can execute SQL without inputs
    .tags = {"sql", "query", "table", "transform"},
    .requiresTimeFrame = false,
    .allowNullInputs = true
  };
}

// Metadata for SQLQueryTransform2 (two outputs)
inline TransformsMetaData CreateSQLQueryTransform2Metadata(const std::string& name = "sql_query_2") {
  return {
    .id = name,
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (2 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::Text,
       .defaultValue = MetaDataOptionDefinition{"SELECT * FROM input0"},
       .isRequired = true,
       .desc = "SQL query to execute. The result will be split into two output tables."},
      {.id = "output0_columns",
       .name = "Output 0 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the first output table"},
      {.id = "output1_columns",
       .name = "Output 1 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the second output table"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query and split results into 2 separate output tables. "
            "Useful for creating multiple derived datasets from a single query.",
    .inputs = {},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output Table 0", true},
      {epoch_core::IODataType::Any, "output1", "Output Table 1", true}
    },
    .atLeastOneInputRequired = false,
    .tags = {"sql", "query", "table", "transform", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = true
  };
}

// Metadata for SQLQueryTransform3 (three outputs)
inline TransformsMetaData CreateSQLQueryTransform3Metadata(const std::string& name = "sql_query_3") {
  return {
    .id = name,
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (3 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::Text,
       .defaultValue = MetaDataOptionDefinition{"SELECT * FROM input0"},
       .isRequired = true,
       .desc = "SQL query to execute. The result will be split into three output tables."},
      {.id = "output0_columns",
       .name = "Output 0 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the first output table"},
      {.id = "output1_columns",
       .name = "Output 1 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the second output table"},
      {.id = "output2_columns",
       .name = "Output 2 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the third output table"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query and split results into 3 separate output tables. "
            "Ideal for complex data transformations requiring multiple derived outputs.",
    .inputs = {},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output Table 0", true},
      {epoch_core::IODataType::Any, "output1", "Output Table 1", true},
      {epoch_core::IODataType::Any, "output2", "Output Table 2", true}
    },
    .atLeastOneInputRequired = false,
    .tags = {"sql", "query", "table", "transform", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = true
  };
}

// Metadata for SQLQueryTransform4 (four outputs)
inline TransformsMetaData CreateSQLQueryTransform4Metadata(const std::string& name = "sql_query_4") {
  return {
    .id = name,
    .category = epoch_core::TransformCategory::Utility,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "SQL Query (4 Outputs)",
    .options = {
      {.id = "sql",
       .name = "SQL Query",
       .type = epoch_core::MetaDataOptionType::Text,
       .defaultValue = MetaDataOptionDefinition{"SELECT * FROM input0"},
       .isRequired = true,
       .desc = "SQL query to execute. The result will be split into four output tables."},
      {.id = "output0_columns",
       .name = "Output 0 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the first output table"},
      {.id = "output1_columns",
       .name = "Output 1 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the second output table"},
      {.id = "output2_columns",
       .name = "Output 2 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the third output table"},
      {.id = "output3_columns",
       .name = "Output 3 Columns",
       .type = epoch_core::MetaDataOptionType::List,
       .defaultValue = MetaDataOptionDefinition{},
       .isRequired = true,
       .desc = "Column names for the fourth output table"}
    },
    .isCrossSectional = false,
    .desc = "Execute SQL query and split results into 4 separate output tables. "
            "Maximum flexibility for complex multi-output transformations.",
    .inputs = {},
    .outputs = {
      {epoch_core::IODataType::Any, "output0", "Output Table 0", true},
      {epoch_core::IODataType::Any, "output1", "Output Table 1", true},
      {epoch_core::IODataType::Any, "output2", "Output Table 2", true},
      {epoch_core::IODataType::Any, "output3", "Output Table 3", true}
    },
    .atLeastOneInputRequired = false,
    .tags = {"sql", "query", "table", "transform", "multi-output"},
    .requiresTimeFrame = false,
    .allowNullInputs = true
  };
}

} // namespace epoch_metadata::transform