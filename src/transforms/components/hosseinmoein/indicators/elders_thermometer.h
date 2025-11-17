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
class EldersThermometer : public ITransform {
public:
  explicit EldersThermometer(const TransformConfiguration &config)
      : ITransform(config),
        m_period(config.GetOptionValue("period").GetInteger()),
        m_buy_factor(config.GetOptionValue("buy_factor").GetDecimal()),
        m_sell_factor(config.GetOptionValue("sell_factor").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::ether_v<double, int64_t> visitor(m_period, m_buy_factor, m_sell_factor);

    const HighSpan highSpan{df};
    const LowSpan lowSpan{df};

    run_visit(df, visitor, lowSpan, highSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(visitor.get_result()),
                    factory::array::make_array(visitor.get_result_ma()),
                    factory::array::make_array(visitor.get_buy_signal()),
                    factory::array::make_array(visitor.get_sell_signal())},
        {GetOutputId("result"), GetOutputId("ema"), GetOutputId("buy_signal"),
         GetOutputId("sell_signal")});
  }

private:
  int64_t m_period;
  double m_buy_factor;
  double m_sell_factor;
};
} // namespace epoch_script::transform