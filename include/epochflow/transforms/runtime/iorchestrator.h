#pragma once
//
// Created by dewe on 1/26/23.
//
#include <epochflow/transforms/runtime/types.h>
#include <epochflow/transforms/components/selectors/card_selector.h>
#include <epochflow/transforms/core/itransform.h>
#include "epoch_protos/tearsheet.pb.h"

namespace epoch_flow::runtime {
    struct IDataFlowOrchestrator {
        using Ptr = std::unique_ptr<IDataFlowOrchestrator>;

        virtual TimeFrameAssetDataFrameMap
        ExecutePipeline(TimeFrameAssetDataFrameMap data) = 0;

        virtual AssetReportMap GetGeneratedReports() const = 0;

        virtual AssetSelectorMap GetGeneratedSelectors() const = 0;

        virtual ~IDataFlowOrchestrator() = default;
    };
} // namespace epoch_flow::runtime