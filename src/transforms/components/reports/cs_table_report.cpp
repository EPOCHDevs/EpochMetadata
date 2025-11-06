#include "cs_table_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/table_builder.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/table_factory.h>
#include <set>
#include <map>
#include <sstream>

namespace epoch_script::reports {

void CSTableReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  using namespace epoch_frame;

  if (normalizedDf.empty() || normalizedDf.num_cols() == 0) {
    std::cerr << "Error: CSTableReport received empty DataFrame" << std::endl;
    return;
  }

  // Get all input columns
  // Expected naming: AssetName for single metric, or AssetName_MetricIdx for multiple metrics
  std::set<std::string> assetNames;
  std::map<std::string, std::vector<std::string>> assetToMetricColumns;

  // Parse column names to extract asset names
  for (const auto& colName : normalizedDf.column_names()) {
    // For cross-sectional execution, columns are named by asset
    // e.g., "XLK", "XLF", or "XLK_SLOT0", "XLK_SLOT1" for multiple inputs
    std::string assetName = colName;
    size_t underscorePos = colName.find('_');
    if (underscorePos != std::string::npos) {
      assetName = colName.substr(0, underscorePos);
    }

    assetNames.insert(assetName);
    assetToMetricColumns[assetName].push_back(colName);
  }

  if (assetNames.empty()) {
    std::cerr << "Error: No asset data found" << std::endl;
    return;
  }

  // Build table structure
  // Columns: ["Asset", "Metric1", "Metric2", ...]
  std::vector<std::string> columnNames = {"Asset"};

  // Determine number of metrics from first asset
  auto firstAsset = *assetNames.begin();
  int numMetrics = assetToMetricColumns[firstAsset].size();

  for (int i = 0; i < numMetrics; ++i) {
    std::ostringstream metricName;
    metricName << "Metric" << (i + 1);
    columnNames.push_back(metricName.str());
  }

  // Build table data
  std::vector<std::vector<std::string>> tableData;

  for (const auto& assetName : assetNames) {
    std::vector<std::string> row;
    row.push_back(assetName);  // First column is asset name

    // Get metric values for this asset
    const auto& metricColumns = assetToMetricColumns[assetName];

    for (const auto& metricColumn : metricColumns) {
      try {
        auto series = normalizedDf[metricColumn];

        // Apply aggregation
        epoch_frame::Scalar aggregatedValue;

        if (m_agg == "last") {
          aggregatedValue = series.iloc(series.size() - 1);
        } else if (m_agg == "first") {
          aggregatedValue = series.iloc(0);
        } else if (m_agg == "mean") {
          aggregatedValue = series.mean();
        } else if (m_agg == "sum") {
          aggregatedValue = series.sum();
        } else if (m_agg == "min") {
          aggregatedValue = series.min();
        } else if (m_agg == "max") {
          aggregatedValue = series.max();
        } else {
          // Default to last
          aggregatedValue = series.iloc(series.size() - 1);
        }

        // Convert to string for table display
        row.push_back(aggregatedValue.repr());

      } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to process metric column '" << metricColumn
                  << "' for asset '" << assetName << "': " << e.what() << std::endl;
        row.push_back("N/A");
      }
    }

    tableData.push_back(row);
  }

  // Convert to DataFrame for TableBuilder
  // Create Arrow schema
  arrow::FieldVector fields;
  for (const auto& colName : columnNames) {
    fields.push_back(arrow::field(colName, arrow::utf8()));
  }
  auto schema = arrow::schema(fields);

  // Create Arrow arrays
  std::vector<std::shared_ptr<arrow::Array>> arrays;
  for (size_t colIdx = 0; colIdx < columnNames.size(); ++colIdx) {
    arrow::StringBuilder builder;
    for (const auto& row : tableData) {
      if (colIdx < row.size()) {
        ARROW_UNUSED(builder.Append(row[colIdx]));
      } else {
        ARROW_UNUSED(builder.AppendNull());
      }
    }
    std::shared_ptr<arrow::Array> array;
    ARROW_UNUSED(builder.Finish(&array));
    arrays.push_back(array);
  }

  // Create Arrow table and DataFrame
  auto arrowTable = AssertTableResultIsOk(arrow::Table::Make(schema, arrays));
  auto df = epoch_frame::DataFrame(arrowTable);

  // Build protobuf Table using TableBuilder
  epoch_tearsheet::TableBuilder tableBuilder;
  tableBuilder.setTitle(m_title.empty() ? "Asset Comparison" : m_title)
              .setCategory(m_category.empty() ? "Cross-Sectional" : m_category)
              .setType(epoch_proto::WidgetDataTable)
              .fromDataFrame(df);

  // Add table to dashboard
  m_dashboard.addTable(tableBuilder.build());
}

} // namespace epoch_script::reports
