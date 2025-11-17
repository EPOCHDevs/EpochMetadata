#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/dataframe.h>
#include "dataframe_utils.h"

// Numeric aggregation types for GroupBy
CREATE_ENUM(GroupByNumericAgg,
  sum,
  mean,
  count,
  first,
  last,
  min,
  max
);

// Boolean aggregation types for GroupBy
CREATE_ENUM(GroupByBooleanAgg,
  AllOf,
  AnyOf,
  NoneOf
);

// Any type aggregation types for GroupBy (Any -> Boolean)
CREATE_ENUM(GroupByAnyAgg,
  IsEqual,
  IsUnique
);

namespace epoch_script::transform {

// Base template for GroupBy aggregation transforms
template <typename AggEnum>
struct GroupByAggTransform final : ITransform {
  explicit GroupByAggTransform(const TransformConfiguration &config)
      : ITransform(config),
        m_agg_type(config.GetOptionValue("agg").GetSelectOption<AggEnum>()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  AggEnum m_agg_type;

  // Helper to get aggregated dataframe based on aggregation type
  epoch_frame::DataFrame ApplyAggregation(
      epoch_frame::DataFrame const &df,
      std::string const &group_key_col,
      std::string const &value_col) const;
};

// Specialization for Numeric aggregations
template <>
inline epoch_frame::DataFrame GroupByAggTransform<epoch_core::GroupByNumericAgg>::ApplyAggregation(
    epoch_frame::DataFrame const &df,
    std::string const &group_key_col,
    std::string const &value_col) const {

  // Drop null group keys - grouping by null doesn't make semantic sense
  // Select only the columns we need, then drop any row with nulls in the key
  auto temp_df = df[{group_key_col, value_col}];
  auto non_null_df = epoch_script::transform::utils::drop_by_key(temp_df, group_key_col);
  auto grouped = non_null_df.group_by_agg(group_key_col);

  switch (m_agg_type) {
    case epoch_core::GroupByNumericAgg::sum:
      return grouped.sum();
    case epoch_core::GroupByNumericAgg::mean:
      return grouped.mean();
    case epoch_core::GroupByNumericAgg::count:
      return grouped.count();
    case epoch_core::GroupByNumericAgg::first:
      return grouped.first();
    case epoch_core::GroupByNumericAgg::last:
      return grouped.last();
    case epoch_core::GroupByNumericAgg::min:
      return grouped.min();
    case epoch_core::GroupByNumericAgg::max:
      return grouped.max();
    default:
      throw std::runtime_error("Unsupported numeric aggregation type");
  }
}

// Specialization for Boolean aggregations
template <>
inline epoch_frame::DataFrame GroupByAggTransform<epoch_core::GroupByBooleanAgg>::ApplyAggregation(
    epoch_frame::DataFrame const &df,
    std::string const &group_key_col,
    std::string const &value_col) const {

  // Drop null group keys - grouping by null doesn't make semantic sense
  // Select only the columns we need, then drop any row with nulls in the key
  auto temp_df = df[{group_key_col, value_col}];
  auto non_null_df = epoch_script::transform::utils::drop_by_key(temp_df, group_key_col);
  auto grouped = non_null_df.group_by_agg(group_key_col);

  switch (m_agg_type) {
    case epoch_core::GroupByBooleanAgg::AllOf:
      return grouped.all();
    case epoch_core::GroupByBooleanAgg::AnyOf:
      return grouped.any();
    case epoch_core::GroupByBooleanAgg::NoneOf: {
      // NoneOf is equivalent to NOT(AnyOf) = all values are false
      auto any_result = grouped.any();
      // Negate the boolean result
      auto series = any_result.to_series();
      return (!series).to_frame(value_col);
    }
    default:
      throw std::runtime_error("Unsupported boolean aggregation type");
  }
}

// Main TransformData implementation
template <typename AggEnum>
epoch_frame::DataFrame GroupByAggTransform<AggEnum>::TransformData(
    epoch_frame::DataFrame const &bars) const {

  // Get input column names
  auto inputs = GetInputIds();
  AssertFromStream(inputs.size() == 2, "GroupByAgg requires exactly 2 inputs: group_key and value");

  std::string group_key_col = inputs[0];  // First input is group_key
  std::string value_col = inputs[1];      // Second input is value

  // Step 1: Create a position column with integer positions (0, 1, 2, ...)
  std::vector<int64_t> positions;
  positions.reserve(bars.size());
  for (int64_t i = 0; i < static_cast<int64_t>(bars.size()); ++i) {
    positions.push_back(i);
  }
  auto position_array = epoch_frame::factory::array::make_array(positions);

  // Create DataFrame with [group_key, value, position]
  epoch_frame::DataFrame df_with_positions = epoch_frame::make_dataframe(
      bars.index(),
      {bars[group_key_col].array(), bars[value_col].array(), position_array},
      {group_key_col, value_col, "__position__"}
  );

  // Step 2: Group by group_key and aggregate value column
  epoch_frame::DataFrame value_agg = ApplyAggregation(
      df_with_positions,
      group_key_col,
      value_col
  );
  // Reset index to get group_key back as a column
  value_agg = value_agg.reset_index(group_key_col);

  // Step 3: Group by group_key and get position for index selection based on aggregation type
  // - If agg is 'first', use first position
  // - If agg is 'last', use last position
  // - Otherwise (sum, mean, count, min, max), default to last position
  // Drop null group keys - grouping by null doesn't make semantic sense
  // Select only the columns we need, then drop any row with nulls in the key
  auto temp_pos_df = df_with_positions[{group_key_col, "__position__"}];
  auto non_null_pos_df = epoch_script::transform::utils::drop_by_key(temp_pos_df, group_key_col);
  auto grouped_positions = non_null_pos_df.group_by_agg(group_key_col);
  epoch_frame::DataFrame position_agg;

  if constexpr (std::is_same_v<AggEnum, epoch_core::GroupByNumericAgg>) {
    if (m_agg_type == epoch_core::GroupByNumericAgg::first) {
      position_agg = grouped_positions.first();
    } else if (m_agg_type == epoch_core::GroupByNumericAgg::last) {
      position_agg = grouped_positions.last();
    } else {
      // Default to last for sum, mean, count, min, max
      position_agg = grouped_positions.last();
    }
  } else {
    // For boolean and any aggregations, default to last
    position_agg = grouped_positions.last();
  }
  // Reset index to get group_key back as a column
  position_agg = position_agg.reset_index(group_key_col);

  // Step 4: Use the aggregated positions to select rows from original dataframe
  // The position_agg dataframe has the __position__ column with the selected positions
  // Cast to int64 to ensure correct type for iloc (aggregations may have converted to double)
  auto selected_positions_array = position_agg["__position__"].array();
  auto selected_positions_series = epoch_frame::Series(selected_positions_array, "__position__");
  auto selected_positions_int64 = selected_positions_series.cast(arrow::int64());
  epoch_frame::Array index_array(selected_positions_int64.array());
  epoch_frame::DataFrame selected_rows = bars.iloc(index_array);
  auto result_index = selected_rows.index();

  // Step 6: Build output DataFrame with [grouped_index, group_key, aggregated_value]
  auto group_key_series = value_agg[group_key_col];
  auto value_series = value_agg[value_col];

  return epoch_frame::make_dataframe(
      result_index,
      {group_key_series.array(), value_series.array()},
      {GetOutputId("group_key"), GetOutputId("value")}
  );
}

// Specialization for Any aggregations (Any -> Boolean)
template <>
inline epoch_frame::DataFrame GroupByAggTransform<epoch_core::GroupByAnyAgg>::ApplyAggregation(
    epoch_frame::DataFrame const &df,
    std::string const &group_key_col,
    std::string const &value_col) const {

  // Drop null group keys - grouping by null doesn't make semantic sense
  // Select only the columns we need, then drop any row with nulls in the key
  auto temp_df = df[{group_key_col, value_col}];
  auto non_null_df = epoch_script::transform::utils::drop_by_key(temp_df, group_key_col);
  auto grouped = non_null_df.group_by_agg(group_key_col);

  switch (m_agg_type) {
    case epoch_core::GroupByAnyAgg::IsEqual: {
      // Check if all values in the group are equal
      // If min == max, then all values are identical
      auto min_df = grouped.min();
      auto max_df = grouped.max();
      auto min_series = min_df[value_col];
      auto max_series = max_df[value_col];
      auto result = min_series == max_series;
      return result.to_frame(value_col);
    }
    case epoch_core::GroupByAnyAgg::IsUnique: {
      // Check if all values in the group are unique (no duplicates)
      // If count == 1, trivially unique
      // Otherwise, check if first != last (heuristic for small groups)
      auto count_df = grouped.count();
      auto first_df = grouped.first();
      auto last_df = grouped.last();
      auto count_series = count_df[value_col];
      auto first_series = first_df[value_col];
      auto last_series = last_df[value_col];

      // Result: (count == 1) OR (first != last)
      // This is a simple heuristic - not perfect for groups > 2
      auto is_single = count_series == epoch_frame::Scalar(static_cast<int64_t>(1));
      auto is_different = first_series != last_series;
      auto result = is_single || is_different;
      return result.to_frame(value_col);
    }
    default:
      throw std::runtime_error("Unsupported Any aggregation type");
  }
}

// Concrete transform classes - one for numeric, one for boolean, one for any
using GroupByNumericAggTransform = GroupByAggTransform<epoch_core::GroupByNumericAgg>;
using GroupByBooleanAggTransform = GroupByAggTransform<epoch_core::GroupByBooleanAgg>;
using GroupByAnyAggTransform = GroupByAggTransform<epoch_core::GroupByAnyAgg>;

} // namespace epoch_script::transform
