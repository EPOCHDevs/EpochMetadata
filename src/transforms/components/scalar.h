#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_frame/factory/array_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <numbers>
#include "type_tags.h"

namespace epoch_script::transform {

// Helper function to get the index from input bars (DRY principle)
// SRP: Responsible only for extracting the last timestamp for scalar output
inline epoch_frame::IndexPtr CreateScalarIndex(const epoch_frame::DataFrame& bars) {
    // Return single-element index with the last timestamp from input bars
    // iat(-1) returns IndexPtr with the last element (Python-style negative indexing)
    return bars.index()->iat(-1);
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
      auto index = CreateScalarIndex(bars);

      if constexpr (std::is_same_v<T, std::nullopt_t>) {
          // Build null array with single element
          auto arrowArray_temp = arrow::MakeArrayOfNull(arrow::null(), 1).MoveValueUnsafe();
          auto chunkedArray = std::make_shared<arrow::ChunkedArray>(arrowArray_temp);
          return make_dataframe(index, {chunkedArray}, {GetOutputId()});
      }
      else {
          // Build single-element array with the scalar value
          auto arrowArray = epoch_frame::factory::array::make_array(
              std::vector<T>{m_value});
          return epoch_frame::make_dataframe(index, {arrowArray}, {GetOutputId()});
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

    // Typed null scalars - DRY template implementation
    // Helper to get Arrow type for each type tag
    template <typename TypeTag>
    struct ArrowTypeMapper;

    template<> struct ArrowTypeMapper<StringType> {
        static std::shared_ptr<arrow::DataType> get() { return arrow::utf8(); }
    };
    template<> struct ArrowTypeMapper<NumberType> {
        static std::shared_ptr<arrow::DataType> get() { return arrow::float64(); }
    };
    template<> struct ArrowTypeMapper<BooleanType> {
        static std::shared_ptr<arrow::DataType> get() { return arrow::boolean(); }
    };
    template<> struct ArrowTypeMapper<TimestampType> {
        static std::shared_ptr<arrow::DataType> get() { return arrow::timestamp(arrow::TimeUnit::NANO); }
    };

    // Generic typed null scalar template
    template <typename TypeTag>
    struct TypedNullScalar : ITransform {
        explicit TypedNullScalar(const TransformConfiguration &config)
            : ITransform(config) {}

        [[nodiscard]] epoch_frame::DataFrame
        TransformData(epoch_frame::DataFrame const &bars) const override {
            auto index = CreateScalarIndex(bars);
            auto arrowType = ArrowTypeMapper<TypeTag>::get();
            auto arrowArray_temp = arrow::MakeArrayOfNull(arrowType, 1).MoveValueUnsafe();
            auto chunkedArray = std::make_shared<arrow::ChunkedArray>(arrowArray_temp);
            return epoch_frame::make_dataframe(index, {chunkedArray}, {GetOutputId()});
        }
    };

    // Type aliases using clear naming convention: null_{type}
    using NullStringScalar = TypedNullScalar<StringType>;
    using NullNumberScalar = TypedNullScalar<NumberType>;
    using NullBooleanScalar = TypedNullScalar<BooleanType>;
    using NullTimestampScalar = TypedNullScalar<TimestampType>;


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
