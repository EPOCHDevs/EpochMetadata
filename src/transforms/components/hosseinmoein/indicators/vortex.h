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
class Vortex : public ITransform {
public:
  explicit Vortex(const TransformConfiguration &config)
      : ITransform(config),
        m_period(config.GetOptionValue("period").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::vtx_v<double, int64_t> visitor(m_period);

    HighSpan highSpan{df};
    LowSpan lowSpan{df};
    CloseSpan closeSpan{df};

    run_visit(df, visitor, lowSpan, highSpan, closeSpan);

    return make_dataframe(
        df.index(),
        std::vector{visitor.get_plus_indicator(),
                    visitor.get_minus_indicator()},
        {GetOutputId("plus_indicator"), GetOutputId("minus_indicator")});
  }

private:
  int64_t m_period;
};
} // namespace epoch_script::transform