#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <arrow/api.h>

namespace epoch_metadata::reports {
  class PieChartReport : public IReporter {
  public:
    explicit PieChartReport(epoch_metadata::transform::TransformConfiguration config)
        : IReporter(std::move(config), true),
          m_chartTitle(m_config.GetOptionValue("title").GetString()),
          m_category(m_config.GetOptionValue("title").GetString()){
    }

  protected:
    void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  private:
    const std::string m_chartTitle;
    const std::string m_category;

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
        .desc = "Generates pie chart from DataFrame with direct column access. Use 'label' and 'value' inputs to specify columns.",
        .inputs = {
          {epoch_core::IODataType::String, "label", "Label Column"},
          {epoch_core::IODataType::Number, "value", "Value Column"}
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