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
class PSL : public ITransform {
public:
  explicit PSL(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    const CloseSpan closeSpan{df};
    const OpenSpan openSpan{df};

    run_visit(df, m_visitor, closeSpan, openSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(m_visitor.get_result())},
        {GetOutputId("result")});
  }

private:
  mutable hmdf::PSLVisitor<double, int64_t> m_visitor;
};
} // namespace epochflow::transform