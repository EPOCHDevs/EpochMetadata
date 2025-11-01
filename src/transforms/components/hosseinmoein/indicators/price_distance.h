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
class PriceDistance : public ITransform {
public:
  explicit PriceDistance(const TransformConfiguration &config)
      : ITransform(config), m_visitor() {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    CloseSpan closeSpan{df};
    HighSpan highSpan{df};
    LowSpan lowSpan{df};
    OpenSpan openSpan{df};

    run_visit(df, m_visitor, lowSpan, highSpan, openSpan, closeSpan);
    return make_dataframe<double>(df.index(), {m_visitor.get_result()},
                                  std::vector{GetOutputId()});
  }

private:
  mutable hmdf::pdist_v<double, int64_t> m_visitor;
};
} // namespace epoch_script::transform