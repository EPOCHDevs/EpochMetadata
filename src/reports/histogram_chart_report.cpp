#include "histogram_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/histogram_chart_builder.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {
  void HistogramChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
    auto valuesColumn = m_config.GetInput("value");

    // Build histogram chart
    epoch_tearsheet::HistogramChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle.empty() ? "Histogram" : m_chartTitle)
                .setCategory(m_category)
                .setBinsCount(m_bins)
                .setXAxisLabel(m_xAxisLabel)
                .setYAxisLabel(m_yAxisLabel);

    // Use fromDataFrame to populate chart
    chartBuilder.fromDataFrame(normalizedDf, valuesColumn, m_bins);

    m_dashboard.addChart(chartBuilder.build());
  }
} // namespace epoch_metadata::reports