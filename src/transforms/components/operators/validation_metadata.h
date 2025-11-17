#pragma once

#include <epoch_script/transforms/core/metadata.h>
#include "validation.h"

namespace epoch_script::transform {

// Function to create metadata for validation transforms
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeValidationMetaData() {
  using namespace epoch_script::transforms;
  std::vector<TransformsMetaData> metadataList;

  // IsNull
  metadataList.emplace_back(TransformsMetaData{
    .id = "is_null",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Is Null",
    .options = {},
    .isCrossSectional = false,
    .desc = "Check if values are null/missing. Returns boolean series where True indicates null values.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"validation", "null", "missing", "data-quality"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {"data-validation", "filtering", "data-cleaning"},
    .relatedTransforms = {"is_valid", "boolean_select"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Use to identify missing data points. Commonly used with boolean_select to filter or fill null values, or to create conditional logic based on data availability.",
    .limitations = "Returns boolean mask. Use with boolean_select or logical operators for actual filtering or replacement."
  });

  // IsValid
  metadataList.emplace_back(TransformsMetaData{
    .id = "is_valid",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Is Valid",
    .options = {},
    .isCrossSectional = false,
    .desc = "Check if values are valid (not null). Returns boolean series where True indicates valid values.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"validation", "null", "valid", "data-quality"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {"data-validation", "filtering", "data-cleaning"},
    .relatedTransforms = {"is_null", "boolean_select"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Opposite of is_null. Use to identify valid data points, commonly for filtering out incomplete data or creating validity-based conditions.",
    .limitations = "Returns boolean mask. Use with boolean_select or logical operators for actual filtering."
  });

  // IsZero
  metadataList.emplace_back(TransformsMetaData{
    .id = "is_zero",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Is Zero",
    .options = {},
    .isCrossSectional = false,
    .desc = "Check if values equal zero. Returns boolean series where True indicates zero values.",
    .inputs = {IOMetaDataConstants::NUMBER_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"validation", "comparison", "zero"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"signal-detection", "filtering", "threshold"},
    .relatedTransforms = {"is_one", "eq", "neq"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Quick check for zero values. Equivalent to 'eq(input, 0)' but more concise. Use for detecting zero crossings, identifying flat periods, or filtering zero values.",
    .limitations = "Only checks exact equality with zero. For near-zero checks, use comparison operators with small threshold."
  });

  // IsOne
  metadataList.emplace_back(TransformsMetaData{
    .id = "is_one",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Is One",
    .options = {},
    .isCrossSectional = false,
    .desc = "Check if values equal one. Returns boolean series where True indicates values equal to 1.",
    .inputs = {IOMetaDataConstants::NUMBER_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"validation", "comparison", "one"},
    .requiresTimeFrame = false,
    .allowNullInputs = false,
    .strategyTypes = {"signal-detection", "filtering", "threshold"},
    .relatedTransforms = {"is_zero", "eq", "neq"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Quick check for values equal to 1. Equivalent to 'eq(input, 1)' but more concise. Useful for detecting normalized peaks, validating boolean-to-number conversions, or identifying unit values.",
    .limitations = "Only checks exact equality with one. For near-one checks, use comparison operators with threshold."
  });

  return metadataList;
}

} // namespace epoch_script::transform
