#pragma once

#include <epoch_metadata/reports/ireport.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/scalar.h>
#include <arrow/api.h>
#include <glaze/glaze.hpp>

namespace epoch_metadata::reports {

// Card rendering type enumeration
enum class CardRenderType {
  Label,         // Plain text label
  MajorNumber,   // Large prominent number
  SideBadge,     // Small badge indicator
  Timestamp,     // Time/date display (used for navigation)
  Percentage,    // Percentage with % symbol
  IconLabel,     // Icon + text combo
  MinorNumber    // Secondary number display
};

// Schema definition for a column in a card selector
struct CardColumnSchema {
  std::string column_id;      // ID of the column from the table
  std::string label;          // Display label for this field
  CardRenderType render_type; // How to render this column
  std::string format_hint;    // Optional formatting (e.g., "%.2f%%", "badge-success")

  // Convert to protobuf CardColumnSchema
  epoch_proto::CardColumnSchema toProto() const;
};

class CardSelectorReport : public IReporter {
public:
  explicit CardSelectorReport(epoch_metadata::transform::TransformConfiguration config)
      : IReporter(std::move(config), true),
        m_sqlQuery(GetSQLQuery()),
        m_cardSchema(GetCardSchema()),
        m_timestampColumn(GetTimestampColumn()),
        m_tableTitle(GetTableTitle()),
        m_addIndex(GetAddIndex()) {
  }

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  // Cached configuration values
  const std::string m_sqlQuery;
  const std::vector<CardColumnSchema> m_cardSchema;
  const std::string m_timestampColumn;
  const std::string m_tableTitle;
  const bool m_addIndex;

  // Configuration getters
  std::string GetSQLQuery() const;
  std::vector<CardColumnSchema> GetCardSchema() const;
  std::string GetTimestampColumn() const;
  std::string GetTableTitle() const;
  bool GetAddIndex() const;

  // Helper methods
  static CardRenderType ParseRenderType(const std::string& typeStr);
  static epoch_proto::CardRenderType ToProtoRenderType(CardRenderType type);
  static std::vector<CardColumnSchema> ParseCardSchemaJson(const std::string& jsonStr);
};

// Metadata specialization for CardSelectorReport
template <> struct ReportMetadata<CardSelectorReport> {
  constexpr static const char *kReportId = "card_selector_report";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kReportId,
      .category = epoch_core::TransformCategory::Reporter,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Card Selector Report",
      .options = {
        {.id = "card_schema",
         .name = "Card Schema",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = R"(JSON array defining how each column renders in cards. Example:
[
  {"column_id": "timestamp", "label": "Time", "render_type": "timestamp"},
  {"column_id": "profit_pct", "label": "Profit", "render_type": "major_number", "format_hint": "%.2f%%"},
  {"column_id": "signal_type", "label": "Signal", "render_type": "side_badge"}
]
Render types: label, major_number, side_badge, timestamp, percentage, icon_label, minor_number)"},
        {.id = "timestamp_column",
         .name = "Timestamp Column",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"timestamp"},
         .isRequired = false,
         .desc = "Column name containing timestamps for chart navigation"},
        {.id = "sql",
         .name = "SQL Query",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = false,
         .desc = "Optional SQL query to filter/transform rows before generating cards"},
        {.id = "add_index",
         .name = "Add Index",
         .type = epoch_core::MetaDataOptionType::Boolean,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{false},
         .isRequired = false,
         .desc = "Add DataFrame index as 'timestamp' column"},
        {.id = "title",
         .name = "Table Title",
         .type = epoch_core::MetaDataOptionType::String,
         .defaultValue = epoch_metadata::MetaDataOptionDefinition{"Card Selector"},
         .isRequired = false,
         .desc = "Title for the card selector widget"}
      },
      .isCrossSectional = false,
      .desc = "Generate an interactive card selector where each row is a clickable card. "
              "Click a card to navigate to that timestamp on the candlestick chart. "
              "Accepts multiple input columns via SLOT connection.",
      .inputs = {
        {.type = epoch_core::IODataType::Any,
         .id = "SLOT",
         .name = "Columns",
         .allowMultipleConnections = true}
      },
      .outputs = {},  // Report outputs via TearSheet
      .atLeastOneInputRequired = true,
      .tags = {"report", "selector", "interactive", "cards", "navigation", "timepoint"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epoch_metadata::reports

// Glaze metadata for CardColumnSchema
namespace glz {
  template <>
  struct meta<epoch_metadata::reports::CardColumnSchema> {
    using T = epoch_metadata::reports::CardColumnSchema;
    static constexpr auto value = object(
      "column_id", &T::column_id,
      "label", &T::label,
      "render_type", [](auto& self) -> std::string {
        // Custom getter for render_type - convert enum to string
        switch (self.render_type) {
          case epoch_metadata::reports::CardRenderType::Label: return "label";
          case epoch_metadata::reports::CardRenderType::MajorNumber: return "major_number";
          case epoch_metadata::reports::CardRenderType::SideBadge: return "side_badge";
          case epoch_metadata::reports::CardRenderType::Timestamp: return "timestamp";
          case epoch_metadata::reports::CardRenderType::Percentage: return "percentage";
          case epoch_metadata::reports::CardRenderType::IconLabel: return "icon_label";
          case epoch_metadata::reports::CardRenderType::MinorNumber: return "minor_number";
          default: return "label";
        }
      },
      "format_hint", &T::format_hint
    );
  };
}

