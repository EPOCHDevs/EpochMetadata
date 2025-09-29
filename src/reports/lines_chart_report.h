#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class LinesChartReport : public IReporter {
public:
  explicit LinesChartReport(const epoch_metadata::transform::TransformConfiguration &config)
      : IReporter(config, true),
        m_sqlQuery(m_config.GetOptionValue("sql").GetString()),
        m_tableName(m_config.GetOptionValue("table_name").GetString()),
        m_chartTitle(m_config.GetOptionValue("title").GetString()),
        m_category(m_config.GetOptionValue("category").GetString()),
        m_useIndex(m_config.GetOptionValue("use_index").GetBoolean()),
        m_indexColumnName(m_config.GetOptionValue("index_column").GetString()),
        m_xAxisColumn(m_config.GetOptionValue("x_axis_column").GetString()),
        m_yValueColumn(m_config.GetOptionValue("y_value_column").GetString()),
        m_xAxisTitle(m_config.GetOptionValue("x_axis_title").GetString()),
        m_yAxisTitle(m_config.GetOptionValue("y_axis_title").GetString()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  const std::string m_sqlQuery;
  const std::string m_tableName;
  const std::string m_chartTitle;
  const std::string m_category;
  const bool m_useIndex;
  const std::string m_indexColumnName;
  const std::string m_xAxisColumn;
  const std::string m_yValueColumn;
  const std::string m_xAxisTitle;
  const std::string m_yAxisTitle;

};

template <> struct ReportMetadata<LinesChartReport> {
  constexpr static const char *kReportId = "lines_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Lines Chart Report",
      .options = {
        {.id = "sql",
         .name = "SQL Query",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Optional SQL query to transform input DataFrame before charting"},
        {.id = "table_name",
         .name = "Table Name",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"input"},
         .isRequired = false,
         .desc = "Name to use for the input table in SQL query"},
        {.id = "title",
         .name = "Chart Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Title for the generated chart"},
        {.id = "category",
         .name = "Chart Category",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"Charts"},
         .isRequired = false,
         .desc = "Category for the chart (default: 'Charts')"},
        {.id = "use_index",
         .name = "Use Index",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .isRequired = false,
         .desc = "Use DataFrame index as x_axis instead of x_axis column"},
        {.id = "index_column",
         .name = "Index Column Name",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Alternative column name to use as x_axis (overrides 'x_axis')"},
        {.id = "x_axis_column",
         .name = "X Axis Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"x_axis"},
         .isRequired = false,
         .desc = "Column name for x-axis data (default: 'x_axis')"},
        {.id = "y_value_column",
         .name = "Y Value Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"y_value"},
         .isRequired = false,
         .desc = "Column name for y-axis values (default: 'y_value')"},
        {.id = "x_axis_title",
         .name = "X Axis Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Title for the x-axis"},
        {.id = "y_axis_title",
         .name = "Y Axis Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Title for the y-axis"}
      },
      .isCrossSectional = false,
      .desc = "Generates line chart from DataFrame. Required columns: x_axis (timestamp/int64), y_value. Option to use DataFrame index as x_axis.",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "lines", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports