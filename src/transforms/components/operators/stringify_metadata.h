#pragma once

#include <epoch_script/transforms/core/metadata.h>
#include "stringify.h"

namespace epoch_script::transform {

// Function to create metadata for stringify transform
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeStringifyMetaData() {
  using namespace epoch_script::transforms;
  std::vector<TransformsMetaData> metadataList;

  // Stringify (str)
  metadataList.emplace_back(TransformsMetaData{
    .id = "stringify",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Stringify",
    .options = {},
    .isCrossSectional = false,
    .desc = "Convert any value to its string representation. Behaves like Python's str() function. Boolean: true→'true', false→'false'. Numbers: converted to decimal representation. Strings: pass-through unchanged.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::STRING_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"conversion", "string", "type-cast", "formatting"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {"utility", "data-formatting", "reporting"},
    .relatedTransforms = {"boolean_select_string", "static_cast_to_string"},
    .assetRequirements = {"single-asset"},
    .usageContext = "Use to convert any type to string for display, concatenation, or string-based operations. Commonly used with boolean_select_string when you need to convert boolean flags to text, or for formatting numeric values as strings.",
    .limitations = "Conversion is one-way (cannot convert back to original type). For null values, produces null strings."
  });

  return metadataList;
}

} // namespace epoch_script::transform
