#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class PieChartReport : public IReporter {
public:
  explicit PieChartReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(m_config.GetOptionValue("sql").GetString()),
        m_tableName(m_config.GetOptionValue("table_name").GetString()),
        m_chartTitle(m_config.GetOptionValue("title").GetString()),
        m_labelColumn(m_config.GetOptionValue("label_column").GetString()),
        m_valueColumn(m_config.GetOptionValue("value_column").GetString()),
        m_innerSize(static_cast<uint32_t>(m_config.GetOptionValue("inner_size").GetInteger())),
        m_addIndex(m_config.GetOptionValue("add_index").GetBoolean()),
        m_indexColumnName(m_config.GetOptionValue("index_column_name").GetString()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  const std::string m_sqlQuery;
  const std::string m_tableName;
  const std::string m_chartTitle;
  const std::string m_labelColumn;
  const std::string m_valueColumn;
  const uint32_t m_innerSize;
  const bool m_addIndex;
  const std::string m_indexColumnName;

};

template <> struct ReportMetadata<PieChartReport> {
  constexpr static const char *kReportId = "pie_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Pie Chart Report",
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
        {.id = "label_column",
         .name = "Label Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"label"},
         .isRequired = false,
         .desc = "Column name for slice labels (default: 'label')"},
        {.id = "value_column",
         .name = "Value Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"value"},
         .isRequired = false,
         .desc = "Column name for slice values (default: 'value')"},
        {.id = "inner_size",
         .name = "Inner Size",
         .type = epoch_core::MetaDataOptionType::Integer,
         .isRequired = false,
         .min = 0,
         .max = 100,
         .desc = "Inner radius percentage for donut chart (0 for pie, 50 for donut)"},
        {.id = "add_index",
         .name = "Add Index",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{true},
         .isRequired = false,
         .desc = "Add DataFrame index as a queryable column for SQL queries"},
        {.id = "index_column_name",
         .name = "Index Column Name",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
         .isRequired = false,
         .desc = "Name for the index column when add_index is true"}
      },
      .isCrossSectional = false,
      .desc = "Generates pie/donut chart from DataFrame. Required columns: label, value",
      .inputs = {
        {epoch_core::IODataType::Any, epoch_metadata::ARG, "", true}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "pie", "donut", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports