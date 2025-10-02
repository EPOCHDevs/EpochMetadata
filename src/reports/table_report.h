#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_metadata/transforms/sql_options.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class TableReport : public IReporter {
public:
  explicit TableReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_tableTitle(GetTableTitle()),
        m_addIndex(GetAddIndex()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;


private:
  // Cached configuration values
  const std::string m_sqlQuery;
  const std::string m_tableTitle;
  const bool m_addIndex;

  // Configuration getters
  std::string GetSQLQuery() const;
  bool GetAddIndex() const;
  std::string GetTableTitle() const;

  // Helper methods
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
        epoch_metadata::transforms::SQL_OPTION,
        epoch_metadata::transforms::ADD_INDEX_OPTION,
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"SQL Query Result"},
         .isRequired = false,
         .desc = "Title for the generated table"}
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