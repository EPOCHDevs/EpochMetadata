#pragma once

#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace epoch_metadata::transform {

template <size_t NumOutputs>
class SQLQueryTransform : public ITransform {
  static_assert(NumOutputs >= 1 && NumOutputs <= 4,
                "SQLQueryTransform supports 1 to 4 outputs only");
public:
  explicit SQLQueryTransform(const TransformConfiguration &config)
      : ITransform(config) {
    ValidateConfiguration();
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &df) const override {
    // Get SQL query from options
    std::string sql = GetSQLQuery();

    // Get all input columns to create input DataFrames
    auto inputIds = GetInputIds();

    if (inputIds.empty()) {
      throw std::runtime_error("At least one input is required for SQLQueryTransform");
    }

    // Prepare input DataFrames map
    std::unordered_map<std::string, epoch_frame::DataFrame> inputs;

    // Check if we have requiredDataSources for market data
    const auto& metadata = m_config.GetTransformDefinition().GetMetadata();
    if (!metadata.requiredDataSources.empty()) {
      // Include market data columns
      std::vector<std::string> marketCols = metadata.requiredDataSources;
      inputs["market_data"] = df[marketCols];
    }

    // Map each input to a DataFrame
    for (const auto& [inputId, columns] : m_config.GetInputs()) {
      if (!columns.empty()) {
        inputs[inputId] = df[columns];
      }
    }

    // If we have variadic inputs (like agg_* pattern), create numbered tables
    if (!inputIds.empty() && inputs.empty()) {
      // All inputs are in GetInputIds()
      for (size_t i = 0; i < inputIds.size(); ++i) {
        inputs["input" + std::to_string(i)] = df[[inputIds[i]]];
      }
    }

    // Execute SQL with inputs
    auto result = ExecuteSQLWithInputs(sql, inputs);

    // Split result into multiple outputs if needed
    if constexpr (NumOutputs == 1) {
      return result;
    } else {
      return SplitOutputs(result);
    }
  }

private:
  void ValidateConfiguration() const {
    if (GetSQLQuery().empty()) {
      throw std::runtime_error("SQLQueryTransform: 'sql' option is required");
    }

    // Validate output column specifications for multi-output variants
    if constexpr (NumOutputs > 1) {
      auto outputCols = GetOutputColumnSets();
      if (outputCols.size() != NumOutputs) {
        throw std::runtime_error(
          "SQLQueryTransform" + std::to_string(NumOutputs) +
          ": Must specify " + std::to_string(NumOutputs) +
          " output column sets");
      }
    }
  }

  std::string GetSQLQuery() const {
    auto options = m_config.GetOptions();
    if (options.contains("sql") && options["sql"].IsScalar()) {
      return options["sql"].as<std::string>();
    }
    return "";
  }

  std::vector<std::vector<std::string>> GetOutputColumnSets() const {
    std::vector<std::vector<std::string>> result;
    auto options = m_config.GetOptions();

    for (size_t i = 0; i < NumOutputs; ++i) {
      std::string key = "output" + std::to_string(i) + "_columns";
      if (options.contains(key) && options[key].IsSequence()) {
        std::vector<std::string> cols;
        for (const auto& col : options[key]) {
          cols.push_back(col.as<std::string>());
        }
        result.push_back(cols);
      }
    }
    return result;
  }

  epoch_frame::DataFrame ExecuteSQL(const std::string& sql) const {
    // Direct SQL execution without DataFrame context
    return epoch_frame::DataFrame::sql(sql);
  }

  epoch_frame::DataFrame ExecuteSQLWithInputs(
      const std::string& sql,
      const std::unordered_map<std::string, epoch_frame::DataFrame>& inputs) const {
    // Execute SQL with multiple DataFrames as context
    return epoch_frame::DataFrame::sql(sql, inputs);
  }

  epoch_frame::DataFrame SplitOutputs(const epoch_frame::DataFrame& result) const {
    // For multi-output variants, split columns into separate result sets
    auto outputCols = GetOutputColumnSets();
    std::vector<epoch_frame::DataFrame> outputs;

    for (const auto& colSet : outputCols) {
      outputs.push_back(result[colSet]);
    }

    // Combine outputs with numbered suffixes
    // This is a simplified approach - actual implementation would need
    // to handle the computation graph's multi-output requirements properly
    epoch_frame::DataFrame combined;
    for (size_t i = 0; i < outputs.size(); ++i) {
      std::string suffix = "_output" + std::to_string(i);
      // Rename columns with output suffix
      auto renamed = outputs[i];
      std::unordered_map<std::string, std::string> renameMap;
      for (const auto& col : outputs[i].column_names()) {
        renameMap[col] = col + suffix;
      }
      renamed = renamed.rename(renameMap);

      if (i == 0) {
        combined = renamed;
      } else {
        // Concatenate horizontally
        combined = combined.concat(renamed, epoch_frame::AxisType::Column);
      }
    }

    return combined;
  }
};

// Type aliases for 1-4 output variants
using SQLQueryTransform1 = SQLQueryTransform<1>;
using SQLQueryTransform2 = SQLQueryTransform<2>;
using SQLQueryTransform3 = SQLQueryTransform<3>;
using SQLQueryTransform4 = SQLQueryTransform<4>;

} // namespace epoch_metadata::transform