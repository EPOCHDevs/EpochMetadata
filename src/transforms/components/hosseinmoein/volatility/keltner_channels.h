#pragma once
#include "../common_utils.h"
#include <epoch_script/transforms/core/itransform.h>
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_frame;

namespace epoch_script::transform {
using keltner_channels_v = hmdf::KeltnerChannelsVisitor<double, int64_t>;
class KeltnerChannels : public ITransform {
public:
  explicit KeltnerChannels(const TransformConfiguration &config)
      : ITransform(config),
        m_roll_period(config.GetOptionValue("roll_period").GetInteger()),
        m_band_multiplier(config.GetOptionValue("band_multiplier").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    keltner_channels_v visitor(m_roll_period, m_band_multiplier);

    LowSpan lowSpan{df};
    HighSpan highSpan{df};
    CloseSpan closeSpan{df};
    run_visit(df, visitor, lowSpan, highSpan, closeSpan);
    return make_dataframe(
        df.index(),
        std::vector{visitor.get_upper_band(), visitor.get_lower_band()},
        {GetOutputId("upper_band"), GetOutputId("lower_band")});
  }

private:
  int64_t m_roll_period;
  double m_band_multiplier;
};
} // namespace epoch_script::transform
