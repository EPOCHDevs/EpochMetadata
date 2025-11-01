#pragma once
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/dataframe.h>
#include <string>
#include <tbb/concurrent_unordered_map.h>
#include <unordered_map>

namespace epoch_flow::runtime {
using TransformType = epoch_metadata::transform::ITransform::Ptr;
using AssetID = std::string;
using AssetDataFrameMap = std::unordered_map<AssetID, epoch_frame::DataFrame>;
using TimeFrameAssetDataFrameMap =
    std::unordered_map<std::string, AssetDataFrameMap>;

// Store complete DataFrames per transform, not individual series
using TransformCache =
    std::unordered_map<std::string,
                       epoch_frame::Series>; // outputId -> Series

// Asset-level cache (using AssetID = std::string)
using AssetCache = std::unordered_map<AssetID, TransformCache>;

// Timeframe-level cache
using TimeFrameCache = tbb::concurrent_unordered_map<std::string, AssetCache>;

} // namespace epoch_flow::runtime
