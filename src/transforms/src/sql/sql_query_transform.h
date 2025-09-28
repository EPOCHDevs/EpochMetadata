#pragma once

#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/dataframe.h>
#include <unordered_map>
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
  m_sqlQuery(m_config.GetOptionValue("sql").GetString()),
  m_tableName(m_config.GetOptionValue("table_name").GetString()),
  m_indexColumnName(m_config.GetOptionValue("index_column_name").GetString())
  {
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &df) const override {
    // Step 1: Store original column names before sanitization
    std::vector<std::string> originalColumns = df.column_names();

    // Step 2: Sanitize column names for SQL compatibility (replace # with _)
    epoch_frame::DataFrame sanitizedDf = reports::ReportUtils::SanitizeColumnNames(df);

    // Step 3: Execute SQL query
    auto resultTable = sanitizedDf.reset_index(m_indexColumnName).query(m_sqlQuery, m_tableName);

    // Convert Arrow Table to DataFrame
    epoch_frame::DataFrame resultDf(resultTable);

    // Step 4: Handle output mapping
    if constexpr (NumOutputs == 1) {
      // For single output, return entire DataFrame from SQL without setting index
      // The SQL query defines the exact columns that should be in the output
      return resultDf;
    } else {
      // For multiple outputs, extract specific columns
      std::unordered_map<std::string, std::string> outputMap;
      std::vector<transforms::IOMetaData> outputMetaData = GetOutputMetaData();
      std::vector<std::string> outputColumns;

      for (auto const& io : outputMetaData) {
        outputMap[io.id] = GetOutputId(io.id);
        outputColumns.emplace_back(io.id);
      }

      resultDf = resultDf[outputColumns].rename(outputMap);
      return resultDf;
    }
  }

private:
  std::string m_sqlQuery;
  std::string m_tableName;
  std::string m_indexColumnName;
};

// Type aliases for 1-4 output variants
using SQLQueryTransform1 = SQLQueryTransform<1>;
using SQLQueryTransform2 = SQLQueryTransform<2>;
using SQLQueryTransform3 = SQLQueryTransform<3>;
using SQLQueryTransform4 = SQLQueryTransform<4>;

} // namespace epoch_metadata::transform