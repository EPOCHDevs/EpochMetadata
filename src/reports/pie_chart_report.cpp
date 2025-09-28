#include "pie_chart_report.h"
#include <epoch_dashboard/tearsheet/pie_chart_builder.h>
#include <epoch_dashboard/tearsheet/chart_types.h>
#include <epoch_frame/dataframe.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void PieChartReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    epoch_frame::DataFrame preparedDf = normalizedDf;

    // Execute SQL if provided
    if (!m_sqlQuery.empty()) {
      auto sanitizedDf = SanitizeColumnNames(preparedDf);
      auto resultTable = sanitizedDf.query(m_sqlQuery, m_tableName);
      preparedDf = epoch_frame::DataFrame(resultTable);
    }

    // Validate required columns
    auto schema = preparedDf.table()->schema();
    bool hasLabelColumn = false;
    bool hasValueColumn = false;

    for (int i = 0; i < schema->num_fields(); ++i) {
      std::string fieldName = schema->field(i)->name();
      if (fieldName == m_labelColumn) hasLabelColumn = true;
      if (fieldName == m_valueColumn) hasValueColumn = true;
    }

    if (!hasLabelColumn || !hasValueColumn) {
      std::cerr << "Error: PieChartReport - required columns not found" << std::endl;
      return;
    }

    // Build pie chart
    epoch_tearsheet::PieChartBuilder chartBuilder;
    chartBuilder.setTitle(m_chartTitle.empty() ? "Pie Chart" : m_chartTitle)
                .setCategory("Charts");

    // Configure pie size and inner size for donut
    epoch_tearsheet::PieSize size(100);  // 100% size
    std::optional<epoch_tearsheet::PieInnerSize> innerSize;
    if (m_innerSize > 0) {
      innerSize = epoch_tearsheet::PieInnerSize(m_innerSize);
    }

    // Use fromDataFrame to populate chart
    chartBuilder.fromDataFrame(preparedDf, m_labelColumn, m_valueColumn, "Series", size, innerSize);

    m_dashboard.addChart(chartBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: PieChartReport execution failed: " << e.what() << std::endl;
  }
}

std::string PieChartReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::string PieChartReport::GetTableName() const {
  auto options = m_config.GetOptions();
  if (options.contains("table_name") && options["table_name"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["table_name"].GetString();
  }
  return "input";
}

std::string PieChartReport::GetChartTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";
}

std::string PieChartReport::GetLabelColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("label_column") && options["label_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["label_column"].GetString();
  }
  return "label";
}

std::string PieChartReport::GetValueColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("value_column") && options["value_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["value_column"].GetString();
  }
  return "value";
}

uint32_t PieChartReport::GetInnerSize() const {
  auto options = m_config.GetOptions();
  if (options.contains("inner_size") && options["inner_size"].IsType(epoch_core::MetaDataOptionType::Integer)) {
    return static_cast<uint32_t>(options["inner_size"].GetInteger());
  }
  return 0;
}

epoch_frame::DataFrame PieChartReport::SanitizeColumnNames(const epoch_frame::DataFrame& df) const {
  auto table = df.table();
  auto schema = table->schema();
  std::unordered_map<std::string, std::string> renameMap;

  for (int i = 0; i < schema->num_fields(); ++i) {
    std::string colName = schema->field(i)->name();
    std::regex hashRegex("#");
    std::string sanitizedName = std::regex_replace(colName, hashRegex, "_");
    if (colName != sanitizedName) {
      renameMap[colName] = sanitizedName;
    }
  }

  return renameMap.empty() ? df : df.rename(renameMap);
}

} // namespace epoch_metadata::reports