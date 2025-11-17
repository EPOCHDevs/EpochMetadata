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
class AccelerationBands : public ITransform {
public:
  explicit AccelerationBands(const TransformConfiguration &config)
      : ITransform(config),
        m_period(config.GetOptionValue("period").GetInteger()),
        m_multiplier(config.GetOptionValue("multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    CloseSpan closeSpan{df};
    HighSpan highSpan{df};
    LowSpan lowSpan{df};

    // Create fresh local visitor to avoid state accumulation across assets
    hmdf::aband_v<double, int64_t> visitor(m_period, m_multiplier);
    run_visit(df, visitor, closeSpan, highSpan, lowSpan);

    return make_dataframe(
        df.index(),
        std::vector{visitor.get_upper_band(), visitor.get_result(),
                    visitor.get_lower_band()},
        {GetOutputId("upper_band"), GetOutputId("middle_band"),
         GetOutputId("lower_band")});
  }

private:
  int64_t m_period;
  double m_multiplier;
};
} // namespace epoch_script::transform