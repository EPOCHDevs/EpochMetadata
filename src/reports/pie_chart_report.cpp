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
    // Get column names from input mapping
    auto labelColumn = m_config.GetInput("label");
    auto valueColumn = m_config.GetInput("value");

    // Normalize series as percentage using utility function
    auto normalizedSeries = ReportUtils::normalizeSeriesAsPercentage(
        normalizedDf, labelColumn, valueColumn);

    // Build simple pie chart directly from data
    epoch_tearsheet::PieChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle)
                .setCategory(m_category);

    // Convert series to pie data using utility function
    auto pieData = ReportUtils::createPieDataFromSeries(normalizedSeries);
    chartBuilder.addSeries(labelColumn, pieData, epoch_tearsheet::PieSize{100}, std::nullopt);

    auto chart = chartBuilder.build();
    m_dashboard.addChart(chart);

  } catch (const std::exception& e) {
    std::cerr << "Error: PieChartReport execution failed: " << e.what() << std::endl;
  }
}



} // namespace epoch_metadata::reports