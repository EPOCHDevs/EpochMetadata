#pragma once
//
// Created by dewe on 1/14/23.
//
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/common/enums.hpp>
#include <epoch_core/macros.h>
#include <epoch_frame/dataframe.h>

// Namespace aliases for epoch_data_sdk types
namespace epoch_script {
namespace asset = data_sdk::asset;

// Use epoch_data_sdk's enums
using data_sdk::DataCategory;
using data_sdk::DataCategoryWrapper;
using data_sdk::BenchmarkKind;
using data_sdk::BenchmarkKindWrapper;

// Use epoch_data_sdk's helper functions
using data_sdk::IsTimeSeriesCategory;
using data_sdk::IsIntraday;
using data_sdk::IsDaily;
using data_sdk::IsAuxiliaryCategory;
}

namespace epoch_script {

inline std::filesystem::path operator/(std::filesystem::path const &os,
                                       DataCategory const &type) {
  return os / DataCategoryWrapper::ToString(type);
}

using AssetDataMap = asset::AssetHashMap<
    std::unordered_map<std::string, std::vector<epoch_frame::Scalar>>>;

using AssetDataFrameMap = asset::AssetHashMap<epoch_frame::DataFrame>;
using AssetDataFrameListMap =
    asset::AssetHashMap<std::vector<epoch_frame::DataFrame>>;
using AssetStringDataFrameMap = asset::AssetHashMap<
    std::unordered_map<std::string, epoch_frame::DataFrame>>;
using StringAssetDataFrameMap =
    std::unordered_map<std::string, AssetDataFrameMap>;
using StringAssetDataFrameListMap =
    std::unordered_map<std::string, AssetDataFrameListMap>;

using IndexRange = std::pair<int64_t, int64_t>;
using TimeFrameNotation = std::string;

using TransformedDataType =
    std::unordered_map<TimeFrameNotation,
                       asset::AssetHashMap<epoch_frame::DataFrame>>;

using DatabaseIndexerValue = std::unordered_map<int64_t, IndexRange>;

struct DatabaseIndexerItem {
  TimeFrameNotation timeframe;
  asset::Asset asset;
  DatabaseIndexerValue indexer;
};
using DatabaseIndexer = std::vector<std::unique_ptr<DatabaseIndexerItem>>;

// Inverted timestamp index for O(1) lookup
// Maps timestamp -> list of (timeframe, asset, index_range) tuples
struct TimestampIndexEntry {
  TimeFrameNotation timeframe;
  asset::Asset asset;
  IndexRange range;
};
using TimestampIndex = std::unordered_map<int64_t, std::vector<TimestampIndexEntry>>;
} // namespace epoch_script