//
// Created by adesola on 11/16/25.
//

#pragma once
#include <epoch_script/strategy/enums.h>

namespace epoch_script::data {
    struct FuturesContinuationInput {
        epoch_core::RolloverType rollover{epoch_core::RolloverType::Null};
        epoch_core::AdjustmentType type{epoch_core::AdjustmentType::Null};
        int arg = 0;
    };
}