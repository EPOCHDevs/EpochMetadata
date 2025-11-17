#pragma once

#include <epoch_script/transforms/core/metadata.h>
#include "static_cast.h"

namespace epoch_script::transform {

// Function to create metadata for static_cast transforms
inline std::vector<epoch_script::transforms::TransformsMetaData> MakeStaticCastMetaData() {
  using namespace epoch_script::transforms;
  std::vector<TransformsMetaData> metadataList;

  // StaticCastToInteger - compiler-inserted Integer type materializer
  metadataList.emplace_back(TransformsMetaData{
    .id = "static_cast_to_integer",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Static Cast To Integer",
    .options = {},
    .isCrossSectional = false,
    .desc = "Internal compiler-inserted transform to materialize resolved Integer types. Not intended for direct use in scripts.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::INTEGER_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"internal", "compiler", "type-system"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {},
    .relatedTransforms = {},
    .assetRequirements = {"single-asset"},
    .usageContext = "Automatically inserted by compiler during type resolution. Not for direct use.",
    .limitations = "Internal use only. Should not appear in user-written scripts."
  });

  // StaticCastToDecimal - compiler-inserted Decimal type materializer
  metadataList.emplace_back(TransformsMetaData{
    .id = "static_cast_to_decimal",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Static Cast To Decimal",
    .options = {},
    .isCrossSectional = false,
    .desc = "Internal compiler-inserted transform to materialize resolved Decimal types. Not intended for direct use in scripts.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::NUMBER_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"internal", "compiler", "type-system"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {},
    .relatedTransforms = {},
    .assetRequirements = {"single-asset"},
    .usageContext = "Automatically inserted by compiler during type resolution. Not for direct use.",
    .limitations = "Internal use only. Should not appear in user-written scripts."
  });

  // StaticCastToBoolean - compiler-inserted Boolean type materializer
  metadataList.emplace_back(TransformsMetaData{
    .id = "static_cast_to_boolean",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Static Cast To Boolean",
    .options = {},
    .isCrossSectional = false,
    .desc = "Internal compiler-inserted transform to materialize resolved Boolean types. Not intended for direct use in scripts.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"internal", "compiler", "type-system"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {},
    .relatedTransforms = {},
    .assetRequirements = {"single-asset"},
    .usageContext = "Automatically inserted by compiler during type resolution. Not for direct use.",
    .limitations = "Internal use only. Should not appear in user-written scripts."
  });

  // StaticCastToString - compiler-inserted String type materializer
  metadataList.emplace_back(TransformsMetaData{
    .id = "static_cast_to_string",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Static Cast To String",
    .options = {},
    .isCrossSectional = false,
    .desc = "Internal compiler-inserted transform to materialize resolved String types. Not intended for direct use in scripts.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaDataConstants::STRING_OUTPUT_METADATA},
    .atLeastOneInputRequired = false,
    .tags = {"internal", "compiler", "type-system"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {},
    .relatedTransforms = {},
    .assetRequirements = {"single-asset"},
    .usageContext = "Automatically inserted by compiler during type resolution. Not for direct use.",
    .limitations = "Internal use only. Should not appear in user-written scripts."
  });

  // StaticCastToTimestamp - compiler-inserted Timestamp type materializer
  metadataList.emplace_back(TransformsMetaData{
    .id = "static_cast_to_timestamp",
    .category = epoch_core::TransformCategory::Utility,
    .name = "Static Cast To Timestamp",
    .options = {},
    .isCrossSectional = false,
    .desc = "Internal compiler-inserted transform to materialize resolved Timestamp types. Not intended for direct use in scripts.",
    .inputs = {IOMetaDataConstants::ANY_INPUT_METADATA},
    .outputs = {IOMetaData{epoch_core::IODataType::Timestamp, "result", ""}},
    .atLeastOneInputRequired = false,
    .tags = {"internal", "compiler", "type-system"},
    .requiresTimeFrame = false,
    .allowNullInputs = true,
    .strategyTypes = {},
    .relatedTransforms = {},
    .assetRequirements = {"single-asset"},
    .usageContext = "Automatically inserted by compiler during type resolution. Not for direct use.",
    .limitations = "Internal use only. Should not appear in user-written scripts."
  });

  return metadataList;
}

} // namespace epoch_script::transform
