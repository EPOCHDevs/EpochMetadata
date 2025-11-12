//
// Created by adesola on 12/18/24.
//

#pragma once

// Use epoch_data_sdk's glaze_custom_types (EpochScript depends on epoch_data_sdk)
#include <epoch_data_sdk/common/glaze_custom_types.hpp>

namespace epoch_script {
// Symbol is defined in epoch_script, but glaze serialization is in data_sdk
// The glaze specializations in data_sdk work with both data_sdk::Symbol and epoch_script::Symbol
// since they're structurally compatible
}
