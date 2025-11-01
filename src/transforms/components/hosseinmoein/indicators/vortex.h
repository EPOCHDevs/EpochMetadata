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
class Vortex : public ITransform {
public:
  explicit Vortex(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    HighSpan highSpan{df};
    LowSpan lowSpan{df};
    CloseSpan closeSpan{df};

    run_visit(df, m_visitor, lowSpan, highSpan, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{m_visitor.get_plus_indicator(),
                    m_visitor.get_minus_indicator()},
        {GetOutputId("plus_indicator"), GetOutputId("minus_indicator")});
  }

private:
  mutable hmdf::vtx_v<double, int64_t> m_visitor;
};
} // namespace epoch_script::transform