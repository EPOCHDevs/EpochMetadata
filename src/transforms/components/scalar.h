#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_frame/factory/array_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <numbers>

namespace epoch_script::transform {

// Helper function to create single-row index from input bars (DRY principle)
// SRP: Responsible only for creating the scalar's timestamp
inline epoch_frame::IndexPtr CreateScalarIndex(const epoch_frame::DataFrame& bars) {
    // Use last timestamp from input bars as the scalar's timestamp
    // iat() returns IndexPtr with single element at the specified position
    const auto last_idx = static_cast<int64_t>(bars.size()) - 1;
    return bars.index()->iat(last_idx);
}

template <typename T> struct ScalarDataFrameTransform : ITransform {
  explicit ScalarDataFrameTransform(const TransformConfiguration &config)
      : ITransform(config),
        m_value(GetValueFromConfig(config)) {}

  explicit ScalarDataFrameTransform(const TransformConfiguration &config,
                                    T constant)
      : ITransform(config), m_value(constant) {}

  // SRP: TransformData's responsibility is to create a single-row DataFrame with the scalar value
  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
      auto singleRowIndex = CreateScalarIndex(bars);

      if constexpr (std::is_same_v<T, std::nullopt_t>) {
          // Build single-element null array
          auto arrowArray_temp = arrow::MakeArrayOfNull(arrow::null(), 1).MoveValueUnsafe();
          auto chunkedArray = std::make_shared<arrow::ChunkedArray>(arrowArray_temp);
          return make_dataframe(singleRowIndex, {chunkedArray}, {GetOutputId()});
      }
      else {
          // Build single-element array with the scalar value
          auto arrowArray = epoch_frame::factory::array::make_array(
              std::vector<T>(1, m_value));  // Single value, not bars.size()
          return epoch_frame::make_dataframe(singleRowIndex, {arrowArray}, {GetOutputId()});
      }
  }

private:
  // SRP: GetValueFromConfig's responsibility is to extract the value from config
  static T GetValueFromConfig(const TransformConfiguration &config) {
    if constexpr (std::is_same_v<T, double>) {
      return config.GetOptionValue("value").GetDecimal();
    } else if constexpr (std::is_same_v<T, std::string>) {
      return config.GetOptionValue("value").GetString();
    } else if constexpr (std::is_same_v<T, bool>) {
      return config.GetOptionValue("value").GetBoolean();
    } else {
      static_assert(std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, bool>,
                    "ScalarDataFrameTransform only supports double, string, and bool types");
    }
  }

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

} // namespace epoch_script::transform
