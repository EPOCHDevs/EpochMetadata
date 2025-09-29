#include "bar_chart_report.h"
#include "report_utils.h"
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
      // Prepare DataFrame with optional index column for SQL access
      epoch_frame::DataFrame indexPreparedDf = ReportUtils::PrepareIndexColumn(normalizedDf, m_addIndex, m_indexColumnName);

      // Execute SQL with sanitization
      preparedDf = ReportUtils::ExecuteSQLWithSanitization(indexPreparedDf, m_sqlQuery, m_tableName);
    } else {
      // For non-SQL queries, use the sanitized column names directly
      preparedDf = ReportUtils::SanitizeColumnNames(normalizedDf);
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





} // namespace epoch_metadata::reports