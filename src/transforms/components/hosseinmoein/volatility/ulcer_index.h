#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
using u_idx_v = hmdf::UlcerIndexVisitor<double, int64_t>;
class UlcerIndex : public SingleResultHMDFTransform<u_idx_v, CloseSpan> {
public:
  explicit UlcerIndex(const TransformConfiguration &config)
      : SingleResultHMDFTransform(
            config, u_idx_v(config.GetOptionValue("period").GetInteger(),
                            config.GetOptionValue("use_sum").GetBoolean())) {}
};
} // namespace epoch_script::transform
