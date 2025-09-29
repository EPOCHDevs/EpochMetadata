#pragma once
#include <epoch_metadata/reports/ireport.h>
#include <glaze/glaze.hpp>
#include <epoch_metadata/bar_attribute.h>

// EpochDashboard includes
#include <epoch_dashboard/tearsheet/table_builder.h>
#include <epoch_dashboard/tearsheet/card_builder.h>
#include <epoch_dashboard/tearsheet/lines_chart_builder.h>
#include <epoch_dashboard/tearsheet/bar_chart_builder.h>
#include <epoch_dashboard/tearsheet/pie_chart_builder.h>
#include <epoch_dashboard/tearsheet/histogram_chart_builder.h>
#include <epoch_dashboard/tearsheet/scalar_converter.h>
#include <epoch_dashboard/tearsheet/dataframe_converter.h>

namespace epoch_metadata::reports {

// Structure to hold comprehensive gap data for reuse across visualizations
struct GapTableData {
    std::shared_ptr<arrow::Table> arrow_table;

    // Cached aggregations for efficiency
    size_t total_gaps = 0;
    size_t gap_up_count = 0;
    size_t gap_down_count = 0;
    size_t filled_count = 0;
    size_t gap_up_filled = 0;
    size_t gap_down_filled = 0;

    // Column indices for quick access
    int gap_size_col = -1;
    int gap_type_col = -1;
    int gap_filled_col = -1;
    int weekday_col = -1;
    int fill_time_col = -1;
    int performance_col = -1;
};

class GapReport : public IReporter {
public:
  explicit GapReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config)), m_pivotHour(m_config.GetOptionValue("fill_time_pivot_hour").GetInteger()) {}

protected:
    int64_t m_pivotHour;
  // Implementation of IReporter's virtual methods
  void
  generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

public:
  epoch_tearsheet::DashboardBuilder generate_impl(const epoch_frame::DataFrame &df) const;

  // Analysis helpers
  std::vector<epoch_proto::CardDef>
  compute_summary_cards(const GapTableData &table) const;

  std::pair<epoch_proto::Table, epoch_proto::Table> create_fill_rate_tables(
      const GapTableData &table) const;

  // New methods that work with table data
  GapTableData build_comprehensive_table_data(const epoch_frame::DataFrame &gaps) const;

  epoch_proto::Chart create_stacked_fill_rate_chart(const GapTableData &data) const;

  epoch_proto::Chart create_day_of_week_chart(const GapTableData &data) const;

  epoch_proto::Chart create_gap_distribution(const GapTableData &data) const;

  epoch_proto::Chart create_gap_category_chart(const GapTableData &data) const;

  epoch_proto::Chart create_weekday_chart(const GapTableData &data) const;

private:
  // Helper functions
  epoch_proto::Chart create_grouped_stacked_chart(
      const GapTableData &data,
      const std::string &title,
      const std::string &x_axis_label,
      const std::vector<std::string> &categories,
      std::function<int(int64_t)> get_category_index,
      std::function<double(double)> process_value) const;

};

// Template specialization for GapReport metadata
template <> struct ReportMetadata<GapReport> {
  constexpr static const char *kReportId = "gap_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
   return {
    .id = kReportId,
    .category = epoch_core::TransformCategory::Executor,
    .renderKind = epoch_core::TransformNodeRenderKind::Standard,
    .name = "Gap Analysis Report",
    .options = {{.id = "fill_time_pivot_hour",
          .name = "Fill Time Pivot Hour",
          .type = epoch_core::MetaDataOptionType::Integer,
          .defaultValue = epoch_metadata::MetaDataOptionDefinition{13.0},
          .isRequired = false,
          .min = 0,
          .max = 23,
          .desc = "The hour used to categorize gap fill times (e.g., 13 for 'before 13:00' vs 'after 13:00'). Used in fill time analysis to identify early vs late session fills."},
        {.id = "histogram_bins",
          .name = "Histogram Bins",
          .type = epoch_core::MetaDataOptionType::Integer,
          .defaultValue = epoch_metadata::MetaDataOptionDefinition{10.0},
          .isRequired = false,
          .min = 3,
          .max = 50,
          .desc = "Number of bins to use for the gap size distribution histogram. Controls the granularity of the size distribution visualization."}},
    .isCrossSectional = false,
    .desc = "Comprehensive gap analysis report that examines price gaps "
            "between trading sessions. Analyzes opening price gaps relative "
            "to prior session close, tracking gap direction (up/down), "
            "size distribution, fill rates, and performance patterns. "
            "Generates visualizations including fill rate charts, streak "
            "analysis, time-of-day distributions, and trend analysis to "
            "identify gap trading opportunities and patterns across different "
            "market conditions and timeframes.",
    .inputs = {{epoch_core::IODataType::Boolean, "gap_filled", "Gap Filled"},
               {epoch_core::IODataType::Decimal, "gap_retrace", "Gap Retrace"},
               {epoch_core::IODataType::Decimal, "gap_size", "Gap Size"},
               {epoch_core::IODataType::Decimal, "psc", "Prior Session Close"},
               {epoch_core::IODataType::Integer, "psc_timestamp", "PSC Timestamp"}},
    .outputs = {},
    .tags = {"gap_classify"},
 .requiresTimeFrame = true,
    .requiredDataSources =
        {epoch_metadata::EpochStratifyXConstants::instance().CLOSE()},
    .intradayOnly=true,
.allowNullInputs=true,
    .isReporter = true};
  }

  // Helper to create a TransformConfiguration from a gap classifier config
  static epoch_metadata::transform::TransformConfiguration
  CreateConfig(const std::string &instance_id,
               const epoch_metadata::transform::TransformConfiguration
                   &gap_classifier_config,
               const YAML::Node &options = {}) {

    YAML::Node config;
    config["id"] = instance_id;
    config["type"] = kReportId;
    // Just pass timeframe as string
    config["timeframe"] = "1D"; // Default for now, could get from
                                // gap_classifier_config if method exists

    // Map the gap classifier's outputs to our inputs
    // The gap classifier produces columns that we need
    YAML::Node inputs;
    std::string gap_id = gap_classifier_config.GetId();

    // Map each required input to the gap classifier's output columns
    auto metadata = Get();
    for (const auto &input : metadata.inputs) {
      // Map input name to gap_classifier_id#column_name format
      inputs[input.name].push_back(gap_id + "#" + input.name);
    }

    config["inputs"] = inputs;
    // SessionRange is optional, skip it for now
    config["options"] = options;

    return epoch_metadata::transform::TransformConfiguration{
        epoch_metadata::TransformDefinition{config}};
  }

  // Simpler helper for testing without a preceding node
  static epoch_metadata::transform::TransformConfiguration
  CreateConfig(const std::string &instance_id,
               const std::string &timeframe = "1D",
               const YAML::Node &options = {}) {

    YAML::Node config;
    config["id"] = instance_id;
    config["type"] = kReportId;
    config["timeframe"] = timeframe;
    config["inputs"] = YAML::Node(); // Empty for testing
    config["sessionRange"] = YAML::Node();
    config["options"] = options;

    return epoch_metadata::transform::TransformConfiguration{
        epoch_metadata::TransformDefinition{config}};
  }
};

} // namespace epoch_metadata::reports