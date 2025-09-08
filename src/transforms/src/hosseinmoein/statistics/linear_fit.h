//
// Created by adesola on 5/16/25.
//

#pragma once
#include "../common_utils.h"
#include "epoch_frame/common.h"
#include "epoch_frame/dataframe.h"
#include "epoch_frame/datetime.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include "epoch_frame/scalar.h"
#include "epoch_metadata/transforms/itransform.h"
#include <DataFrame/DataFrameFinancialVisitors.h>

#include <vector>

namespace epoch_metadata::transform {

// Rolling linear regression fit (y on x) using HMDF LinearFit visitor
// Inputs: x, y
// Options: window
// Outputs: fitted (back-of-window), residual (y - fitted)
class LinearFit final : public ITransform {
public:
  explicit LinearFit(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;

    const Series x = df[GetInputId("x")];
    const Series y = df[GetInputId("y")];
    std::vector<double> residuals, intercepts;

    std::vector<DateTime> index;
    residuals.reserve(df.num_rows());
    intercepts.reserve(df.num_rows());

    const Series fitted =
        x.rolling_apply({.window_size = m_window}).apply([&](const Series &xw) {
          const Series yw = y.loc(xw.index());
          hmdf::linfit_v<double, int64_t> visitor;
          const SeriesSpan<> xs{xw};
          const SeriesSpan<> ys{yw};
          run_visit(xw, visitor, xs, ys);
          index.push_back(xw.index()->at(-1).to_datetime());
          residuals.push_back(visitor.get_residual());
          intercepts.push_back(visitor.get_intercept());
          return Scalar{visitor.get_slope()};
        });

    auto residualInterceptDF =
        make_dataframe(factory::index::make_datetime_index(index),
                       {factory::array::make_array(residuals),
                        factory::array::make_array(intercepts)},
                       {GetOutputId("residual"), GetOutputId("intercept")});

    return epoch_frame::concat(ConcatOptions{
        .frames = {residualInterceptDF, fitted.to_frame(GetOutputId("slope"))},
        .joinType = epoch_frame::JoinType::Outer,
        .axis = epoch_frame::AxisType::Column});
  }

private:
  int64_t m_window;
};

} // namespace epoch_metadata::transform
