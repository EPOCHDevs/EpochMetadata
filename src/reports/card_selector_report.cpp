#include "card_selector_report.h"
#include "report_utils.h"
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>
#include <unordered_map>
#include <regex>

namespace epoch_metadata::reports {

// Convert to protobuf CardColumnSchema
epoch_proto::CardColumnSchema CardColumnSchema::toProto() const {
  epoch_proto::CardColumnSchema proto;
  proto.set_column_id(column_id);
  proto.set_label(label);
  proto.set_render_type(CardSelectorReport::ToProtoRenderType(render_type));
  if (!format_hint.empty()) {
    proto.set_format_hint(format_hint);
  }
  return proto;
}

void CardSelectorReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  if (m_cardSchema.empty()) {
    std::cerr << "Warning: CardSelectorReport requires 'card_schema' option" << std::endl;
    return;
  }

  try {
    // Build input rename mapping (input0, input1, input2, ...)
    auto inputRenameMap = BuildVARGInputRenameMapping();
    epoch_frame::DataFrame inputDf = normalizedDf.rename(inputRenameMap);

    // Execute SQL query if provided
    epoch_frame::DataFrame resultDf;
    if (!m_sqlQuery.empty()) {
      // If add_index=true, index is added as 'timestamp' column
      if (m_addIndex) {
        inputDf = inputDf.reset_index("timestamp");
      }
      auto resultTable = inputDf.query(m_sqlQuery, "input");
      resultDf = epoch_frame::DataFrame(resultTable);
    } else {
      // No SQL query, just add index if requested
      if (m_addIndex) {
        resultDf = inputDf.reset_index("timestamp");
      } else {
        resultDf = inputDf;
      }
    }

    // Build protobuf CardSelectorTable
    epoch_proto::CardSelectorTable cardSelectorTable;
    cardSelectorTable.set_type(epoch_proto::EpochFolioDashboardWidget::WidgetCardSelector);
    cardSelectorTable.set_category("Reports");
    cardSelectorTable.set_title(m_tableTitle.empty() ? "Card Selector" : m_tableTitle);
    cardSelectorTable.set_timestamp_column(m_timestampColumn);

    // Add column definitions
    auto table = resultDf.table();
    auto schema = table->schema();
    for (int i = 0; i < schema->num_fields(); ++i) {
      auto field = schema->field(i);
      auto* colDef = cardSelectorTable.add_columns();
      colDef->set_id(field->name());
      colDef->set_name(field->name());

      // Infer protobuf type from Arrow type
      switch (field->type()->id()) {
        case arrow::Type::INT8:
        case arrow::Type::INT16:
        case arrow::Type::INT32:
        case arrow::Type::INT64:
        case arrow::Type::UINT8:
        case arrow::Type::UINT16:
        case arrow::Type::UINT32:
        case arrow::Type::UINT64:
          colDef->set_type(epoch_proto::EpochFolioType::TypeInteger);
          break;
        case arrow::Type::FLOAT:
        case arrow::Type::DOUBLE:
          colDef->set_type(epoch_proto::EpochFolioType::TypeDecimal);
          break;
        case arrow::Type::BOOL:
          colDef->set_type(epoch_proto::EpochFolioType::TypeBoolean);
          break;
        case arrow::Type::TIMESTAMP:
          colDef->set_type(epoch_proto::EpochFolioType::TypeDateTime);
          break;
        case arrow::Type::STRING:
        default:
          colDef->set_type(epoch_proto::EpochFolioType::TypeString);
          break;
      }
    }

    // Add card schema
    for (const auto& schemaEntry : m_cardSchema) {
      auto* protoSchema = cardSelectorTable.add_card_schema();
      *protoSchema = schemaEntry.toProto();
    }

    // Add row data
    auto* tableData = cardSelectorTable.mutable_data();
    int64_t numRows = table->num_rows();
    for (int64_t rowIdx = 0; rowIdx < numRows; ++rowIdx) {
      auto* row = tableData->add_rows();

      for (int colIdx = 0; colIdx < schema->num_fields(); ++colIdx) {
        auto* scalar = row->add_values();
        auto column = table->column(colIdx);
        auto chunk = column->chunk(0);  // Assuming single chunk for simplicity

        if (chunk->IsNull(rowIdx)) {
          scalar->set_null_value(epoch_proto::NullValue::NULL_VALUE);
          continue;
        }

        // Convert Arrow value to protobuf Scalar
        switch (schema->field(colIdx)->type()->id()) {
          case arrow::Type::INT8:
          case arrow::Type::INT16:
          case arrow::Type::INT32:
          case arrow::Type::INT64: {
            auto arr = std::static_pointer_cast<arrow::Int64Array>(chunk);
            scalar->set_integer_value(arr->Value(rowIdx));
            break;
          }
          case arrow::Type::UINT8:
          case arrow::Type::UINT16:
          case arrow::Type::UINT32:
          case arrow::Type::UINT64: {
            auto arr = std::static_pointer_cast<arrow::UInt64Array>(chunk);
            scalar->set_integer_value(static_cast<int64_t>(arr->Value(rowIdx)));
            break;
          }
          case arrow::Type::FLOAT: {
            auto arr = std::static_pointer_cast<arrow::FloatArray>(chunk);
            scalar->set_decimal_value(arr->Value(rowIdx));
            break;
          }
          case arrow::Type::DOUBLE: {
            auto arr = std::static_pointer_cast<arrow::DoubleArray>(chunk);
            scalar->set_decimal_value(arr->Value(rowIdx));
            break;
          }
          case arrow::Type::BOOL: {
            auto arr = std::static_pointer_cast<arrow::BooleanArray>(chunk);
            scalar->set_boolean_value(arr->Value(rowIdx));
            break;
          }
          case arrow::Type::STRING: {
            auto arr = std::static_pointer_cast<arrow::StringArray>(chunk);
            scalar->set_string_value(arr->GetString(rowIdx));
            break;
          }
          case arrow::Type::TIMESTAMP: {
            auto arr = std::static_pointer_cast<arrow::TimestampArray>(chunk);
            scalar->set_timestamp_ms(arr->Value(rowIdx));
            break;
          }
          default:
            // Fallback to string representation
            scalar->set_string_value("unsupported_type");
            break;
        }
      }
    }

    // Add to dashboard
    // Note: DashboardBuilder needs to be extended with addCardSelectorTable method
    // For now, we'll need to manually add it to the tearsheet
    // This is a limitation until EpochDashboard library is updated
    auto* tearsheet = m_dashboard.build().mutable_card_selector_tables();
    tearsheet->add_card_selector_tables()->CopyFrom(cardSelectorTable);

  } catch (const std::exception& e) {
    std::cerr << "Error: CardSelectorReport execution failed: " << e.what() << std::endl;
    return;
  }
}

std::string CardSelectorReport::GetSQLQuery() const {
  auto options = m_config.GetOptions();
  if (options.contains("sql") && options["sql"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["sql"].GetString();
  }
  return "";
}

std::vector<CardColumnSchema> CardSelectorReport::GetCardSchema() const {
  auto options = m_config.GetOptions();
  if (options.contains("card_schema") && options["card_schema"].IsType(epoch_core::MetaDataOptionType::String)) {
    std::string schemaJson = options["card_schema"].GetString();
    return ParseCardSchemaJson(schemaJson);
  }
  return {};
}

std::vector<CardColumnSchema> CardSelectorReport::ParseCardSchemaJson(const std::string& jsonStr) {
  std::vector<CardColumnSchema> schemas;

  try {
    // Parse JSON using glaze generic
    glz::generic json{};
    auto error = glz::read_json(json, jsonStr);
    if (error) {
      throw std::runtime_error("JSON parse error: " + glz::format_error(error, jsonStr));
    }

    if (!json.is_array()) {
      throw std::runtime_error("card_schema must be a JSON array");
    }

    for (const auto& item : json.get_array()) {
      CardColumnSchema schema;

      // Extract column_id
      if (item.contains("column_id") && item.at("column_id").is_string()) {
        schema.column_id = item.at("column_id").get_string();
      } else {
        throw std::runtime_error("CardColumnSchema: 'column_id' field is required and must be a string");
      }

      // Extract label (default to column_id)
      if (item.contains("label") && item.at("label").is_string()) {
        schema.label = item.at("label").get_string();
      } else {
        schema.label = schema.column_id;
      }

      // Extract render_type (default to Label)
      if (item.contains("render_type") && item.at("render_type").is_string()) {
        schema.render_type = ParseRenderType(item.at("render_type").get_string());
      } else {
        schema.render_type = CardRenderType::Label;
      }

      // Extract format_hint (optional)
      if (item.contains("format_hint") && item.at("format_hint").is_string()) {
        schema.format_hint = item.at("format_hint").get_string();
      }

      schemas.push_back(schema);
    }

    return schemas;
  } catch (const std::exception& e) {
    std::cerr << "Error parsing card_schema JSON: " << e.what() << std::endl;
    throw;
  }
}

std::string CardSelectorReport::GetTimestampColumn() const {
  auto options = m_config.GetOptions();
  if (options.contains("timestamp_column") && options["timestamp_column"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["timestamp_column"].GetString();
  }
  return "timestamp"; // Default
}

std::string CardSelectorReport::GetTableTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return ""; // Empty means use default
}

bool CardSelectorReport::GetAddIndex() const {
  auto options = m_config.GetOptions();
  if (options.contains("add_index") && options["add_index"].IsType(epoch_core::MetaDataOptionType::Boolean)) {
    return options["add_index"].GetBoolean();
  }
  return false; // Default to not adding index
}

CardRenderType CardSelectorReport::ParseRenderType(const std::string& typeStr) {
  std::string lower = typeStr;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "label") return CardRenderType::Label;
  if (lower == "major_number") return CardRenderType::MajorNumber;
  if (lower == "side_badge") return CardRenderType::SideBadge;
  if (lower == "timestamp") return CardRenderType::Timestamp;
  if (lower == "percentage") return CardRenderType::Percentage;
  if (lower == "icon_label") return CardRenderType::IconLabel;
  if (lower == "minor_number") return CardRenderType::MinorNumber;

  std::cerr << "Warning: Unknown render_type '" << typeStr << "', defaulting to 'label'" << std::endl;
  return CardRenderType::Label;
}

epoch_proto::CardRenderType CardSelectorReport::ToProtoRenderType(CardRenderType type) {
  switch (type) {
    case CardRenderType::Label:
      return epoch_proto::CardRenderType::CARD_RENDER_LABEL;
    case CardRenderType::MajorNumber:
      return epoch_proto::CardRenderType::CARD_RENDER_MAJOR_NUMBER;
    case CardRenderType::SideBadge:
      return epoch_proto::CardRenderType::CARD_RENDER_SIDE_BADGE;
    case CardRenderType::Timestamp:
      return epoch_proto::CardRenderType::CARD_RENDER_TIMESTAMP;
    case CardRenderType::Percentage:
      return epoch_proto::CardRenderType::CARD_RENDER_PERCENTAGE;
    case CardRenderType::IconLabel:
      return epoch_proto::CardRenderType::CARD_RENDER_ICON_LABEL;
    case CardRenderType::MinorNumber:
      return epoch_proto::CardRenderType::CARD_RENDER_MINOR_NUMBER;
    default:
      return epoch_proto::CardRenderType::CARD_RENDER_LABEL;
  }
}

} // namespace epoch_metadata::reports
