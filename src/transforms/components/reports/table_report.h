#pragma once

#include <epochflow/transforms/components/reports/ireport.h>
#include <epochflow/transforms/core/sql_options.h>
#include <epochflow/core/sql_statement.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <arrow/api.h>

namespace epochflow::reports {

class TableReport : public IReporter {
public:
  explicit TableReport(epochflow::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlStatement(GetSQLStatement()),
        m_tableTitle(GetTableTitle()),
        m_addIndex(GetAddIndex()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;


private:
  // Cached configuration values
  const SqlStatement m_sqlStatement;
  const std::string m_tableTitle;
  const bool m_addIndex;

  // Configuration getters
  SqlStatement GetSQLStatement() const;
  bool GetAddIndex() const;
  std::string GetTableTitle() const;

  // Helper methods
};

// Metadata specialization for TableReport
template <> struct ReportMetadata<TableReport> {
  constexpr static const char *kReportId = "table_report";

  static epochflow::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Table Report",
      .options = {
        epochflow::transforms::SQL_OPTION,
        epochflow::transforms::ADD_INDEX_OPTION,
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epochflow::MetaDataOptionDefinition{"SQL Query Result"},
         .isRequired = false,
         .desc = "Title for the generated table"}
      },
      .isCrossSectional = false,
      .desc = "Execute SQL query on input DataFrame and generate table output for tearsheet visualization",
      .inputs = {
        {epoch_core::IODataType::Any, epochflow::ARG, "", true}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "table", "sql", "query"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epochflow::reports