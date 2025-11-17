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
        m_p_period(config.GetOptionValue("p_period").GetInteger()),
        m_q_period(config.GetOptionValue("q_period").GetInteger()),
        m_multiplier(config.GetOptionValue("multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::cksp_v<double, int64_t> visitor(m_p_period, m_q_period, m_multiplier);

    HighSpan highSpan{df};
    LowSpan lowSpan{df};
    CloseSpan closeSpan{df};

    run_visit(df, visitor, lowSpan, highSpan, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{visitor.get_long_stop(), visitor.get_short_stop()},
        {GetOutputId("long_stop"), GetOutputId("short_stop")});
  }

private:
  int64_t m_p_period;
  int64_t m_q_period;
  double m_multiplier;
};
} // namespace epoch_script::transform