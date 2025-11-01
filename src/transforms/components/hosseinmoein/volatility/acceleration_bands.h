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
class AccelerationBands : public ITransform {
public:
  explicit AccelerationBands(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("period").GetInteger(),
                  config.GetOptionValue("multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    CloseSpan closeSpan{df};
    HighSpan highSpan{df};
    LowSpan lowSpan{df};

    run_visit(df, m_visitor, closeSpan, highSpan, lowSpan);

    return make_dataframe(
        df.index(),
        std::vector{m_visitor.get_upper_band(), m_visitor.get_result(),
                    m_visitor.get_lower_band()},
        {GetOutputId("upper_band"), GetOutputId("middle_band"),
         GetOutputId("lower_band")});
  }

private:
  mutable hmdf::aband_v<double, int64_t> m_visitor;
};
} // namespace epochflow::transform