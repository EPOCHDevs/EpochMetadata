#include "table_report.h"
#include <epoch_dashboard/tearsheet/tearsheet_builder.h>
#include <epoch_frame/dataframe.h>
#include <algorithm>

namespace epoch_metadata::reports {

void TableReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  // Execute SQL query on the normalized DataFrame
  auto result = ExecuteSQL(normalizedDf);

  // Apply row limit
  auto maxRows = GetMaxRows();
  if (result.num_rows() > maxRows) {
    result = result.head(maxRows);
  }

  // Apply column selection if specified
  auto selectedCols = GetSelectedColumns();
  if (!selectedCols.empty()) {
    // Filter to only selected columns
    std::vector<std::string> validCols;
    auto allCols = result.column_names();
    for (const auto& col : selectedCols) {
      if (std::find(allCols.begin(), allCols.end(), col) != allCols.end()) {
        validCols.push_back(col);
      }
    }
    if (!validCols.empty()) {
      result = result[validCols];
    }
  }

  // Apply sorting if specified
  auto sortCol = GetSortColumn();
  if (!sortCol.empty()) {
    auto allCols = result.column_names();
    if (std::find(allCols.begin(), allCols.end(), sortCol) != allCols.end()) {
      result = result.sort_values({sortCol}, {GetSortAscending()});
    }
  }

  // Build table from result
  auto table = BuildTableFromDataFrame(result);

  // Add to tearsheet
  *m_tearsheet.add_tables() = table;

  // Also add to dashboard
  *m_dashboard.add_tables() = table;
}

std::string TableReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  throw std::runtime_error("TableReport: 'sql' option is required");
}

std::string TableReport::GetTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "Results";
}

std::string TableReport::GetCategory() const {
  auto options = m_config.GetOptions();
  if (options.contains("category") && options["category"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["category"].GetString();
  }
  return "Data";
}

uint32_t TableReport::GetMaxRows() const {
  auto options = m_config.GetOptions();
  if (options.contains("max_rows") && (options["max_rows"].IsType(epoch_core::MetaDataOptionType::Integer) || options["max_rows"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    return static_cast<uint32_t>(options["max_rows"].GetDecimal());
  }
  return 100;
}

bool TableReport::GetShowIndex() const {
  auto options = m_config.GetOptions();
  if (options.contains("show_index") && options["show_index"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["show_index"].GetBoolean();
  }
  return false;
}

std::vector<std::string> TableReport::GetSelectedColumns() const {
  std::vector<std::string> cols;
  auto options = m_config.GetOptions();
  if (options.contains("selected_columns") && options["selected_columns"].IsType(epoch_core::MetaDataOptionType::StringList)) {
    // Need to get the sequence and iterate through it
    const auto& variant = options["selected_columns"].GetVariant();
    if (std::holds_alternative<epoch_metadata::Sequence>(variant)) {
      const auto& seq = std::get<epoch_metadata::Sequence>(variant);
      for (const auto& item : seq) {
        if (std::holds_alternative<std::string>(item)) {
          cols.push_back(std::get<std::string>(item));
        }
      }
    }
  }
  return cols;
}

std::string TableReport::GetSortColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("sort_column") && options["sort_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sort_column"].GetString();
  }
  return "";
}

bool TableReport::GetSortAscending() const {
  auto options = m_config.GetOptions();
  if (options.contains("sort_ascending") && options["sort_ascending"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["sort_ascending"].GetBoolean();
  }
  return true;
}

epoch_proto::EpochFolioDashboardWidget TableReport::GetWidgetType() const {
  auto options = m_config.GetOptions();
  if (options.contains("widget_type") && options["widget_type"].IsType(epoch_core::MetaDataOptionType::String)) {
    std::string type = options["widget_type"].GetString();
    // Map string to enum
    if (type == "GRID") {
      return epoch_proto::EpochFolioDashboardWidget::GRID;
    }
    // Add more mappings as needed
  }
  return epoch_proto::EpochFolioDashboardWidget::TABLE;
}

epoch_frame::DataFrame TableReport::ExecuteSQL(const epoch_frame::DataFrame& df) const {
  std::string sql = GetSQLQuery();

  // Get input IDs for variadic inputs
  auto inputIds = GetInputIds();

  if (inputIds.empty()) {
    // No inputs, execute SQL directly
    return epoch_frame::DataFrame::sql(sql);
  }

  // Prepare input DataFrames map
  std::unordered_map<std::string, epoch_frame::DataFrame> inputs;

  // Add required data sources if any
  const auto& metadata = m_config.GetTransformDefinition().GetMetadata();
  if (!metadata.requiredDataSources.empty()) {
    inputs["market_data"] = df[metadata.requiredDataSources];
  }

  // Map configured inputs
  for (const auto& [inputId, columns] : m_config.GetInputs()) {
    if (!columns.empty()) {
      inputs[inputId] = df[columns];
    }
  }

  // Handle variadic inputs
  if (!inputIds.empty() && inputs.empty()) {
    for (size_t i = 0; i < inputIds.size(); ++i) {
      inputs["input" + std::to_string(i)] = df[[inputIds[i]]];
    }
  }

  // Execute SQL with inputs
  return epoch_frame::DataFrame::sql(sql, inputs);
}

epoch_proto::Table TableReport::BuildTableFromDataFrame(
    const epoch_frame::DataFrame& result) const {

  epoch_tearsheet::TableBuilder builder;

  // Set basic properties
  builder.setType(GetWidgetType())
         .setCategory(GetCategory())
         .setTitle(GetTitle());

  // Get formatting options
  auto options = m_config.GetOptions();
  bool formatNumbers = true;
  bool highlightNegative = false;
  int decimalPlaces = 4;

  if (options.contains("format_numbers") && options["format_numbers"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    formatNumbers = options["format_numbers"].GetBoolean();
  }
  if (options.contains("highlight_negative") && options["highlight_negative"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    highlightNegative = options["highlight_negative"].GetBoolean();
  }
  if (options.contains("decimal_places") && (options["decimal_places"].IsType(epoch_core::MetaDataOptionType::Integer) || options["decimal_places"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    decimalPlaces = static_cast<int>(options["decimal_places"].GetDecimal());
  }

  // Get column type hints if provided
  std::vector<std::string> columnTypes;
  if (options.contains("column_types") && options["column_types"].IsType(epoch_core::MetaDataOptionType::StringList)) {
    // Need to get the sequence and iterate through it
    const auto& variant = options["column_types"].GetVariant();
    if (std::holds_alternative<epoch_metadata::Sequence>(variant)) {
      const auto& seq = std::get<epoch_metadata::Sequence>(variant);
      for (const auto& item : seq) {
        if (std::holds_alternative<std::string>(item)) {
          columnTypes.push_back(std::get<std::string>(item));
        }
      }
    }
  }

  // Add index column if requested
  if (GetShowIndex()) {
    builder.addColumn("index", "Index", epoch_proto::EpochFolioType::INTEGER);
  }

  // Add data columns
  auto columns = result.column_names();
  for (size_t i = 0; i < columns.size(); ++i) {
    const auto& colName = columns[i];
    auto series = result[colName];

    // Determine column type
    epoch_proto::EpochFolioType colType;
    if (i < columnTypes.size()) {
      // Use provided type hint
      if (columnTypes[i] == "INTEGER") {
        colType = epoch_proto::EpochFolioType::INTEGER;
      } else if (columnTypes[i] == "DECIMAL") {
        colType = epoch_proto::EpochFolioType::DECIMAL;
      } else if (columnTypes[i] == "STRING") {
        colType = epoch_proto::EpochFolioType::STRING;
      } else if (columnTypes[i] == "BOOLEAN") {
        colType = epoch_proto::EpochFolioType::BOOLEAN;
      } else {
        colType = epoch_proto::EpochFolioType::ANY;
      }
    } else {
      // Infer from data type
      if (series.is_integer()) {
        colType = epoch_proto::EpochFolioType::INTEGER;
      } else if (series.is_numeric()) {
        colType = epoch_proto::EpochFolioType::DECIMAL;
      } else if (series.is_string_type()) {
        colType = epoch_proto::EpochFolioType::STRING;
      } else if (series.is_boolean()) {
        colType = epoch_proto::EpochFolioType::BOOLEAN;
      } else {
        colType = epoch_proto::EpochFolioType::ANY;
      }
    }

    builder.addColumn(colName, colName, colType);
  }

  // Use the builder's fromDataFrame method to populate data
  builder.fromDataFrame(result, columns);

  return builder.build();
}

std::string TableReport::GetSortColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("sort_column") && options["sort_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sort_column"].GetString();
  }
  return "";
}

bool TableReport::GetSortAscending() const {
  auto options = m_config.GetOptions();
  if (options.contains("sort_ascending") && options["sort_ascending"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["sort_ascending"].GetBoolean();
  }
  return true;
}

} // namespace epoch_metadata::reports