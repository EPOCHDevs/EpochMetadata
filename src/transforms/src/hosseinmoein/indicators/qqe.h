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
class QuantQualEstimation : public ITransform {
public:
  explicit QuantQualEstimation(const TransformConfiguration &config)
      : ITransform(config),
        m_visitor(config.GetOptionValue("avg_period").GetInteger(),
                  config.GetOptionValue("smooth_period").GetInteger(),
                  config.GetOptionValue("width_factor").GetDecimal()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    CloseSpan closeSpan{df};

    run_visit(df, m_visitor, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{factory::array::make_array(m_visitor.get_result()),
                    factory::array::make_array(m_visitor.get_rsi_ma()),
                    factory::array::make_array(m_visitor.get_long_line()),
                    factory::array::make_array(m_visitor.get_short_line())},
        {GetOutputId("result"), GetOutputId("rsi_ma"), GetOutputId("long_line"),
         GetOutputId("short_line")});
  }

private:
  mutable hmdf::qqe_v<double, int64_t> m_visitor;
};
} // namespace epoch_metadata::transform