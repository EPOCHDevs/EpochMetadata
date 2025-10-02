#include "bar_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/bar_chart_builder.h>
#include "epoch_dashboard/tearsheet/scalar_converter.h"
#include <epoch_frame/dataframe.h>
#include <regex>

namespace epoch_metadata::reports {

void BarChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  // Get column names from input mapping
  auto labelColumn = m_config.GetInput("label");
  auto valueColumn = m_config.GetInput("value");

  // Group by label and aggregate values
  auto df = normalizedDf[{labelColumn, valueColumn}];
  auto grouped = df.group_by_agg(labelColumn)
                   .agg(epoch_core::BarChartAggWrapper::ToString(m_agg))
                   .to_series();

  // Build categories and data arrays from grouped series
  std::vector<std::string> categories;
  epoch_proto::Array data;
  for (int64_t i = 0; i < static_cast<int64_t>(grouped.size()); ++i) {
    categories.emplace_back(grouped.index()->at(i).repr());
    *data.add_values() = epoch_tearsheet::ScalarFactory::create(grouped.iloc(i));
  }

  // Build the bar chart using BarChartBuilder
  epoch_tearsheet::BarChartBuilder chartBuilder;
  chartBuilder.setTitle(m_chartTitle)
              .setCategory(m_category)
              .setVertical(m_vertical)
              .setStacked(false)
              .setYAxisType(epoch_proto::AxisLinear)
              .setYAxisLabel(m_yAxisLabel)
              .setXAxisType(epoch_proto::AxisCategory)
              .setXAxisLabel(m_xAxisLabel)
              .setXAxisCategories(categories)
              .setData(data);

  // Add chart to dashboard
  auto chart = chartBuilder.build();
  std::cerr << "DEBUG BarChartReport: built chart with type_case=" << chart.chart_type_case()
            << " (expected kBarDef=3)" << std::endl;
  m_dashboard.addChart(chart);
}





} // namespace epoch_metadata::reports