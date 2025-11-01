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
class ChandeKrollStop : public ITransform {
public:
  explicit ChandeKrollStop(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("p_period").GetInteger(),
                  config.GetOptionValue("q_period").GetInteger(),
                  config.GetOptionValue("multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    HighSpan highSpan{df};
    LowSpan lowSpan{df};
    CloseSpan closeSpan{df};

    run_visit(df, m_visitor, lowSpan, highSpan, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{m_visitor.get_long_stop(), m_visitor.get_short_stop()},
        {GetOutputId("long_stop"), GetOutputId("short_stop")});
  }

private:
  mutable hmdf::cksp_v<double, int64_t> m_visitor;
};
} // namespace epoch_script::transform