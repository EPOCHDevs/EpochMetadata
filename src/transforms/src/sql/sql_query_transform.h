#pragma once

#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/dataframe.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <regex>
#include <iostream>

#include "reports/report_utils.h"

namespace epoch_metadata::transform {

template <size_t NumOutputs>
class SQLQueryTransform : public ITransform {
  static_assert(NumOutputs >= 1 && NumOutputs <= 4,
                "SQLQueryTransform supports 1 to 4 outputs only");
public:
  explicit SQLQueryTransform(const TransformConfiguration &config)
      : ITransform(config),
  m_sqlQuery(m_config.GetOptionValue("sql").GetString())
  {
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &df) const override {
    // Step 1: Build input rename mapping (input0, input1, input2, ...)
    auto inputRenameMap = this->BuildVARGInputRenameMapping();

    // Step 2: Rename data columns to input0, input1, input2, ...
    epoch_frame::DataFrame inputDf = df.rename(inputRenameMap);

    // Step 3: Execute SQL query with timestamp index
    // SQLQueryTransform is a timeseries transform - index is always added as 'timestamp'
    auto resultTable = inputDf.reset_index("timestamp").query(m_sqlQuery, "input");

    // Convert Arrow Table to DataFrame
    epoch_frame::DataFrame resultDf(resultTable);

    // Step 4: Validate required columns exist
    auto columnNames = resultDf.column_names();
    std::unordered_set<std::string> availableColumns(columnNames.begin(), columnNames.end());

    // Timestamp must always be present for timeseries transforms
    if (availableColumns.find("timestamp") == availableColumns.end()) {
      throw std::runtime_error("SQL query result missing required 'timestamp' column");
    }

    // Step 5: Handle output mapping
    if constexpr (NumOutputs == 1) {
      // For single output, return entire DataFrame from SQL with timestamp as index
      // The SQL query defines the exact columns that should be in the output
      return resultDf.set_index("timestamp");
    } else {
      // For multiple outputs, validate and extract specific columns
      std::unordered_map<std::string, std::string> outputMap;
      std::vector<transforms::IOMetaData> outputMetaData = GetOutputMetaData();
      std::vector<std::string> outputColumns;

      for (auto const& io : outputMetaData) {
        // Validate that each expected output column exists
        if (availableColumns.find(io.id) == availableColumns.end()) {
          throw std::runtime_error("SQL query result missing required column: " + io.id);
        }
        outputMap[io.id] = GetOutputId(io.id);
        outputColumns.emplace_back(io.id);
      }

      // Include timestamp in the column selection
      outputColumns.emplace_back("timestamp");

      resultDf = resultDf[outputColumns].rename(outputMap);
      return resultDf.set_index("timestamp");
    }
  }

private:
  std::string m_sqlQuery;
};

// Type aliases for 1-4 output variants
using SQLQueryTransform1 = SQLQueryTransform<1>;
using SQLQueryTransform2 = SQLQueryTransform<2>;
using SQLQueryTransform3 = SQLQueryTransform<3>;
using SQLQueryTransform4 = SQLQueryTransform<4>;

} // namespace epoch_metadata::transform