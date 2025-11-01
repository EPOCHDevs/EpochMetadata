#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
using gk_vol_v_t = hmdf::gk_vol_v<double, int64_t>;
class GarmanKlass
    : public SingleResultHMDFTransform<gk_vol_v_t, LowSpan, HighSpan, OpenSpan,
                                       CloseSpan> {
public:
  explicit GarmanKlass(const TransformConfiguration &config)
      : SingleResultHMDFTransform(
            config,
            gk_vol_v_t(config.GetOptionValue("period").GetInteger(),
                       config.GetOptionValue("trading_days").GetInteger())) {}
};
} // namespace epoch_script::transform