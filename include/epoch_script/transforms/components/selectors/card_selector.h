#pragma once

#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/metadata.h>
#include <epoch_script/core/metadata_options.h>
#include <epoch_frame/dataframe.h>
#include <glaze/glaze.hpp>

namespace epoch_script::transform {

// Card Selector - filters DataFrame by boolean column and displays as interactive cards
class CardSelectorFromFilter : public epoch_script::transform::ITransform {
public:
  explicit CardSelectorFromFilter(epoch_script::transform::TransformConfiguration config)
      : ITransform(std::move(config)),
        m_schema(GetSchemaFromConfig(config)) {}

  epoch_frame::DataFrame TransformData(const epoch_frame::DataFrame &df) const override {
    // Filter by boolean column specified in select_key
    epoch_frame::DataFrame result = df.loc(df[m_schema.select_key]);

    // Collect selector data and store in base class
    result = result.reset_index("pivot");
    this->SetSelectorData(SelectorData(
      m_schema.title,
      m_schema.schemas,
      result,
      m_schema.schemas.size()-1,
      m_schema.icon
    ));

    return result;
  }

  CardSchemaFilter GetSchema() const { return m_schema; }

private:
  static CardSchemaFilter GetSchemaFromConfig(const epoch_script::transform::TransformConfiguration& config) {
    CardSchemaFilter schema = config.GetOptionValue("card_schema").GetCardSchemaList();

    // Automatically add index column as timestamp for chart navigation
    schema.schemas.emplace_back(CardColumnSchema{
    .column_id = "pivot",
    .slot = epoch_core::CardSlot::Subtitle,
    .render_type = epoch_core::CardRenderType::Timestamp,
    .color_map = {},
    .label = std::nullopt
    });
    return schema;
  }

  const CardSchemaFilter m_schema;
};

// Metadata for CardSelectorFromFilter
struct SelectorMetadata {
  constexpr static const char *kSelectorId = "card_selector_filter";

  static epoch_script::transforms::TransformsMetaData Get() {
    return {
      .id = kSelectorId,
      .category = epoch_core::TransformCategory::Selector,
      .name = "Card Selector",
      .options = {
        {.id = "card_schema",
         .name = "Card Schema",
         .type = epoch_core::MetaDataOptionType::CardSchema,
         .isRequired = true,
         .desc = std::string("Card layout configuration using boolean column filter. The 'select_key' field specifies a boolean column name to filter rows (only rows where the column is true are shown as cards). For SQL filtering, use a SQL Transform node first, then pipe output to this selector. JSON Schema:\n") + glz::write_json_schema<CardSchemaFilter>().value_or("{}")}
      },
      .isCrossSectional = false,
      .desc = "Generate an interactive card selector where each row is a clickable card, filtered by a boolean column. "
              "Click a card to navigate to that timestamp on the candlestick chart. "
              "Accepts multiple input columns via SLOT connection. "
              "For SQL-based filtering, use a SQL Transform node before this selector.",
      .inputs = {
        {.type = epoch_core::IODataType::Any,
         .id = "SLOT",
         .name = "Columns",
         .allowMultipleConnections = true}
      },
      .outputs = {},  // Selector outputs via GetSelectorData()
      .atLeastOneInputRequired = true,
      .tags = {"selector", "interactive", "cards", "navigation", "timepoint", "filter"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epoch_script::transform
