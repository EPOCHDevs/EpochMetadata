#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class AreaChartReport : public IReporter {
public:
  explicit AreaChartReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_tableName(GetTableName()),
        m_chartTitle(GetChartTitle()),
        m_xAxisColumn(GetXAxisColumn()),
        m_yValueColumn(GetYValueColumn()),
        m_stacked(GetStacked()),
        m_xAxisTitle(GetXAxisTitle()),
        m_yAxisTitle(GetYAxisTitle()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  void generateDashboard(const epoch_frame::DataFrame &normalizedDf) const override {
    generateTearsheet(normalizedDf);
  }

private:
  const std::string m_sqlQuery;
  const std::string m_tableName;
  const std::string m_chartTitle;
  const std::string m_xAxisColumn;
  const std::string m_yValueColumn;
  const bool m_stacked;
  const std::string m_xAxisTitle;
  const std::string m_yAxisTitle;

  std::string GetSQLQuery() const;
  std::string GetTableName() const;
  std::string GetChartTitle() const;
  std::string GetXAxisColumn() const;
  std::string GetYValueColumn() const;
  bool GetStacked() const;
  std::string GetXAxisTitle() const;
  std::string GetYAxisTitle() const;

  epoch_frame::DataFrame SanitizeColumnNames(const epoch_frame::DataFrame& df) const;
};

template <> struct ReportMetadata<AreaChartReport> {
  constexpr static const char *kReportId = "area_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Area Chart Report",
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
        {.id = "stacked",
         .name = "Stacked",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .isRequired = false,
         .desc = "Stack areas for multiple series"},
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
      .desc = "Generates area chart from DataFrame. Required columns: x_axis, y_value",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "area", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports