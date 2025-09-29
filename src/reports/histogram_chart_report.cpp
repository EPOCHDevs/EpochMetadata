#include "histogram_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/histogram_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void HistogramChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
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


} // namespace epoch_metadata::reports