#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {

class NestedPieChartReport : public IReporter {
public:
  explicit NestedPieChartReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_chartTitle(m_config.GetOptionValue("title").GetString()),
        m_category(m_config.GetOptionValue("category").GetString()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  const std::string m_chartTitle;
  const std::string m_category;
};

template <> struct ReportMetadata<NestedPieChartReport> {
  constexpr static const char *kReportId = "nested_pie_chart_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Executor,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Nested Pie Chart Report",
      .options = {
        {.id = "title",
         .name = "Chart Title",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Title for the generated chart"},
      {.id = "category",
      .name = "Category",
      .type = epoch_core::MetaDataOptionType::String,
      .isRequired = true,
      .desc = "For grouping in dashboard"}
      },
      .isCrossSectional = false,
      .desc = "Generates nested pie chart with inner and outer rings. Groups by both inner_label and outer_label, sums and normalizes values.",
      .inputs = {
        {epoch_core::IODataType::String, "inner_label", "Inner Label Column"},
        {epoch_core::IODataType::String, "outer_label", "Outer Label Column"},
        {epoch_core::IODataType::Number, "value", "Value Column"}
      },
      .outputs = {},
      .atLeastOneInputRequired = true,
      .tags = {"report", "chart", "pie", "nested", "visualization"},
      .requiresTimeFrame = false,
      .allowNullInputs = false,
      .isReporter = true
    };
  }
};

} // namespace epoch_metadata::reports
