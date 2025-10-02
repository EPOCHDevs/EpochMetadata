#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_frame/factory/array_factory.h"
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/factory/dataframe_factory.h>
#include <numbers>

namespace epoch_metadata::transform {
template <typename T> struct ScalarDataFrameTransform : ITransform {
  explicit ScalarDataFrameTransform(const TransformConfiguration &config)
      : ITransform(config),
        m_value(std::get<T>(m_config.GetOptionValue("value").GetVariant())) {}

  explicit ScalarDataFrameTransform(const TransformConfiguration &config,
                                    T constant)
      : ITransform(config), m_value(constant) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
      if constexpr (std::is_same_v<T, std::nullopt_t>) {
          auto arrowArray = arrow::MakeArrayOfNull(arrow::null(), bars.size()).MoveValueUnsafe();
          return make_dataframe(bars.index(), {std::make_shared<arrow::ChunkedArray>(arrowArray)},
                                             {GetOutputId()});
      }
      else {
          auto arrowArray = epoch_frame::factory::array::make_array(
    std::vector<T>(bars.size(), m_value));
          return epoch_frame::make_dataframe(bars.index(), {arrowArray},
                                             {GetOutputId()});
      }
  }

private:
  T m_value;
};

    struct NullScalar : ScalarDataFrameTransform<std::nullopt_t> {
        explicit NullScalar(const TransformConfiguration &config)
            : ScalarDataFrameTransform(config, std::nullopt) {}
    };


struct ZeroScalar : ScalarDataFrameTransform<double> {
  explicit ZeroScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, 0) {}
};

struct OneScalar : ScalarDataFrameTransform<double> {
  explicit OneScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, 1) {}
};

struct NegativeOneScalar : ScalarDataFrameTransform<double> {
  explicit NegativeOneScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, -1) {}
};

struct PiScalar : ScalarDataFrameTransform<double> {
  explicit PiScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::pi) {}
};

struct EScalar : ScalarDataFrameTransform<double> {
  explicit EScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::e) {}
};

struct PhiScalar : ScalarDataFrameTransform<double> {
  explicit PhiScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::phi) {}
};

struct Sqrt2Scalar : ScalarDataFrameTransform<double> {
  explicit Sqrt2Scalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::sqrt(2)) {}
};

struct Sqrt3Scalar : ScalarDataFrameTransform<double> {
  explicit Sqrt3Scalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::sqrt(3)) {}
};

struct Sqrt5Scalar : ScalarDataFrameTransform<double> {
  explicit Sqrt5Scalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::sqrt(5)) {}
};

struct Ln2Scalar : ScalarDataFrameTransform<double> {
  explicit Ln2Scalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::ln2) {}
};

struct Ln10Scalar : ScalarDataFrameTransform<double> {
  explicit Ln10Scalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::ln10) {}
};

struct Log2EScalar : ScalarDataFrameTransform<double> {
  explicit Log2EScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::log2e) {}
};

struct Log10EScalar : ScalarDataFrameTransform<double> {
  explicit Log10EScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, std::numbers::log10e) {}
};

struct BoolTrueScalar : ScalarDataFrameTransform<bool> {
  explicit BoolTrueScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, true) {}
};

struct BoolFalseScalar : ScalarDataFrameTransform<bool> {
  explicit BoolFalseScalar(const TransformConfiguration &config)
      : ScalarDataFrameTransform(config, false) {}
};

using NumericScalarDataFrameTransform = ScalarDataFrameTransform<double>;
    using StringScalarDataFrameTransform = ScalarDataFrameTransform<std::string>;

} // namespace epoch_metadata::transform
