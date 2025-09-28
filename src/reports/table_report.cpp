#include "table_report.h"
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
    epoch_frame::DataFrame preparedDf = PrepareInputDataFrame(normalizedDf, m_addIndex, m_indexColumnName);

    // Store original column names before sanitization
    auto originalTable = preparedDf.table();
    auto originalSchema = originalTable->schema();
    std::vector<std::string> originalColumns;
    for (int i = 0; i < originalSchema->num_fields(); ++i) {
      originalColumns.push_back(originalSchema->field(i)->name());
    }

    // Sanitize column names by replacing # with _ for SQL compatibility
    epoch_frame::DataFrame sanitizedDf = SanitizeColumnNames(preparedDf);

    // Execute SQL using DataFrame::query method (SQL should use underscore column names)
    auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);

    // Convert Arrow Table to DataFrame
    epoch_frame::DataFrame resultDf(resultTable);

    // Restore original column names with # in the result
    // Build a map of sanitized to original names
    std::unordered_map<std::string, std::string> sanitizedToOriginal;
    for (const auto& origCol : originalColumns) {
      std::regex hashRegex("#");
      std::string sanitized = std::regex_replace(origCol, hashRegex, "_");
      sanitizedToOriginal[sanitized] = origCol;
    }

    // Check result columns and restore where applicable
    auto resultSchema = resultTable->schema();
    std::unordered_map<std::string, std::string> restoreMap;
    for (int i = 0; i < resultSchema->num_fields(); ++i) {
      std::string colName = resultSchema->field(i)->name();

      // If this column name exactly matches a sanitized original column, restore it
      auto it = sanitizedToOriginal.find(colName);
      if (it != sanitizedToOriginal.end()) {
        restoreMap[colName] = it->second;
      }
    }

    // Apply restoration if needed
    if (!restoreMap.empty()) {
      resultDf = resultDf.rename(restoreMap);
    }

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


epoch_frame::DataFrame TableReport::PrepareInputDataFrame(const epoch_frame::DataFrame& df,
                                                          bool addIndex,
                                                          const std::string& indexColName) const {
  if (addIndex) {
    // Check if the column name already exists to avoid duplicates
    auto table = df.table();
    auto schema = table->schema();
    for (int i = 0; i < schema->num_fields(); ++i) {
      if (schema->field(i)->name() == indexColName) {
        // Column already exists, don't add index
        return df;
      }
    }

    // Add index as a column for SQL access
    try {
      return df.reset_index(indexColName);
    } catch (const std::exception& e) {
      std::cerr << "Warning: Could not add index column '" << indexColName << "': " << e.what() << std::endl;
      return df;
    }
  }
  return df;
}

epoch_frame::DataFrame TableReport::SanitizeColumnNames(const epoch_frame::DataFrame& df) const {
  // Get column names from table schema
  auto table = df.table();
  auto schema = table->schema();
  std::unordered_map<std::string, std::string> renameMap;

  for (int i = 0; i < schema->num_fields(); ++i) {
    std::string colName = schema->field(i)->name();
    // Replace # with _
    std::regex hashRegex("#");
    std::string sanitizedName = std::regex_replace(colName, hashRegex, "_");
    if (colName != sanitizedName) {
      renameMap[colName] = sanitizedName;
    }
  }

  // Only rename if there are columns to rename
  if (renameMap.empty()) {
    return df;
  }

  return df.rename(renameMap);
}

} // namespace epoch_metadata::reports