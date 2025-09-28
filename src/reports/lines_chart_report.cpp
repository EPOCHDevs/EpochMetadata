#include "lines_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/lines_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>

namespace epoch_metadata::reports {

void LinesChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    epoch_frame::DataFrame preparedDf = normalizedDf;

    // If SQL query is provided, execute it first
    if (!m_sqlQuery.empty()) {
      preparedDf = ReportUtils::PrepareIndexColumn(normalizedDf, m_useIndex, m_indexColumnName);
      // Use shared utility for SQL execution with sanitization
      preparedDf = ReportUtils::ExecuteSQLWithSanitization(preparedDf, m_sqlQuery, m_tableName);
    } else if (m_useIndex || !m_indexColumnName.empty()) {
      // If no SQL but need to use index, prepare the DataFrame
      preparedDf = ReportUtils::PrepareIndexColumn(normalizedDf, m_useIndex, m_indexColumnName);
    }

    // Determine x and y column names
    std::string xColumn = m_xAxisColumn;
    std::string yColumn = m_yValueColumn;

    // Override x column if index_column is specified
    if (!m_indexColumnName.empty()) {
      xColumn = m_indexColumnName;
    }

    // Validate required columns exist
    auto schema = preparedDf.table()->schema();
    bool hasXColumn = false;
    bool hasYColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      std::string fieldName = schema->field(i)->name();
      if (fieldName == xColumn) hasXColumn = true;
      if (fieldName == yColumn) hasYColumn = true;
    }

    if (!hasXColumn) {
      std::cerr << "Error: LinesChartReport - x-axis column '" << xColumn << "' not found in DataFrame" << std::endl;
      return;
    }

    if (!hasYColumn) {
      std::cerr << "Error: LinesChartReport - y-value column '" << yColumn << "' not found in DataFrame" << std::endl;
      return;
    }

    // Build the lines chart using LinesChartBuilder
    epoch_tearsheet::LinesChartBuilder chartBuilder;

    // Set chart title and axis titles
    chartBuilder.setTitle(m_chartTitle.empty() ? "Line Chart" : m_chartTitle)
                .setCategory(m_category);

    if (!m_xAxisTitle.empty()) {
      chartBuilder.setXAxisLabel(m_xAxisTitle);
    }

    if (!m_yAxisTitle.empty()) {
      chartBuilder.setYAxisLabel(m_yAxisTitle);
    }

    // Use fromDataFrame to populate the chart with single line
    std::vector<std::string> yColumns = {yColumn};
    chartBuilder.fromDataFrame(preparedDf, yColumns);

    // Add chart to dashboard
    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: LinesChartReport execution failed: " << e.what() << std::endl;
    return;
  }
}



} // namespace epoch_metadata::reports