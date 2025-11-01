//
// Created by dewe on 10/8/24.
//

#include "select.h"
#include "epoch_frame/array.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include <arrow/chunked_array.h>
#include <memory>

namespace epoch_script::transform {
epoch_frame::DataFrame BooleanSelectTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {
  const epoch_frame::Array index =
      bars[GetInputId("condition")].contiguous_array();
  const epoch_frame::Array true_ = bars[GetInputId("true")].contiguous_array();
  const epoch_frame::Array false_ =
      bars[GetInputId("false")].contiguous_array();
  return epoch_frame::make_dataframe(
      bars.index(), {true_.where(index, false_).as_chunked_array()},
      {GetOutputId()});
}

// Helper function to apply a function to an index_sequence
template <typename F, size_t... Is>
auto apply_index_sequence(F &&f, std::index_sequence<Is...>) {
  return f(Is...);
}

template <size_t N>
epoch_frame::DataFrame ZeroIndexSelectTransform<N>::TransformData(
    epoch_frame::DataFrame const &bars) const {
  auto indices = bars[GetInputId("index")].array();

  // A lambda that receives a parameter pack of indices and returns a Datum
  // vector
  auto gather_arrays = [&](auto... Is) {
    return arrow::compute::CallFunction(
        "choose",
        {indices, bars[GetInputId(std::format("*{}", Is))].array()...});
  };

  // Apply the gather_arrays lambda with a generated index sequence [0, 1, 2,
  // ..., N-1]
  auto &&maybe_result =
      apply_index_sequence(gather_arrays, std::make_index_sequence<N>{});

  // Handle the result (assuming epoch_frame::AssertResultIsOk checks and
  // unwraps a Result)
  auto result = epoch_frame::AssertArrayResultIsOk(std::move(maybe_result));

  // Construct and return the new DataFrame
  return epoch_frame::make_dataframe(bars.index(), {result}, {GetOutputId()});
}

template class ZeroIndexSelectTransform<2>;

template class ZeroIndexSelectTransform<3>;

template class ZeroIndexSelectTransform<4>;

template class ZeroIndexSelectTransform<5>;

// FirstNonNull (Coalesce) implementation
epoch_frame::DataFrame FirstNonNullTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  // Get all VARARG inputs (SLOT0, SLOT1, SLOT2, ...)
  auto input_ids = GetInputIds();

  AssertFromStream(!input_ids.empty(),
                   "first_non_null requires at least one input");

  // Collect all input arrays
  std::vector<arrow::Datum> input_arrays;
  for (const auto& id : input_ids) {
    input_arrays.push_back(bars[id].array());
  }

  // Call Arrow's coalesce function
  auto maybe_result = arrow::compute::CallFunction("coalesce", input_arrays);
  auto result = epoch_frame::AssertArrayResultIsOk(std::move(maybe_result));

  return epoch_frame::make_dataframe(bars.index(), {result}, {GetOutputId()});
}

// ConditionalSelect (Case When) implementation
epoch_frame::DataFrame ConditionalSelectTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  // Get all VARARG inputs
  auto input_ids = GetInputIds();

  // Must have at least 2 inputs (1 condition + 1 value)
  AssertFromStream(input_ids.size() >= 2,
                   "conditional_select requires at least one condition/value pair");

  // Build inputs for Arrow's case_when:
  // Inputs alternate: condition0, value0, condition1, value1, ...
  // Last input (if odd count) is the default value

  size_t num_pairs = input_ids.size() / 2;
  bool has_default = (input_ids.size() % 2 == 1);

  // Collect condition chunked arrays (even indices: 0, 2, 4, ...)
  std::vector<std::shared_ptr<arrow::ChunkedArray>> condition_chunked_arrays;
  std::vector<std::shared_ptr<arrow::Field>> fields;

  for (size_t i = 0; i < num_pairs; i++) {
    auto condition_chunked = bars[input_ids[i * 2]].array();
    condition_chunked_arrays.push_back(condition_chunked);
    fields.push_back(arrow::field("cond_" + std::to_string(i), condition_chunked->type()));
  }

  // Create conditions struct - combine all condition arrays into a struct
  // We need to combine all chunks for each condition into a single array
  std::vector<std::shared_ptr<arrow::Array>> condition_arrays;
  for (const auto& chunked : condition_chunked_arrays) {
    // Combine chunks if needed
    auto maybe_combined = chunked->num_chunks() == 1
        ? arrow::Result<std::shared_ptr<arrow::Array>>(chunked->chunk(0))
        : arrow::Concatenate(chunked->chunks());
    condition_arrays.push_back(epoch_frame::AssertResultIsOk(std::move(maybe_combined)));
  }

  auto conditions_result = arrow::StructArray::Make(condition_arrays, fields);
  auto conditions_array = epoch_frame::AssertResultIsOk(std::move(conditions_result));

  // Collect value arrays (odd indices: 1, 3, 5, ...)
  std::vector<arrow::Datum> value_datums;
  for (size_t i = 0; i < num_pairs; i++) {
    value_datums.push_back(bars[input_ids[i * 2 + 1]].array());
  }

  // Prepare arguments for case_when
  std::vector<arrow::Datum> case_when_args;
  case_when_args.push_back(conditions_array);

  // Add all value datums
  for (const auto& value : value_datums) {
    case_when_args.push_back(value);
  }

  // Add default value if present (last odd index)
  if (has_default) {
    case_when_args.push_back(bars[input_ids.back()].array());
  }

  // Call Arrow's case_when function
  auto maybe_result = arrow::compute::CallFunction("case_when", case_when_args);
  auto result = epoch_frame::AssertArrayResultIsOk(std::move(maybe_result));

  return epoch_frame::make_dataframe(bars.index(), {result}, {GetOutputId()});
}

} // namespace epoch_script::transform