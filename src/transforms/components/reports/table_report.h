#pragma once

#include <epoch_script/transforms/components/reports/ireport.h>
#include <epoch_script/transforms/core/sql_options.h>
#include <epoch_script/core/sql_statement.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <arrow/api.h>

namespace epoch_script::reports {

class TableReport : public IReporter {
public:
  explicit TableReport(epoch_script::transform::TransformConfiguration config)
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

  static epoch_script::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .name = "Table Report",
      .options = {
        epoch_script::transforms::SQL_OPTION,
        epoch_script::transforms::ADD_INDEX_OPTION,
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_script::MetaDataOptionDefinition{"SQL Query Result"},
         .isRequired = false,
         .desc = "Title for the generated table"}
      },
      .isCrossSectional = false,
      .desc = "Execute SQL query on input DataFrame and generate table output for tearsheet visualization",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_script::ARG, "", true}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "table", "sql", "query"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epoch_script::reports