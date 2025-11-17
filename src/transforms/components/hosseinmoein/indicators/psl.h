//
// Created by adesola on 4/16/25.
//

#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
class PSL : public ITransform {
public:
  explicit PSL(const TransformConfiguration &config)
      : ITransform(config),
        m_period(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::PSLVisitor<double, int64_t> visitor(m_period);

    const CloseSpan closeSpan{df};
    const OpenSpan openSpan{df};

    run_visit(df, visitor, closeSpan, openSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(visitor.get_result())},
        {GetOutputId("result")});
  }

private:
  int64_t m_period;
};
} // namespace epoch_script::transform