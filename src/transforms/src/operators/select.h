//
// Created by dewe on 9/21/24.
//

#pragma once
#include "epoch_frame/array.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_metadata/transforms/itransform.h"

namespace epoch_metadata::transform {
class BooleanSelectTransform : public ITransform {
public:
  explicit BooleanSelectTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;
};

template <size_t N> class ZeroIndexSelectTransform : public ITransform {
public:
  explicit ZeroIndexSelectTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;
};

extern template class ZeroIndexSelectTransform<2>;

extern template class ZeroIndexSelectTransform<3>;

extern template class ZeroIndexSelectTransform<4>;

extern template class ZeroIndexSelectTransform<5>;

using Select2 = ZeroIndexSelectTransform<2>;
using Select3 = ZeroIndexSelectTransform<3>;
using Select4 = ZeroIndexSelectTransform<4>;
using Select5 = ZeroIndexSelectTransform<5>;

// Advanced Selection classes
class PercentileSelect : public ITransform {
public:
  explicit PercentileSelect(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("lookback").GetInteger()),
        m_percentile(config.GetOptionValue("percentile").GetInteger()) {
    AssertFromStream(m_lookback > 0, "Lookback must be greater than 0");
    AssertFromStream(m_percentile >= 0 && m_percentile <= 100,
                     "Percentile must be between 0 and 100");
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series value = bars[GetInputId("value")];
    epoch_frame::Array high_output =
        bars[GetInputId("high")].contiguous_array();
    epoch_frame::Array low_output = bars[GetInputId("low")].contiguous_array();

    // Calculate the rolling percentile
    epoch_frame::Array percentile_value =
        value.rolling_agg({.window_size = m_lookback})
            .quantile(m_percentile / 100.0)
            .contiguous_array();

    return epoch_frame::make_dataframe(
        bars.index(),
        {high_output
             .where(value.contiguous_array() >= percentile_value, low_output)
             .as_chunked_array()},
        {GetOutputId()});
  }

private:
  int m_lookback;
  double m_percentile;
};

// BooleanBranch takes a single boolean input and splits data into two outputs
class BooleanBranch : public ITransform {
public:
  explicit BooleanBranch(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    using namespace epoch_frame;

    // Get the condition and value series
    Series truth = bars[GetInputId()];
    Series false_mask = !truth;

    return make_dataframe(bars.index(), {truth.array(), false_mask.array()},
                          {GetOutputId("true"), GetOutputId("false")});
  }
};
;

// RatioBranch outputs signals based on the ratio between two values
class RatioBranch : public ITransform {
public:
  explicit RatioBranch(const TransformConfiguration &config)
      : ITransform(config) {
    // Get thresholds for ratio comparison
    m_threshold_high =
        config.GetOptionValue("threshold_high").GetNumericValue();
    m_threshold_low = config.GetOptionValue("threshold_low").GetNumericValue();
    AssertFromStream(m_threshold_high > m_threshold_low,
                     "Threshold high must be greater than threshold low");
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    using namespace epoch_frame;

    // Get the numerator and denominator
    Series ratio = bars[GetInputId("ratio")];

    // Create output masks based on ratio thresholds
    Series high = ratio > Scalar(m_threshold_high);
    Series normal = (ratio >= Scalar(m_threshold_low)) &&
                    (ratio <= Scalar(m_threshold_high));
    Series low = ratio < Scalar(m_threshold_low);

    // Return DataFrame with all outputs
    return make_dataframe(
        bars.index(), {high.array(), normal.array(), low.array()},
        {GetOutputId("high"), GetOutputId("normal"), GetOutputId("low")});
  }

private:
  double m_threshold_high;
  double m_threshold_low;
};
} // namespace epoch_metadata::transform