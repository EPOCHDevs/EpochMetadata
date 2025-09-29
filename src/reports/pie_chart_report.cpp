#include "pie_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/pie_chart_builder.h>
#include <epoch_dashboard/tearsheet/chart_types.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void PieChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
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

    // Validate required columns
    auto schema = preparedDf.table()->schema();
    bool hasLabelColumn = false;
    bool hasValueColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      std::string fieldName = schema->field(i)->name();
      if (fieldName == m_labelColumn) hasLabelColumn = true;
      if (fieldName == m_valueColumn) hasValueColumn = true;
    }

    if (!hasLabelColumn || !hasValueColumn) {
      std::cerr << "Error: PieChartReport - required columns not found" << std::endl;
      return;
    }

    // Build pie chart
    epoch_tearsheet::PieChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle.empty() ? "Pie Chart" : m_chartTitle)
                .setCategory("Charts");

    // Configure pie size and inner size for donut
    epoch_tearsheet::PieSize size(100);  // 100% size
    std::optional<epoch_tearsheet::PieInnerSize> innerSize;
    if (m_innerSize > 0) {
      innerSize = epoch_tearsheet::PieInnerSize(m_innerSize);
    }

    // Use fromDataFrame to populate chart
    chartBuilder.fromDataFrame(preparedDf, m_labelColumn, m_valueColumn, "Series", size, innerSize);

    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: PieChartReport execution failed: " << e.what() << std::endl;
  }
}



} // namespace epoch_metadata::reports