#pragma once

#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <arrow/api.h>
#include <optional>

using namespace std::chrono_literals;

namespace investigation_test {

// Helper to create a dataframe with a string column containing nulls
inline epoch_frame::DataFrame make_dataframe_with_nullable_strings(
    const epoch_frame::IndexPtr& index,
    const std::vector<std::optional<std::string>>& string_values,
    const std::vector<double>& numeric_values,
    const std::string& string_col_name,
    const std::string& numeric_col_name) {

  // Build string array with nulls using Arrow directly
  arrow::StringBuilder string_builder;
  for (const auto& opt_str : string_values) {
    if (opt_str.has_value()) {
      if (!string_builder.Append(opt_str.value()).ok()) {
        throw std::runtime_error("Failed to append string");
      }
    } else {
      if (!string_builder.AppendNull().ok()) {
        throw std::runtime_error("Failed to append null");
      }
    }
  }

  std::shared_ptr<arrow::Array> string_arrow_array;
  if (!string_builder.Finish(&string_arrow_array).ok()) {
    throw std::runtime_error("Failed to finish string array");
  }

  // Create chunked arrays
  auto string_chunked = std::make_shared<arrow::ChunkedArray>(string_arrow_array);
  auto numeric_array = epoch_frame::factory::array::make_array(numeric_values);

  std::vector<arrow::ChunkedArrayPtr> chunked_arrays = {string_chunked, numeric_array};
  return epoch_frame::make_dataframe(index, chunked_arrays, {string_col_name, numeric_col_name});
}

// Helper to create datetime index for a range of days
inline epoch_frame::IndexPtr make_date_range(int start_day, int num_days) {
  std::vector<epoch_frame::DateTime> dates;
  for (int i = 0; i < num_days; ++i) {
    dates.push_back(epoch_frame::DateTime{2020y, std::chrono::January, 1d + std::chrono::days(start_day + i)});
  }
  return epoch_frame::factory::index::make_datetime_index(dates);
}

// Helper to create a simple string series for index function testing
inline epoch_frame::Series make_string_series(
    const epoch_frame::IndexPtr& index,
    const std::vector<std::string>& string_values) {

  auto string_array = epoch_frame::factory::array::make_array(string_values);
  return epoch_frame::Series(index, string_array);
}

} // namespace investigation_test