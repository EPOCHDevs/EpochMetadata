//
// Created by Claude Code
// StaticCast - Compiler-inserted transform to materialize resolved types
//

#pragma once
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/series.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <arrow/type.h>
#include <arrow/compute/api.h>

namespace epoch_script::transform {

// StaticCastToInteger - materializes Integer type
class StaticCastToInteger : public ITransform {
public:
  explicit StaticCastToInteger(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    // If input has null type (from empty DataFrames with Any type), materialize as INT64
    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::int64(), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Validate input is compatible with Integer type (INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, UINT64)
    if (!arrow::is_integer(input_type->id())) {
      throw std::runtime_error("StaticCastToInteger: Input type " + input_type->ToString() +
                               " is not compatible with Integer type");
    }

    // Input already has compatible integer type - pass through unchanged
    return MakeResult(input);
  }
};

// StaticCastToDecimal - materializes Decimal type and handles type conversions
class StaticCastToDecimal : public ITransform {
public:
  explicit StaticCastToDecimal(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    // Handle null type - materialize as FLOAT64
    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::float64(), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Handle Boolean→Decimal conversion: true→1.0, false→0.0
    if (input_type->id() == arrow::Type::BOOL) {
      auto result = arrow::compute::Cast(input.array()->chunk(0), arrow::float64());
      if (!result.ok()) {
        throw std::runtime_error("StaticCastToDecimal: Failed to cast Boolean to Decimal: " + result.status().ToString());
      }
      auto chunked = std::make_shared<arrow::ChunkedArray>(result.ValueOrDie().make_array());
      std::vector<arrow::ChunkedArrayPtr> data_vec = {chunked};
      return epoch_frame::make_dataframe(bars.index(), data_vec, {GetOutputId()});
    }

    // Validate input is compatible with Decimal type (FLOAT, DOUBLE, or INTEGER types)
    if (!arrow::is_floating(input_type->id()) && !arrow::is_integer(input_type->id())) {
      throw std::runtime_error("StaticCastToDecimal: Input type " + input_type->ToString() +
                               " is not compatible with Decimal type");
    }

    // Already numeric - pass through
    return MakeResult(input);
  }
};

// StaticCastToBoolean - materializes Boolean type and handles type conversions
class StaticCastToBoolean : public ITransform {
public:
  explicit StaticCastToBoolean(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    // Handle null type - materialize as BOOL
    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::boolean(), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Handle Numeric→Boolean conversion: 0→false, non-zero→true
    if (arrow::is_floating(input_type->id()) || arrow::is_integer(input_type->id())) {
      // Use not_equal(input, 0) to convert: num != 0
      auto zero_scalar = arrow::MakeScalar(0.0);
      auto result = arrow::compute::CallFunction("not_equal", {input.array()->chunk(0), zero_scalar});
      if (!result.ok()) {
        throw std::runtime_error("StaticCastToBoolean: Failed to cast Numeric to Boolean: " + result.status().ToString());
      }
      auto chunked = std::make_shared<arrow::ChunkedArray>(result.ValueOrDie().make_array());
      std::vector<arrow::ChunkedArrayPtr> data_vec = {chunked};
      return epoch_frame::make_dataframe(bars.index(), data_vec, {GetOutputId()});
    }

    // Validate input is already Boolean
    if (input_type->id() != arrow::Type::BOOL) {
      throw std::runtime_error("StaticCastToBoolean: Input type " + input_type->ToString() +
                               " is not compatible with Boolean type");
    }

    // Already boolean - pass through
    return MakeResult(input);
  }
};

// StaticCastToString - materializes String type
class StaticCastToString : public ITransform {
public:
  explicit StaticCastToString(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::utf8(), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Validate input is compatible with String type (STRING, LARGE_STRING)
    if (!arrow::is_base_binary_like(input_type->id())) {
      throw std::runtime_error("StaticCastToString: Input type " + input_type->ToString() +
                               " is not compatible with String type");
    }

    return MakeResult(input);
  }
};

// Timestamp specialization - uses epoch_frame::DateTime
class StaticCastToTimestamp : public ITransform {
public:
  explicit StaticCastToTimestamp(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::timestamp(arrow::TimeUnit::NANO), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Validate input is compatible with Timestamp type
    auto type_id = input_type->id();
    if (type_id != arrow::Type::TIMESTAMP && type_id != arrow::Type::DATE32 && type_id != arrow::Type::DATE64) {
      throw std::runtime_error("StaticCastToTimestamp: Input type " + input_type->ToString() +
                               " is not compatible with Timestamp type");
    }

    return MakeResult(input);
  }
};

} // namespace epoch_script::transform
