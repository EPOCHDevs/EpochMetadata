#pragma once

#include <epoch_metadata/transforms/itransform.h>
#include <epoch_metadata/transforms/metadata.h>
#include <epoch_metadata/metadata_options.h>
#include <epoch_frame/dataframe.h>
#include <glaze/glaze.hpp>

namespace epoch_metadata::selectors {

// Forward declare SelectorMetadata template
template <typename T> struct SelectorMetadata;

// Card Selector using boolean column filter - returns filtered DataFrame
class CardSelectorFromFilter : public epoch_metadata::transform::ITransform {
public:
  explicit CardSelectorFromFilter(epoch_metadata::transform::TransformConfiguration config)
      : ITransform(std::move(config)),
        m_schema([this]() {
          // Parse JSON string into CardSchemaFilter
          std::string jsonStr = std::get<std::string>(m_config.GetOptionValue("card_schema").GetVariant());
          CardSchemaFilter schema;
          auto error = glz::read_json(schema, jsonStr);
          if (error) {
            throw std::runtime_error("Failed to parse CardSchemaFilter JSON: " + glz::format_error(error, jsonStr));
          }
          return schema;
        }()) {
  }

  epoch_frame::DataFrame TransformData(const epoch_frame::DataFrame &df) const override {
    // Filter by boolean column specified in select_key
    return df.loc(df[m_schema.select_key]);
  }

  CardSchemaFilter GetSchema() const { return m_schema; }

private:
  const CardSchemaFilter m_schema;
};

// Card Selector using SQL query - returns filtered DataFrame
class CardSelectorFromSQL : public epoch_metadata::transform::ITransform {
public:
  explicit CardSelectorFromSQL(epoch_metadata::transform::TransformConfiguration config)
      : ITransform(std::move(config)),
        m_schema([this]() {
          // Parse JSON string into CardSchemaSQL
          std::string jsonStr = std::get<std::string>(m_config.GetOptionValue("card_schema").GetVariant());
          CardSchemaSQL schema;
          auto error = glz::read_json(schema, jsonStr);
          if (error) {
            throw std::runtime_error("Failed to parse CardSchemaSQL JSON: " + glz::format_error(error, jsonStr));
          }
          return schema;
        }()) {
  }

  epoch_frame::DataFrame TransformData(const epoch_frame::DataFrame &df) const override {
    // Build input rename mapping (SLOT0, SLOT1, SLOT2, ...)
    auto inputRenameMap = this->BuildVARGInputRenameMapping();

    // Rename data columns to SLOT0, SLOT1, SLOT2, ...
    epoch_frame::DataFrame inputDf = df.rename(inputRenameMap);

    // Apply SQL filtering - DuckDB registers the DataFrame as 'self' by default
    return epoch_frame::DataFrame(inputDf.query(m_schema.sql));
  }

  CardSchemaSQL GetSchema() const { return m_schema; }

private:
  const CardSchemaSQL m_schema;
};

// Metadata specialization for CardSelectorFromFilter
template <> struct SelectorMetadata<CardSelectorFromFilter> {
  constexpr static const char *kSelectorId = "card_selector_filter";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kSelectorId,
      .category = epoch_core::TransformCategory::Selector,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Card Selector (Filter)",
      .options = {
        {.id = "card_schema",
         .name = "Card Schema",
         .type = epoch_core::MetaDataOptionType::CardSchema,
         .isRequired = true,
         .desc = std::string("Card layout configuration using boolean column filter. The 'select_key' field specifies a boolean column name to filter rows (only rows where the column is true are shown as cards). JSON Schema:\n") + glz::write_json_schema<CardSchemaFilter>().value_or("{}")}
      },
      .isCrossSectional = false,
      .desc = "Generate an interactive card selector where each row is a clickable card, filtered by a boolean column. "
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
      .tags = {"selector", "interactive", "cards", "navigation", "timepoint", "filter"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

// Metadata specialization for CardSelectorFromSQL
template <> struct SelectorMetadata<CardSelectorFromSQL> {
  constexpr static const char *kSelectorId = "card_selector_sql";

  static epoch_metadata::transforms::TransformsMetaData Get() {
    return {
      .id = kSelectorId,
      .category = epoch_core::TransformCategory::Selector,
      .renderKind = epoch_core::TransformNodeRenderKind::Output,
      .name = "Card Selector (SQL)",
      .options = {
        {.id = "card_schema",
         .name = "Card Schema",
         .type = epoch_core::MetaDataOptionType::CardSchema,
         .isRequired = true,
         .desc = std::string("Card layout configuration using SQL query filter. The 'sql' field contains the query (MUST use 'FROM self'). Input columns are renamed to SLOT0, SLOT1, SLOT2, etc. JSON Schema:\n") + glz::write_json_schema<CardSchemaSQL>().value_or("{}")}
      },
      .isCrossSectional = false,
      .desc = "Generate an interactive card selector where each row is a clickable card, filtered by a SQL query. "
              "Click a card to navigate to that timestamp on the candlestick chart. "
              "SQL queries use 'FROM self' and input columns are named SLOT0, SLOT1, etc.",
      .inputs = {
        {.type = epoch_core::IODataType::Any,
         .id = "SLOT",
         .name = "Columns",
         .allowMultipleConnections = true}
      },
      .outputs = {},  // Selector outputs via GetSelectorData()
      .atLeastOneInputRequired = true,
      .tags = {"selector", "interactive", "cards", "navigation", "timepoint", "sql"},
      .requiresTimeFrame = false,
      .allowNullInputs = false
    };
  }
};

} // namespace epoch_metadata::selectors
