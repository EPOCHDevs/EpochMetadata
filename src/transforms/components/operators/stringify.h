//
// Created by Claude Code
// Stringify - Converts any type to its string representation
//

#pragma once
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/series.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <arrow/type.h>
#include <arrow/compute/api.h>

namespace epoch_script::transform {

// Stringify Transform - converts Any type to String
// Behaves like Python's str() function
class Stringify : public ITransform {
public:
  explicit Stringify(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series input = bars[GetInputId()];
    auto input_type = input.array()->type();

    // Handle null type - materialize as empty string array
    if (input_type->id() == arrow::Type::NA) {
      size_t size = input.size();
      auto null_array = arrow::MakeArrayOfNull(arrow::utf8(), size).ValueOrDie();
      auto chunked_array = std::make_shared<arrow::ChunkedArray>(null_array);
      return epoch_frame::make_dataframe(bars.index(), {chunked_array}, {GetOutputId()});
    }

    // Already a string - pass through
    if (arrow::is_base_binary_like(input_type->id())) {
      return MakeResult(input);
    }

    // For all other types, use Arrow's cast to string
    // This handles: Boolean, Integer, Float, Decimal, Timestamp, etc.
    arrow::compute::CastOptions cast_opts;
    cast_opts.to_type = arrow::utf8();

    auto result = arrow::compute::Cast(input.array()->chunk(0), arrow::utf8(), cast_opts);
    if (!result.ok()) {
      throw std::runtime_error("Stringify: Failed to convert " + input_type->ToString() +
                             " to String: " + result.status().ToString());
    }

    auto chunked = std::make_shared<arrow::ChunkedArray>(result.ValueOrDie().make_array());
    return epoch_frame::make_dataframe(bars.index(), {chunked}, {GetOutputId()});
  }
};

} // namespace epoch_script::transform