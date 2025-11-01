#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
using hodges_tompkins_vol_v = hmdf::HodgesTompkinsVolVisitor<double, int64_t>;
class HodgesTompkins
    : public SingleResultHMDFTransform<hodges_tompkins_vol_v, CloseSpan> {
public:
  explicit HodgesTompkins(const TransformConfiguration &config)
      : SingleResultHMDFTransform(
            config,
            hodges_tompkins_vol_v(
                config.GetOptionValue("period").GetInteger(),
                config.GetOptionValue("trading_periods").GetInteger())) {}
};
} // namespace epoch_script::transform
