#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class HistogramChartReport : public IReporter {
public:
  explicit HistogramChartReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_tableName(GetTableName()),
        m_chartTitle(GetChartTitle()),
        m_valuesColumn(GetValuesColumn()),
        m_bins(GetBins()),
        m_xAxisTitle(GetXAxisTitle()),
        m_yAxisTitle(GetYAxisTitle()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  const std::string m_sqlQuery;
  const std::string m_tableName;
  const std::string m_chartTitle;
  const std::string m_valuesColumn;
  const uint32_t m_bins;
  const std::string m_xAxisTitle;
  const std::string m_yAxisTitle;

  std::string GetSQLQuery() const;
  std::string GetTableName() const;
  std::string GetChartTitle() const;
  std::string GetValuesColumn() const;
  uint32_t GetBins() const;
  std::string GetXAxisTitle() const;
  std::string GetYAxisTitle() const;

  epoch_frame::DataFrame SanitizeColumnNames(const epoch_frame::DataFrame& df) const;
};

template <> struct ReportMetadata<HistogramChartReport> {
  constexpr static const char *kReportId = "histogram_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Histogram Chart Report",
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
        {.id = "values_column",
         .name = "Values Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"values"},
         .isRequired = false,
         .desc = "Column name for values to histogram (default: 'values')"},
        {.id = "bins",
         .name = "Number of Bins",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = false,
         .min = 1,
         .max = 100,
         .desc = "Number of bins for the histogram (default: 30)"},
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
      .desc = "Generates histogram from DataFrame. Required column: values",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "histogram", "distribution", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports