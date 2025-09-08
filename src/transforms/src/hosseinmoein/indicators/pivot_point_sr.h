//
// Created by adesola on 4/16/25.
//

#pragma once
#include "../common_utils.h"
#include "epoch_metadata/transforms/itransform.h"
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_metadata::transform {
class PivotPointSR : public ITransform {
public:
  explicit PivotPointSR(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    LowSpan lowSpan{df};
    HighSpan highSpan{df};
    CloseSpan closeSpan{df};

    // Use run_visit for consistency with other visitors
    run_visit(df, m_visitor, lowSpan, highSpan, closeSpan);

    // Return all 7 vectors from the visitor
    return make_dataframe(
        df.index(),
        std::vector{m_visitor.get_result(), m_visitor.get_resist_1(),
                    m_visitor.get_support_1(), m_visitor.get_resist_2(),
                    m_visitor.get_support_2(), m_visitor.get_resist_3(),
                    m_visitor.get_support_3()},
        {GetOutputId("pivot"), GetOutputId("resist_1"),
         GetOutputId("support_1"), GetOutputId("resist_2"),
         GetOutputId("support_2"), GetOutputId("resist_3"),
         GetOutputId("support_3")});
  }

private:
  mutable hmdf::PivotPointSRVisitor<double, int64_t> m_visitor;
};
} // namespace epoch_metadata::transform