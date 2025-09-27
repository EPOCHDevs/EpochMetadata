#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_dashboard/tearsheet/table_builder.h>
#include <epoch_dashboard/tearsheet/dataframe_converter.h>
#include <epoch_frame/dataframe.h>

namespace epoch_metadata::reports {

class TableReport : public IReporter {
public:
  explicit TableReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config)) {}

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  void generateDashboard(const epoch_frame::DataFrame &normalizedDf) const override {
    // Dashboard generation uses the same implementation as tearsheet
    generateTearsheet(normalizedDf);
  }

private:
  // Helper methods
  std::string GetSQLQuery() const;
  std::string GetTitle() const;
  std::string GetCategory() const;
  uint32_t GetMaxRows() const;
  bool GetShowIndex() const;
  std::vector<std::string> GetSelectedColumns() const;
  epoch_proto::EpochFolioDashboardWidget GetWidgetType() const;

  epoch_frame::DataFrame ExecuteSQL(const epoch_frame::DataFrame& df) const;

  epoch_proto::Table BuildTableFromDataFrame(
      const epoch_frame::DataFrame& result) const;
};

// Template specialization for TableReport metadata
template <> struct ReportMetadata<TableReport> {
  constexpr static const char *kReportId = "table_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Table Report",
      .options = {
        {.id = "sql",
         .name = "SQL Query",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{
           "SELECT * FROM input0 LIMIT 100"},
         .isRequired = true,
         .desc = "SQL query to generate table data. The result will be "
                 "displayed as a formatted table in the report."},
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"Results"},
         .isRequired = false,
         .desc = "Title displayed above the table"},
        {.id = "category",
         .name = "Category",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"Data"},
         .isRequired = false,
         .desc = "Category name for grouping tables in the report"},
        {.id = "max_rows",
         .name = "Maximum Rows",
         .type = epoch_core::MetaDataOptionType::Integer,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{100.0},
         .isRequired = false,
         .min = 1,
         .max = 10000,
         .desc = "Maximum number of rows to display in the table"},
        {.id = "show_index",
         .name = "Show Index",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{false},
         .isRequired = false,
         .desc = "Include the DataFrame index as the first column"},
        {.id = "selected_columns",
         .name = "Selected Columns",
         .type = epoch_core::MetaDataOptionType::StringList,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{},
         .isRequired = false,
         .desc = "List of column names to include in the table. "
                 "If empty, all columns are included."},
        {.id = "widget_type",
         .name = "Widget Type",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"TABLE"},
         .isRequired = false,
         .desc = "Dashboard widget type (TABLE, GRID, etc.)"},
        {.id = "column_types",
         .name = "Column Types",
         .type = epoch_core::MetaDataOptionType::StringList,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{},
         .isRequired = false,
         .desc = "List of column type hints for formatting (DECIMAL, INTEGER, STRING, etc.)"},
        {.id = "decimal_places",
         .name = "Decimal Places",
         .type = epoch_core::MetaDataOptionType::Integer,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{4.0},
         .isRequired = false,
         .min = 0,
         .max = 10,
         .desc = "Number of decimal places for numeric columns"},
        {.id = "format_numbers",
         .name = "Format Numbers",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{true},
         .isRequired = false,
         .desc = "Apply number formatting (thousands separators, etc.)"},
        {.id = "highlight_negative",
         .name = "Highlight Negative",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{false},
         .isRequired = false,
         .desc = "Highlight negative values in red"},
        {.id = "sort_column",
         .name = "Sort Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{""},
         .isRequired = false,
         .desc = "Column name to sort by (optional)"},
        {.id = "sort_ascending",
         .name = "Sort Ascending",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{true},
         .isRequired = false,
         .desc = "Sort in ascending order (false for descending)"}
      },
      .isCrossSectional = false,
      .desc = "Generate table visualizations from SQL query results. "
              "Executes SQL on input data and creates formatted tables for report display. "
              "Supports column selection, sorting, formatting, and row limits.",
      .inputs = {},  // Variadic inputs
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = false,
      .tags = {"report", "table", "dashboard", "sql", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = true,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports