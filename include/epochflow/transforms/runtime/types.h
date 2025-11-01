#pragma once
#include <epoch_frame/dataframe.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <epochflow/transforms/core/itransform.h>
#include "epoch_protos/tearsheet.pb.h"

namespace epoch_flow::runtime {
    using AssetID = std::string;
    using AssetDataFrameMap = std::unordered_map<AssetID, epoch_frame::DataFrame>;
    using TimeFrameAssetDataFrameMap = std::unordered_map<std::string, AssetDataFrameMap>;

    using AssetReportMap = std::unordered_map<AssetID, epoch_proto::TearSheet>;
    using AssetSelectorMap = std::unordered_map<AssetID, std::vector<epochflow::transform::SelectorData>>;
}
