#include "area_chart_report.h"
#include <epoch_dashboard/tearsheet/area_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void AreaChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    epoch_frame::DataFrame preparedDf = normalizedDf;

    // Execute SQL if provided
    if (!m_sqlQuery.empty()) {
      auto sanitizedDf = SanitizeColumnNames(preparedDf);
      auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);
      preparedDf = epoch_frame::DataFrame(resultTable);
    }

    // Validate required columns
    auto schema = preparedDf.table()->schema();
    bool hasXColumn = false;
    bool hasYColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      std::string fieldName = schema->field(i)->name();
      if (fieldName == m_xAxisColumn) hasXColumn = true;
      if (fieldName == m_yValueColumn) hasYColumn = true;
    }

    if (!hasXColumn || !hasYColumn) {
      std::cerr << "Error: AreaChartReport - required columns not found" << std::endl;
      return;
    }

    // Build area chart
    epoch_tearsheet::AreaChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle.empty() ? "Area Chart" : m_chartTitle)
                .setCategory("Charts")
                .setStacked(m_stacked);

    if (!m_xAxisTitle.empty()) {
      chartBuilder.setXAxisLabel(m_xAxisTitle);
    }

    if (!m_yAxisTitle.empty()) {
      chartBuilder.setYAxisLabel(m_yAxisTitle);
    }

    // Use fromDataFrame with single y column for now
    std::vector<std::string> yColumns = {m_yValueColumn};
    chartBuilder.fromDataFrame(preparedDf, yColumns);

    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: AreaChartReport execution failed: " << e.what() << std::endl;
  }
}

std::string AreaChartReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::string AreaChartReport::GetTableName() const {
  auto options = m_config.GetOptions();
  if (options.contains("table_name") && options["table_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["table_name"].GetString();
  }
  return "input";
}

std::string AreaChartReport::GetChartTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";
}

std::string AreaChartReport::GetXAxisColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("x_axis_column") && options["x_axis_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["x_axis_column"].GetString();
  }
  return "x_axis";
}

std::string AreaChartReport::GetYValueColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("y_value_column") && options["y_value_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["y_value_column"].GetString();
  }
  return "y_value";
}

bool AreaChartReport::GetStacked() const {
  auto options = m_config.GetOptions();
  if (options.contains("stacked") && options["stacked"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["stacked"].GetBoolean();
  }
  return false;
}

std::string AreaChartReport::GetXAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("x_axis_title") && options["x_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["x_axis_title"].GetString();
  }
  return "";
}

std::string AreaChartReport::GetYAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("y_axis_title") && options["y_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["y_axis_title"].GetString();
  }
  return "";
}

epoch_frame::DataFrame AreaChartReport::SanitizeColumnNames(const epoch_frame::DataFrame& df) const {
  auto table = df.table();
  auto schema = table->schema();
  std::unordered_map<std::string, std::string> renameMap;

  for (int i = 0; i < schema->num_fields(); ++i) {
    std::string colName = schema->field(i)->name();
    std::regex hashRegex("#");
    std::string sanitizedName = std::regex_replace(colName, hashRegex, "_");
    if (colName != sanitizedName) {
      renameMap[colName] = sanitizedName;
    }
  }

  return renameMap.empty() ? df : df.rename(renameMap);
}

} // namespace epoch_metadata::reports