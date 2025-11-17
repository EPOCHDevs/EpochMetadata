#pragma once

#include <epoch_script/transforms/core/metadata.h>
#include "groupby_agg.h"

namespace epoch_script::transform {

// Function to create Numeric GroupByAgg metadata
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeGroupByNumericAggMetaData() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // GroupBy Numeric Aggregation (with select option for aggregation type)
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "groupby_numeric_agg",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Group By Numeric Aggregation",
    .options = {
      MetaDataOption{
        .id = "agg",
        .name = "Aggregation Type",
        .type = epoch_core::MetaDataOptionType::Select,
        .isRequired = true,
        .selectOption = {
          {"Sum", "sum"},
          {"Mean", "mean"},
          {"Count", "count"},
          {"First", "first"},
          {"Last", "last"},
          {"Min", "min"},
          {"Max", "max"}
        },
        .desc = "Type of aggregation to perform on grouped values"
      }
    },
    .isCrossSectional = false,
    .desc = "Groups data by group_key and performs selected numeric aggregation on values. "
            "Supports sum, mean, count, first, last, min, max. "
            "Returns grouped index (selected based on aggregation - first index for 'first', "
            "otherwise uses aggregated index), group keys, and aggregated values.",
    .inputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false, false},
      {epoch_core::IODataType::Number, "value", "Value to Aggregate", false, false}
    },
    .outputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false},
      {epoch_core::IODataType::Number, "value", "Aggregated Value", false}
    },
    .atLeastOneInputRequired = true,
    .tags = {"groupby", "aggregation", "numeric", "operator"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .usageContext = "Group rows by a key column and aggregate numeric values. "
                    "For example, group by 'sector' and sum 'market_cap'. "
                    "The index of the result depends on the aggregation: "
                    "'first' uses the first occurrence index, all others use the aggregated index position.",
    .limitations = "Requires non-null group keys. All values in the value column must be numeric. "
                   "Groups are processed independently - cannot aggregate across groups."
  });

  return metadataList;
}

// Function to create Boolean GroupByAgg metadata
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeGroupByBooleanAggMetaData() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // GroupBy Boolean Aggregation (with select option for aggregation type)
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "groupby_boolean_agg",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Group By Boolean Aggregation",
    .options = {
      MetaDataOption{
        .id = "agg",
        .name = "Aggregation Type",
        .type = epoch_core::MetaDataOptionType::Select,
        .isRequired = true,
        .selectOption = {
          {"All Of (AND)", "AllOf"},
          {"Any Of (OR)", "AnyOf"},
          {"None Of (NOR)", "NoneOf"}
        },
        .desc = "Type of boolean aggregation to perform on grouped values"
      }
    },
    .isCrossSectional = false,
    .desc = "Groups data by group_key and performs selected boolean aggregation on values. "
            "Supports AllOf (all true), AnyOf (at least one true), NoneOf (all false). "
            "Returns grouped index (aggregated position), group keys, and boolean results.",
    .inputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false, false},
      {epoch_core::IODataType::Boolean, "value", "Boolean Value to Aggregate", false, false}
    },
    .outputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false},
      {epoch_core::IODataType::Boolean, "value", "Aggregated Boolean", false}
    },
    .atLeastOneInputRequired = true,
    .tags = {"groupby", "aggregation", "boolean", "operator"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .usageContext = "Group rows by a key column and aggregate boolean values using logical operations. "
                    "For example, group by 'sector' and check if all stocks have 'profitable' = true. "
                    "AllOf returns true only if all values in the group are true. "
                    "AnyOf returns true if at least one value is true. "
                    "NoneOf returns true if all values are false.",
    .limitations = "Requires non-null group keys. All values in the value column must be boolean. "
                   "NoneOf is implemented as NOT(AnyOf)."
  });

  return metadataList;
}

// Function to create Any GroupByAgg metadata (Any -> Boolean)
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeGroupByAnyAggMetaData() {
  std::vector<epoch_script::transforms::TransformsMetaData> metadataList;

  // GroupBy Any Aggregation (with select option for aggregation type)
  metadataList.emplace_back(epoch_script::transforms::TransformsMetaData{
    .id = "groupby_any_agg",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Group By Any Aggregation",
    .options = {
      MetaDataOption{
        .id = "agg",
        .name = "Aggregation Type",
        .type = epoch_core::MetaDataOptionType::Select,
        .isRequired = true,
        .selectOption = {
          {"Is Equal (all values equal)", "IsEqual"},
          {"Is Unique (all values unique)", "IsUnique"}
        },
        .desc = "Type of comparison aggregation to perform on grouped values"
      }
    },
    .isCrossSectional = false,
    .desc = "Groups data by group_key and performs comparison checks on values. "
            "Supports IsEqual (checks if all values in group are equal), "
            "IsUnique (checks if all values in group are unique). "
            "Returns grouped index (aggregated position), group keys, and boolean results.",
    .inputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false, false},
      {epoch_core::IODataType::Any, "value", "Value to Compare", false, false}
    },
    .outputs = {
      {epoch_core::IODataType::Any, "group_key", "Group Key", false},
      {epoch_core::IODataType::Boolean, "value", "Comparison Result", false}
    },
    .atLeastOneInputRequired = true,
    .tags = {"groupby", "aggregation", "comparison", "operator"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .usageContext = "Group rows by a key column and perform comparison checks on any-typed values. "
                    "For example, group by 'portfolio' and check if all holdings have the same 'rating'. "
                    "IsEqual returns true if all values in the group are identical. "
                    "IsUnique returns true if all values in the group are distinct from each other.",
    .limitations = "Requires non-null group keys. "
                   "IsEqual uses nunique() == 1 internally. "
                   "IsUnique checks if nunique() == count()."
  });

  return metadataList;
}

} // namespace epoch_script::transform
