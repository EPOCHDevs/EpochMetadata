#pragma once

#include <epoch_frame/dataframe.h>
#include <epoch_frame/index.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_dashboard/tearsheet/chart_types.h>
#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <regex>

namespace epoch_script::reports {

// Shared utility functions for all report types
class ReportUtils {
public:
  // Convert a Series to vector of PieData for pie chart rendering
  static std::vector<epoch_proto::PieData> createPieDataFromSeries(const epoch_frame::Series& series) {
    std::vector<epoch_proto::PieData> pieData;
    for (int64_t i = 0; i < static_cast<int64_t>(series.size()); ++i) {
      epoch_proto::PieData data;
      data.set_name(series.index()->at(i).repr());
      data.set_y(series.iloc(i).as_double());
      pieData.emplace_back(std::move(data));
    }
    return pieData;
  }

  // Group by column, sum values, and normalize as percentage
  // Preserves the original order of appearance in the DataFrame
  static epoch_frame::Series normalizeSeriesAsPercentage(
      const epoch_frame::DataFrame& df,
      const std::string& groupColumn,
      const std::string& valueColumn) {
    // First, aggregate using group_by (this will sort alphabetically)
    auto grouped = df[{groupColumn, valueColumn}]
                      .group_by_agg(groupColumn)
                      .sum()
                      .to_series();
    auto total = df[valueColumn].sum();
    auto percentage_series = (grouped / total) * epoch_frame::Scalar{100.0};

    // Create a map of label to percentage for quick lookup
    std::map<std::string, double> label_to_percentage;
    for (int64_t i = 0; i < static_cast<int64_t>(percentage_series.size()); ++i) {
      label_to_percentage[percentage_series.index()->at(i).repr()] = percentage_series.iloc(i).as_double();
    }

    // Now iterate through original DataFrame to preserve order
    std::vector<std::string> ordered_labels;
    std::vector<double> ordered_values;
    std::set<std::string> seen_labels;

    auto label_series = df[groupColumn];
    for (int64_t i = 0; i < static_cast<int64_t>(label_series.size()); ++i) {
      std::string label = label_series.iloc(i).repr();
      if (seen_labels.find(label) == seen_labels.end()) {
        seen_labels.insert(label);
        ordered_labels.push_back(label);
        ordered_values.push_back(label_to_percentage[label]);
      }
    }

    // Build a new Series with the original order
    auto index = epoch_frame::factory::index::make_object_index(ordered_labels);
    return epoch_frame::make_series(index, ordered_values);
  }

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
                                                           const std::string& sqlQuery) {
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
    auto resultTable = sanitizedDf.query(sqlQuery);
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

} // namespace epoch_script::reports