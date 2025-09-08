//
// Created by dewe on 10/8/24.
//

#include "select.h"
#include "epoch_frame/array.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include <arrow/chunked_array.h>
#include <memory>

namespace epoch_metadata::transform {
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
} // namespace epoch_metadata::transform