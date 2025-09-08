//
// Created by adesola on 12/30/24.
//
#include "moving_average.h"
#include "epoch_metadata/transforms/config_helper.h"

namespace epoch_metadata::transform {
MovingAverage::MovingAverage(const TransformConfiguration &config)
    : ITransform(config),
      m_model(ma(config.GetOptionValue("type").GetSelectOption(),
                 config.GetId(), config.GetInput(),
                 config.GetOptionValue("period").GetInteger(),
                 config.GetTimeframe())) {}
} // namespace epoch_metadata::transform