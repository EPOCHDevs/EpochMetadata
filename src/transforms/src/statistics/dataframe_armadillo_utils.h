#pragma once
//
// DataFrame to Armadillo Matrix Conversion Utilities
//
#include "epoch_frame/dataframe.h"
#include "epoch_frame/series.h"
#include <armadillo>
#include <arrow/table.h>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace epoch_metadata::transform::utils {

/**
 * @brief Converts specified columns from an epoch_frame::DataFrame to an
 * Armadillo matrix
 *
 * This function provides optimized conversion from DataFrame columns to
 * arma::mat using efficient memory operations (memcpy) and Armadillo's advanced
 * constructors where possible.
 *
 * @param df The source DataFrame
 * @param column_names Vector of column names to extract
 * @return arma::mat Matrix with dimensions [num_rows x num_columns]
 *
 * @throws std::runtime_error if no columns specified or columns don't exist
 *
 * Performance optimizations:
 * - Uses memcpy for bulk column copying (much faster than element-by-element)
 * - Special case optimization for single columns using advanced constructor
 * - Handles type casting to double automatically
 * - Leverages Armadillo's column-major storage layout
 */
inline arma::mat
MatFromDataFrame(const epoch_frame::DataFrame &df,
                 const std::vector<std::string> &column_names) {
  if (column_names.empty()) {
    throw std::runtime_error("No columns specified for matrix conversion");
  }

  const size_t T = df.num_rows();       // number of rows (time points)
  const size_t D = column_names.size(); // number of columns (dimensions)

  if (T == 0) {
    return arma::mat(0, D); // empty matrix with correct number of columns
  }

  // Special case: single column - can use advanced constructor
  if (D == 1) {
    const auto &col_name = column_names[0];
    auto column_array = df[col_name].contiguous_array();

    // Cast to double if needed
    if (column_array.type()->id() != arrow::Type::DOUBLE) {
      column_array = column_array.cast(arrow::float64());
    }

    const auto column_view = column_array.template to_view<double>();
    const double *raw_data = column_view->raw_values();

    // Use advanced constructor with copy_aux_mem=true for safety
    // This is still more efficient than element-by-element copying
    return arma::mat(const_cast<double *>(raw_data), T, 1, true, false);
  }

  // Multi-column case: use memcpy approach
  arma::mat X(T, D);

  for (size_t j = 0; j < D; ++j) {
    const auto &col_name = column_names[j];

    // Get column data as contiguous array and convert to double view
    auto column_array = df[col_name].contiguous_array();

    // Cast to double if needed
    if (column_array.type()->id() != arrow::Type::DOUBLE) {
      column_array = column_array.cast(arrow::float64());
    }

    const auto column_view = column_array.template to_view<double>();
    const double *raw_data = column_view->raw_values();

    // Use memcpy to copy entire column at once
    // Armadillo uses column-major storage, so column j starts at X.colptr(j)
    std::memcpy(X.colptr(j), raw_data, T * sizeof(double));
  }

  return X;
}

/**
 * @brief Converts a single column from DataFrame to Armadillo column vector
 *
 * @param df The source DataFrame
 * @param column_name Name of the column to extract
 * @return arma::vec Column vector
 */
inline arma::vec VecFromDataFrame(const epoch_frame::DataFrame &df,
                                  const std::string &column_name) {
  auto matrix = MatFromDataFrame(df, {column_name});
  return matrix.col(0);
}

/**
 * @brief Converts all numeric columns from DataFrame to Armadillo matrix
 *
 * @param df The source DataFrame
 * @return arma::mat Matrix containing all numeric columns
 *
 * @note This function automatically filters for numeric columns only
 */
inline arma::mat MatFromDataFrameAllNumeric(const epoch_frame::DataFrame &df) {
  std::vector<std::string> numeric_columns;

  // Get all column names from the DataFrame's table
  auto table = df.table();
  auto schema = table->schema();

  for (int i = 0; i < schema->num_fields(); ++i) {
    auto field = schema->field(i);
    auto type_id = field->type()->id();

    // Check if column is numeric (double, float, int types)
    if (type_id == arrow::Type::DOUBLE || type_id == arrow::Type::FLOAT ||
        type_id == arrow::Type::INT64 || type_id == arrow::Type::INT32 ||
        type_id == arrow::Type::INT16 || type_id == arrow::Type::INT8 ||
        type_id == arrow::Type::UINT64 || type_id == arrow::Type::UINT32 ||
        type_id == arrow::Type::UINT16 || type_id == arrow::Type::UINT8) {
      numeric_columns.push_back(field->name());
    }
  }

  if (numeric_columns.empty()) {
    throw std::runtime_error("No numeric columns found in DataFrame");
  }

  return MatFromDataFrame(df, numeric_columns);
}

} // namespace epoch_metadata::transform::utils
