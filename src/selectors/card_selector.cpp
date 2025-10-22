#include "card_selector.h"
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api.h>
#include <sstream>

namespace epoch_metadata::selectors {

void CardSelectorTransform::generateSelector(const epoch_frame::DataFrame &normalizedDf) const {
  try {
    // Build the card selector output
    auto output = BuildCardSelectorOutput(normalizedDf);

    // Serialize to JSON using glaze
    auto json_result = glz::write_json(output);
    if (json_result) {
      m_selectorData = json_result.value();
    } else {
      std::cerr << "Error: Failed to serialize CardSelectorOutput to JSON" << std::endl;
      m_selectorData = "{}";
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: CardSelectorTransform execution failed: " << e.what() << std::endl;
    m_selectorData = "{}";
  }
}

CardSchemaList CardSelectorTransform::GetCardSchema() const {
  auto options = m_config.GetOptions();
  if (options.contains("card_schema") &&
      options["card_schema"].IsType(epoch_core::MetaDataOptionType::String)) {
    std::string schemaJson = options["card_schema"].GetString();

    // Parse JSON using glaze
    CardSchemaList schema;
    auto error = glz::read_json(schema, schemaJson);
    if (error) {
      std::cerr << "Error parsing card_schema JSON: "
                << glz::format_error(error, schemaJson) << std::endl;
      throw std::runtime_error("Failed to parse card_schema");
    }

    return schema;
  }

  throw std::runtime_error("card_schema option is required");
}

CardSelectorOutput CardSelectorTransform::BuildCardSelectorOutput(const epoch_frame::DataFrame &df) const {
  CardSelectorOutput output;
  output.title = m_schema.title;

  // Apply filtering if specified
  epoch_frame::DataFrame filteredDf = df;

  if (!m_schema.select_key.empty()) {
    // Filter by boolean column using SQL
    std::string filter_sql = "SELECT * FROM input WHERE " + m_schema.select_key;
    auto resultTable = df.query(filter_sql, "input");
    filteredDf = epoch_frame::DataFrame(resultTable);
  } else if (!m_schema.sql.empty()) {
    // Filter by SQL query
    auto resultTable = df.query(m_schema.sql, "input");
    filteredDf = epoch_frame::DataFrame(resultTable);
  }

  // Frontend handles card rendering from the filtered DataFrame
  // The m_schema contains all the configuration needed for the UI to render cards

  return output;
}

std::string CardSelectorTransform::DetermineColor(
    const CardColumnSchema &schema,
    const glz::generic &value) const {

  // Extract string representation of value for matching
  std::string valueStr;
  if (value.is_string()) {
    valueStr = value.get_string();
  } else if (value.is_number()) {
    double numVal = value.get<double>();
    valueStr = std::to_string(numVal);
  } else if (value.is_boolean()) {
    valueStr = value.get<bool>() ? "true" : "false";
  } else {
    return "";  // No color mapping for other types
  }

  // Check each color mapping
  for (const auto& [color, values] : schema.color_map) {
    for (const auto& mappedValue : values) {
      if (valueStr == mappedValue) {
        // Convert enum to string
        switch (color) {
          case epoch_core::CardColor::Default: return "default";
          case epoch_core::CardColor::Primary: return "primary";
          case epoch_core::CardColor::Info: return "info";
          case epoch_core::CardColor::Success: return "success";
          case epoch_core::CardColor::Warning: return "warning";
          case epoch_core::CardColor::Error: return "error";
          default: return "";
        }
      }
    }
  }

  return "";  // No matching color found
}

} // namespace epoch_metadata::selectors
