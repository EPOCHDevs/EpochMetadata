#pragma once

#include <epoch_frame/dataframe.h>
#include <string>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

// Shared utility functions for all report types
class ReportUtils {
public:
  // Sanitize column names by replacing # with _ for SQL compatibility
  static epoch_frame::DataFrame SanitizeColumnNames(const epoch_frame::DataFrame& df) {
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

  // Prepare DataFrame with index column if needed
  static epoch_frame::DataFrame PrepareIndexColumn(const epoch_frame::DataFrame& df,
                                                   bool useIndex,
                                                   const std::string& indexColName) {
    if (useIndex) {
      std::string targetColName = indexColName.empty() ? "row_index" : indexColName;

      // Check if the column already exists to avoid duplicates
      auto table = df.table();
      auto schema = table->schema();
      for (int i = 0; i < schema->num_fields(); ++i) {
        if (schema->field(i)->name() == targetColName) {
          // Column already exists, don't add index
          return df;
        }
      }

      // Add index as a column for access
      try {
        return df.reset_index(targetColName);
      } catch (const std::exception& e) {
        std::cerr << "Warning: Could not add index column '" << targetColName << "': " << e.what() << std::endl;
        return df;
      }
    }
    return df;
  }

  // Execute SQL query with column name sanitization
  static epoch_frame::DataFrame ExecuteSQLWithSanitization(const epoch_frame::DataFrame& df,
                                                           const std::string& sqlQuery,
                                                           const std::string& tableName) {
    // Store original column names before sanitization
    auto originalTable = df.table();
    auto originalSchema = originalTable->schema();
    std::vector<std::string> originalColumns;
    for (int i = 0; i < originalSchema->num_fields(); ++i) {
      originalColumns.push_back(originalSchema->field(i)->name());
    }

    // Sanitize column names for SQL compatibility
    epoch_frame::DataFrame sanitizedDf = SanitizeColumnNames(df);

    // Execute SQL query
    auto resultTable = sanitizedDf.query(sqlQuery, tableName);
    epoch_frame::DataFrame resultDf(resultTable);

    // Build map for restoration
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
      auto it = sanitizedToOriginal.find(colName);
      if (it != sanitizedToOriginal.end()) {
        restoreMap[colName] = it->second;
      }
    }

    // Apply restoration if needed
    if (!restoreMap.empty()) {
      resultDf = resultDf.rename(restoreMap);
    }

    return resultDf;
  }
};

} // namespace epoch_metadata::reports