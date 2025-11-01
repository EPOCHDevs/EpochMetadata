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
class EldersThermometer : public ITransform {
public:
  explicit EldersThermometer(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("period").GetInteger(),
                  config.GetOptionValue("buy_factor").GetDecimal(),
                  config.GetOptionValue("sell_factor").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    const HighSpan highSpan{df};
    const LowSpan lowSpan{df};

    run_visit(df, m_visitor, lowSpan, highSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(m_visitor.get_result()),
                    factory::array::make_array(m_visitor.get_result_ma()),
                    factory::array::make_array(m_visitor.get_buy_signal()),
                    factory::array::make_array(m_visitor.get_sell_signal())},
        {GetOutputId("result"), GetOutputId("ema"), GetOutputId("buy_signal"),
         GetOutputId("sell_signal")});
  }

private:
  mutable hmdf::ether_v<double, int64_t> m_visitor;
};
} // namespace epochflow::transform