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
class PivotPointSR : public ITransform {
public:
  explicit PivotPointSR(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const final {
    // Create local visitor to avoid state accumulation across assets
    hmdf::PivotPointSRVisitor<double, int64_t> visitor;

    LowSpan lowSpan{df};
    HighSpan highSpan{df};
    CloseSpan closeSpan{df};

    // Use run_visit for consistency with other visitors
    run_visit(df, visitor, lowSpan, highSpan, closeSpan);

    // Return all 7 vectors from the visitor
    return make_dataframe(
        df.index(),
        std::vector{visitor.get_result(), visitor.get_resist_1(),
                    visitor.get_support_1(), visitor.get_resist_2(),
                    visitor.get_support_2(), visitor.get_resist_3(),
                    visitor.get_support_3()},
        {GetOutputId("pivot"), GetOutputId("resist_1"),
         GetOutputId("support_1"), GetOutputId("resist_2"),
         GetOutputId("support_2"), GetOutputId("resist_3"),
         GetOutputId("support_3")});
  }
};
} // namespace epoch_script::transform