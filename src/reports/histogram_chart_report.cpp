#include "histogram_chart_report.h"
#include <epoch_dashboard/tearsheet/histogram_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void HistogramChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    epoch_frame::DataFrame preparedDf = normalizedDf;

    // Execute SQL if provided
    if (!m_sqlQuery.empty()) {
      auto sanitizedDf = SanitizeColumnNames(preparedDf);
      auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);
      preparedDf = epoch_frame::DataFrame(resultTable);
    }

    // Validate required column
    auto schema = preparedDf.table()->schema();
    bool hasValuesColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      if (schema->field(i)->name() == m_valuesColumn) {
        hasValuesColumn = true;
        break;
      }
    }

    if (!hasValuesColumn) {
      std::cerr << "Error: HistogramChartReport - values column '" << m_valuesColumn << "' not found" << std::endl;
      return;
    }

    // Build histogram chart
    epoch_tearsheet::HistogramChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle.empty() ? "Histogram" : m_chartTitle)
                .setCategory("Charts")
                .setBinsCount(m_bins);

    if (!m_xAxisTitle.empty()) {
      chartBuilder.setXAxisLabel(m_xAxisTitle);
    }

    if (!m_yAxisTitle.empty()) {
      chartBuilder.setYAxisLabel(m_yAxisTitle);
    }

    // Use fromDataFrame to populate chart
    chartBuilder.fromDataFrame(preparedDf, m_valuesColumn, m_bins);

    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: HistogramChartReport execution failed: " << e.what() << std::endl;
  }
}

std::string HistogramChartReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::string HistogramChartReport::GetTableName() const {
  auto options = m_config.GetOptions();
  if (options.contains("table_name") && options["table_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["table_name"].GetString();
  }
  return "input";
}

std::string HistogramChartReport::GetChartTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";
}

std::string HistogramChartReport::GetValuesColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("values_column") && options["values_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["values_column"].GetString();
  }
  return "values";
}

uint32_t HistogramChartReport::GetBins() const {
  auto options = m_config.GetOptions();
  if (options.contains("bins") && options["bins"].IsType(epoch_core::MetaDataOptionType::Integer)) {
    return static_cast<uint32_t>(options["bins"].GetInteger());
  }
  return 30;
}

std::string HistogramChartReport::GetXAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("x_axis_title") && options["x_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["x_axis_title"].GetString();
  }
  return "";
}

std::string HistogramChartReport::GetYAxisTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("y_axis_title") && options["y_axis_title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["y_axis_title"].GetString();
  }
  return "";
}

epoch_frame::DataFrame HistogramChartReport::SanitizeColumnNames(const epoch_frame::DataFrame& df) const {
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