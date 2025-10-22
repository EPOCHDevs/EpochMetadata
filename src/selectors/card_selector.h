#pragma once

#include <epoch_metadata/selectors/iselector.h>
#include <epoch_metadata/metadata_options.h>
#include <epoch_frame/dataframe.h>
#include <glaze/glaze.hpp>

namespace epoch_metadata::selectors {

// Output structure for a single card
struct CardData {
  std::unordered_map<std::string, glz::generic> fields;  // column_id -> value
  std::unordered_map<std::string, std::string> colors;   // column_id -> color
  glz::generic navigator_value;  // Value from Navigator column for chart navigation
};

// Output structure for the card selector widget
struct CardSelectorOutput {
  std::string title;
  std::vector<CardData> cards;
};

class CardSelectorTransform : public ISelector {
public:
  explicit CardSelectorTransform(epoch_metadata::transform::TransformConfiguration config)
      : ISelector(std::move(config), false),
        m_schema(GetCardSchema()) {
  }

protected:
  void generateSelector(const epoch_frame::DataFrame &normalizedDf) const override;

private:
  const CardSchemaList m_schema;

  // Configuration getters
  CardSchemaList GetCardSchema() const;

  // Helper methods
  CardSelectorOutput BuildCardSelectorOutput(const epoch_frame::DataFrame &df) const;
  std::string DetermineColor(const CardColumnSchema &schema, const glz::generic &value) const;
};

// Template specialization for CardSelectorTransform metadata
template <> struct SelectorMetadata<CardSelectorTransform> {
  constexpr static const char *kSelectorId = "card_selector";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kSelectorId,
      .category = epoch_core::TransformCategory::Selector,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Card Selector",
      .options = {
        {.id = "card_schema",
         .name = "Card Schema",
         .type = epoch_core::MetaDataOptionType::String,
         .isRequired = true,
         .desc = R"(JSON configuration for card layout and behavior. Example:
{
  "title": "Trade Signals",
  "select_key": "is_signal",
  "sql": "",
  "schemas": [
    {"column_id": "direction", "slot": "PrimaryBadge", "render_type": "Badge", "color_map": {"Success": ["BUY"], "Error": ["SELL"]}},
    {"column_id": "profit_pct", "slot": "Hero", "render_type": "Number", "color_map": {"Success": [], "Error": []}},
    {"column_id": "timestamp", "slot": "Footer", "render_type": "Navigator", "color_map": {}}
  ]
}
Use either 'select_key' (boolean column to filter) or 'sql' (SQL query), not both.)"}
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
      .outputs = {},  // Selector outputs via GetSelectorData()
      .atLeastOneInputRequired = true,
      .tags = {"selector", "interactive", "cards", "navigation", "timepoint"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epoch_metadata::selectors

// Glaze metadata for output structures
namespace glz {
  template <>
  struct meta<epoch_metadata::selectors::CardData> {
    using T = epoch_metadata::selectors::CardData;
    static constexpr auto value = object(
      "fields", &T::fields,
      "colors", &T::colors,
      "navigator_value", &T::navigator_value
    );
  };

  template <>
  struct meta<epoch_metadata::selectors::CardSelectorOutput> {
    using T = epoch_metadata::selectors::CardSelectorOutput;
    static constexpr auto value = object(
      "title", &T::title,
      "cards", &T::cards
    );
  };
}
