//
// Created by adesola on 4/16/25.
//

#pragma once
#include "../common_utils.h"
#include <epochflow/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epochflow::transform {
using p_vol_v = hmdf::ParkinsonVolVisitor<double, int64_t>;
class Parkinson : public SingleResultHMDFTransform<p_vol_v, LowSpan, HighSpan> {
public:
  explicit Parkinson(const TransformConfiguration &config)
      : SingleResultHMDFTransform(
            config,
            p_vol_v(config.GetOptionValue("period").GetInteger(),
                    config.GetOptionValue("trading_periods").GetInteger())) {}
};
} // namespace epochflow::transform