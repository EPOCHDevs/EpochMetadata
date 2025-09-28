#include "bar_chart_report.h"
#include <epoch_dashboard/tearsheet/bar_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void BarChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    epoch_frame::DataFrame preparedDf = normalizedDf;

    // If SQL query is provided, execute it first
    if (!m_sqlQuery.empty()) {
      preparedDf = PrepareInputDataFrame(normalizedDf);

      // Store original column names before sanitization
      auto originalTable = preparedDf.table();
      auto originalSchema = originalTable->schema();
      std::vector<std::string> originalColumns;
      for (int i = 0; i < originalSchema->num_fields(); ++i) {
        originalColumns.push_back(originalSchema->field(i)->name());
      }

      // Sanitize column names for SQL compatibility
      epoch_frame::DataFrame sanitizedDf = SanitizeColumnNames(preparedDf);

      // Execute SQL query
      auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);
      preparedDf = epoch_frame::DataFrame(resultTable);

      // Restore original column names if needed
      std::unordered_map<std::string, std::string> sanitizedToOriginal;
      for (const auto& origCol : originalColumns) {
        std::regex hashRegex("#");
        std::string sanitized = std::regex_replace(origCol, hashRegex, "_");
        sanitizedToOriginal[sanitized] = origCol;
      }

      auto resultSchema = resultTable->schema();
      std::unordered_map<std::string, std::string> restoreMap;
      for (int i = 0; i < resultSchema->num_fields(); ++i) {
        std::string colName = resultSchema->field(i)->name();
        auto it = sanitizedToOriginal.find(colName);
        if (it != sanitizedToOriginal.end()) {
          restoreMap[colName] = it->second;
        }
      }

      if (!restoreMap.empty()) {
        preparedDf = preparedDf.rename(restoreMap);
      }
    }

    // Validate required columns exist
    auto schema = preparedDf.table()->schema();
    bool hasCategoryColumn = false;
    bool hasValueColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      std::string fieldName = schema->field(i)->name();
      if (fieldName == m_categoryColumn) hasCategoryColumn = true;
      if (fieldName == m_valueColumn) hasValueColumn = true;
    }

    if (!hasCategoryColumn) {
      std::cerr << "Error: BarChartReport - category column '" << m_categoryColumn << "' not found in DataFrame" << std::endl;
      return;
    }

    if (!hasValueColumn) {
      std::cerr << "Error: BarChartReport - value column '" << m_valueColumn << "' not found in DataFrame" << std::endl;
      return;
    }

    // Build the bar chart using BarChartBuilder
    epoch_tearsheet::BarChartBuilder chartBuilder;

    // Set chart title and properties
    chartBuilder.setTitle(m_chartTitle.empty() ? "Bar Chart" : m_chartTitle)
                .setCategory("Charts")
                .setVertical(m_vertical)
                .setStacked(m_stacked);

    if (m_barWidth > 0) {
      chartBuilder.setBarWidth(m_barWidth);
    }

    if (!m_xAxisTitle.empty()) {
      chartBuilder.setXAxisLabel(m_xAxisTitle);
    }

    if (!m_yAxisTitle.empty()) {
      chartBuilder.setYAxisLabel(m_yAxisTitle);
    }

    // Use fromDataFrame to populate the chart
    chartBuilder.fromDataFrame(preparedDf, m_valueColumn);

    // Add chart to dashboard
    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: BarChartReport execution failed: " << e.what() << std::endl;
    return;
  }
}

std::string BarChartReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::string BarChartReport::GetTableName() const {
  auto options = m_config.GetOptions();
  if (options.contains("table_name") && options["table_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["table_name"].GetString();
  }
  return "input";
}

std::string BarChartReport::GetChartTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";
}

std::string BarChartReport::GetCategoryColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("category_column") && options["category_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["category_column"].GetString();
  }
  return "category";
}

std::string BarChartReport::GetValueColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("value_column") && options["value_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["value_column"].GetString();
  }
  return "value";
}

bool BarChartReport::GetVertical() const {
  auto options = m_config.GetOptions();
  if (options.contains("vertical") && options["vertical"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["vertical"].GetBoolean();
  }
  return true;
}

bool BarChartReport::GetStacked() const {
  auto options = m_config.GetOptions();
  if (options.contains("stacked") && options["stacked"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["stacked"].GetBoolean();
  }
  return false;
}

uint32_t BarChartReport::GetBarWidth() const {
  auto options = m_config.GetOptions();
  if (options.contains("bar_width") && options["bar_width"].IsType(epoch_core::MetaDataOptionType::Integer)) {
    return static_cast<uint32_t>(options["bar_width"].GetInteger());
  }
  return 0;
}

std::string BarChartReport::GetXAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("x_axis_title") && options["x_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["x_axis_title"].GetString();
  }
  return "";
}

std::string BarChartReport::GetYAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("y_axis_title") && options["y_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["y_axis_title"].GetString();
  }
  return "";
}

epoch_frame::DataFrame BarChartReport::PrepareInputDataFrame(const epoch_frame::DataFrame& df) const {
  return df;
}

epoch_frame::DataFrame BarChartReport::SanitizeColumnNames(const epoch_frame::DataFrame& df) const {
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