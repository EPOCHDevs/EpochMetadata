#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class BarChartReport : public IReporter {
public:
  explicit BarChartReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_tableName(GetTableName()),
        m_chartTitle(GetChartTitle()),
        m_categoryColumn(GetCategoryColumn()),
        m_valueColumn(GetValueColumn()),
        m_vertical(GetVertical()),
        m_stacked(GetStacked()),
        m_barWidth(GetBarWidth()),
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
  const std::string m_categoryColumn;
  const std::string m_valueColumn;
  const bool m_vertical;
  const bool m_stacked;
  const uint32_t m_barWidth;
  const std::string m_xAxisTitle;
  const std::string m_yAxisTitle;

  std::string GetSQLQuery() const;
  std::string GetTableName() const;
  std::string GetChartTitle() const;
  std::string GetCategoryColumn() const;
  std::string GetValueColumn() const;
  bool GetVertical() const;
  bool GetStacked() const;
  uint32_t GetBarWidth() const;
  std::string GetXAxisTitle() const;
  std::string GetYAxisTitle() const;

  epoch_frame::DataFrame PrepareInputDataFrame(const epoch_frame::DataFrame& df) const;
  epoch_frame::DataFrame SanitizeColumnNames(const epoch_frame::DataFrame& df) const;
};

template <> struct ReportMetadata<BarChartReport> {
  constexpr static const char *kReportId = "bar_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Bar Chart Report",
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
        {.id = "category_column",
         .name = "Category Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"category"},
         .isRequired = false,
         .desc = "Column name for categories (default: 'category')"},
        {.id = "value_column",
         .name = "Value Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"value"},
         .isRequired = false,
         .desc = "Column name for bar values (default: 'value')"},
        {.id = "vertical",
         .name = "Vertical Bars",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .isRequired = false,
         .desc = "Use vertical bars (true) or horizontal bars (false)"},
        {.id = "stacked",
         .name = "Stacked",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .isRequired = false,
         .desc = "Stack bars for multiple series"},
        {.id = "bar_width",
         .name = "Bar Width",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = false,
         .min = 0,
         .max = 1000,
         .desc = "Width of bars in pixels (0 for auto)"},
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
      .desc = "Generates bar chart from DataFrame. Required columns: category, value",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "bar", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports