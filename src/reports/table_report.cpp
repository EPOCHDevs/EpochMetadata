#include "table_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/table_builder.h>
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void TableReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  if (m_sqlQuery.empty()) {
    std::cerr << "Warning: TableReport requires 'sql' option" << std::endl;
    return;
  }

  try {
    // Prepare DataFrame for SQL execution (with optional index column)
    epoch_frame::DataFrame preparedDf = normalizedDf;
    if (m_addIndex) {
      std::string targetColName = m_indexColumnName.empty() ? "row_index" : m_indexColumnName;
      // Check if the column already exists to avoid duplicates
      auto table = preparedDf.table();
      auto schema = table->schema();
      bool columnExists = false;
      for (int i = 0; i < schema->num_fields(); ++i) {
        if (schema->field(i)->name() == targetColName) {
          columnExists = true;
          break;
        }
      }
      if (!columnExists) {
        try {
          preparedDf = preparedDf.reset_index(targetColName);
        } catch (const std::exception& e) {
          std::cerr << "Warning: Could not add index column '" << targetColName << "': " << e.what() << std::endl;
        }
      }
    }

    // Store original column names before sanitization
    auto originalTable = preparedDf.table();
    auto originalSchema = originalTable->schema();
    std::vector<std::string> originalColumns;
    for (int i = 0; i < originalSchema->num_fields(); ++i) {
      std::string colName = originalSchema->field(i)->name();
      originalColumns.push_back(colName);
    }

    // Sanitize column names for SQL compatibility and execute query
    epoch_frame::DataFrame sanitizedDf = reports::ReportUtils::SanitizeColumnNames(preparedDf);

    // Execute SQL query on sanitized DataFrame
    auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);
    epoch_frame::DataFrame resultDf(resultTable);


    // For table reports, we keep the sanitized column names since SQL operates on them
    // and test expectations are based on sanitized names

    // Build protobuf Table using TableBuilder
    epoch_tearsheet::TableBuilder tableBuilder;
    tableBuilder.setTitle(m_tableTitle.empty() ? "SQL Query Result" : m_tableTitle)
                .setCategory("Reports")
                .setType(epoch_proto::WidgetDataTable)
                .fromDataFrame(resultDf);

    // Add table to dashboard
    m_dashboard.addTable(tableBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: TableReport SQL execution failed: " << e.what() << std::endl;
    return;
  }
}

std::string TableReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::string TableReport::GetTableName() const {
  auto options = m_config.GetOptions();
  if (options.contains("table_name") && options["table_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["table_name"].GetString();
  }
  return "input"; // Default table name for SQL
}

bool TableReport::GetAddIndex() const {
  auto options = m_config.GetOptions();
  if (options.contains("add_index") && options["add_index"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["add_index"].GetBoolean();
  }
  return false; // Default to not adding index
}

std::string TableReport::GetIndexColumnName() const {
  auto options = m_config.GetOptions();
  if (options.contains("index_column_name") && options["index_column_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["index_column_name"].GetString();
  }
  return "row_index"; // Default index column name
}

std::string TableReport::GetTableTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return ""; // Empty means auto-generate
}



} // namespace epoch_metadata::reports