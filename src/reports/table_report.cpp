#include "table_report.h"
#include "report_utils.h"
#include <epoch_dashboard/tearsheet/table_builder.h>
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

void TableReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  if (m_sqlQuery.empty()) {
    std::cerr << "Warning: TableReport requires 'sql' option" << std::endl;
    return;
  }

  try {
    // Build input rename mapping (input0, input1, input2, ...)
    auto inputRenameMap = BuildVARGInputRenameMapping();
    epoch_frame::DataFrame inputDf = normalizedDf.rename(inputRenameMap);

    // Execute SQL query
    // If add_index=true, index is added as 'timestamp' column
    // Store intermediate DataFrame to avoid using temporary in query()
    if (m_addIndex) {
      inputDf = inputDf.reset_index("timestamp");
    }
    auto resultTable = inputDf.query(m_sqlQuery, "input");
    epoch_frame::DataFrame resultDf(resultTable);


    // For table reports, we keep the sanitized column names since SQL operates on them
    // and test expectations are based on sanitized names

    // Build protobuf Table using TableBuilder
    epoch_tearsheet::TableBuilder tableBuilder;
    tableBuilder.setTitle(m_tableTitle.empty() ? "SQL Query Result" : m_tableTitle)
                .setCategory("Reports")
                .setType(epoch_proto::WidgetDataTable)
                .fromDataFrame(resultDf);

    // Add table to dashboard
    m_dashboard.addTable(tableBuilder.build());

  } catch (const std::exception& e) {
    std::cerr << "Error: TableReport SQL execution failed: " << e.what() << std::endl;
    return;
  }
}

std::string TableReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

bool TableReport::GetAddIndex() const {
  auto options = m_config.GetOptions();
  if (options.contains("add_index") && options["add_index"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["add_index"].GetBoolean();
  }
  return false; // Default to not adding index
}

std::string TableReport::GetTableTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return ""; // Empty means auto-generate
}



} // namespace epoch_metadata::reports