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
class QuantQualEstimation : public ITransform {
public:
  explicit QuantQualEstimation(const TransformConfiguration &config)
      : ITransform(config),
        m_avg_period(config.GetOptionValue("avg_period").GetInteger()),
        m_smooth_period(config.GetOptionValue("smooth_period").GetInteger()),
        m_width_factor(config.GetOptionValue("width_factor").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::qqe_v<double, int64_t> visitor(m_avg_period, m_smooth_period, m_width_factor);

    CloseSpan closeSpan{df};

    run_visit(df, visitor, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(visitor.get_result()),
                    factory::array::make_array(visitor.get_rsi_ma()),
                    factory::array::make_array(visitor.get_long_line()),
                    factory::array::make_array(visitor.get_short_line())},
        {GetOutputId("result"), GetOutputId("rsi_ma"), GetOutputId("long_line"),
         GetOutputId("short_line")});
  }

private:
  int64_t m_avg_period;
  int64_t m_smooth_period;
  double m_width_factor;
};
} // namespace epoch_script::transform