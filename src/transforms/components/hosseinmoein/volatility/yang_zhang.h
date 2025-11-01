#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
using yz_vol_v_t = hmdf::yz_vol_v<double, int64_t>;
class YangZhang
    : public SingleResultHMDFTransform<yz_vol_v_t, LowSpan, HighSpan, OpenSpan,
                                       CloseSpan> {
public:
  explicit YangZhang(const TransformConfiguration &config)
      : SingleResultHMDFTransform(
            config,
            yz_vol_v_t(config.GetOptionValue("period").GetInteger(),
                       config.GetOptionValue("trading_periods").GetInteger())) {
  }
};
} // namespace epoch_script::transform
