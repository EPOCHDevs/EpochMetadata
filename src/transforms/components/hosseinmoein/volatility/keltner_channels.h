#pragma once
#include "../common_utils.h"
#include <epochflow/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epochflow::transform {
using keltner_channels_v = hmdf::KeltnerChannelsVisitor<double, int64_t>;
class KeltnerChannels : public ITransform {
public:
  explicit KeltnerChannels(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("roll_period").GetInteger(),
                  config.GetOptionValue("band_multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    LowSpan lowSpan{df};
    HighSpan highSpan{df};
    CloseSpan closeSpan{df};
    run_visit(df, m_visitor, lowSpan, highSpan, closeSpan);
    return make_dataframe(
        df.index(),
        std::vector{m_visitor.get_upper_band(), m_visitor.get_lower_band()},
        {GetOutputId("upper_band"), GetOutputId("lower_band")});
  }

private:
  mutable keltner_channels_v m_visitor;
};
} // namespace epochflow::transform
