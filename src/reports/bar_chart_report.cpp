#include "bar_chart_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/bar_chart_builder.h>
#include "epoch_dashboard/tearsheet/scalar_converter.h"
#include <epoch_frame/dataframe.h>
#include <regex>
#include <set>
#include <map>

namespace epoch_metadata::reports {

void BarChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  // Get column names from input mapping
  auto labelColumn = m_config.GetInput("label");
  auto valueColumn = m_config.GetInput("value");

  // Group by label and aggregate values - preserve original order
  auto df = normalizedDf[{labelColumn, valueColumn}];
  auto grouped = df.group_by_agg(labelColumn)
                   .agg(epoch_core::BarChartAggWrapper::ToString(m_agg))
                   .to_series();

  // Build categories and data arrays from grouped series
  // We need to preserve the original order of appearance, not sorted order
  // Create a map to store aggregated values by label
  std::map<std::string, epoch_proto::Scalar> value_map;
  for (int64_t i = 0; i < static_cast<int64_t>(grouped.size()); ++i) {
    value_map[grouped.index()->at(i).repr()] = epoch_tearsheet::ScalarFactory::create(grouped.iloc(i));
  }

  // Iterate through original dataframe to preserve order
  std::vector<std::string> categories;
  epoch_proto::Array data;
  std::set<std::string> seen_labels;

  auto label_series = df[labelColumn];
  for (int64_t i = 0; i < static_cast<int64_t>(label_series.size()); ++i) {
    std::string label = label_series.iloc(i).repr();
    if (seen_labels.find(label) == seen_labels.end()) {
      seen_labels.insert(label);
      categories.push_back(label);
      *data.add_values() = value_map[label];
    }
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
  m_dashboard.addChart(chart);
}





} // namespace epoch_metadata::reports