#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class TableReport : public IReporter {
public:
  explicit TableReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_tableName(GetTableName()),
        m_tableTitle(GetTableTitle()),
        m_addIndex(GetAddIndex()),
        m_indexColumnName(GetIndexColumnName()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  void generateDashboard(const epoch_frame::DataFrame &normalizedDf) const override {
    // Dashboard generation uses the same implementation as tearsheet
    generateTearsheet(normalizedDf);
  }

private:
  // Cached configuration values
  const std::string m_sqlQuery;
  const std::string m_tableName;
  const std::string m_tableTitle;
  const bool m_addIndex;
  const std::string m_indexColumnName;

  // Configuration getters
  std::string GetSQLQuery() const;
  std::string GetTableName() const;
  bool GetAddIndex() const;
  std::string GetIndexColumnName() const;
  std::string GetTableTitle() const;

  // Helper methods
  epoch_frame::DataFrame PrepareInputDataFrame(const epoch_frame::DataFrame& df,
                                               bool addIndex,
                                               const std::string& indexColName) const;
  epoch_frame::DataFrame SanitizeColumnNames(const epoch_frame::DataFrame& df) const;
};

// Metadata specialization for TableReport
template <> struct ReportMetadata<TableReport> {
  constexpr static const char *kReportId = "table_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Table Report",
      .options = {
        {.id = "sql",
         .name = "SQL Query",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = "SQL query to execute on the input DataFrame"},
        {.id = "table_name",
         .name = "Table Name",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
         .isRequired = false,
         .desc = "Name to use for the input table in SQL query"},
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"SQL Query Result"},
         .isRequired = false,
         .desc = "Title for the generated table"},
        {.id = "add_index",
         .name = "Add Index",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{false},
         .isRequired = false,
         .desc = "Add DataFrame index as a queryable column"},
        {.id = "index_column_name",
         .name = "Index Column Name",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"row_index"},
         .isRequired = false,
         .desc = "Name for the index column when add_index is true"}
      },
      .isCrossSectional = false,
      .desc = "Execute SQL query on input DataFrame and generate table output for tearsheet visualization",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "table", "sql", "query"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports