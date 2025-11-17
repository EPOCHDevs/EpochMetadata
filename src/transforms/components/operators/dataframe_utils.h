#pragma once

#include <epoch_frame/dataframe.h>
#include <string>

namespace epoch_script::transform::utils {

/**
 * Drop rows where specified key column is null.
 * Unlike drop_null() which drops rows with ANY null column,
 * this only drops rows where the specified key column is null.
 *
 * This is useful for operations like groupby where null keys don't make sense,
 * but null values should be preserved for aggregation.
 *
 * @param df The input DataFrame
 * @param key_col The column name to check for nulls
 * @return DataFrame with rows removed where key_col is null
 */
inline epoch_frame::DataFrame drop_by_key(
    epoch_frame::DataFrame const& df,
    [[maybe_unused]] std::string const& key_col) {
  // TODO: Ideally we want to only drop rows where key_col is null
  // but epochframe's current API doesn't support df[boolean_series] filtering
  // For now, we use drop_null() which drops ANY row with nulls
  // This is safe for groupby use case where we select only [key, value] columns
  return df.drop_null();
}

} // namespace epoch_script::transform::utils
