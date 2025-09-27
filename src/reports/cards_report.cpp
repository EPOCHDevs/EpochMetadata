#include "cards_report.h"
#include <epoch_dashboard/tearsheet/scalar_converter.h>
#include <epoch_frame/dataframe.h>
#include <sstream>

namespace epoch_metadata::reports {

void CardsReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  // Get the single input column
  auto inputCol = GetInputId();

  // Get the series for the input column
  auto series = normalizedDf[inputCol];

  // Get the single aggregation to apply
  auto aggregation = GetAggregation();

  try {
    // Apply aggregation
    auto result = series.agg(epoch_frame::AxisType::Column, aggregation);

    // Skip if result is null
    if (result.is_null()) {
      return;
    }

    // Create CardDef using builder
    epoch_tearsheet::CardBuilder cardBuilder;

    cardBuilder.setType(GetWidgetType())
               .setCategory(GetCategory())
               .setGroupSize(GetGroupSize());

    // Build card data
    epoch_tearsheet::CardDataBuilder DataBuilder;

    // Set title - use custom title or generate from aggregation
    std::string title = GetTitle();
    if (title.empty()) {
      std::ostringstream titleStream;
      titleStream << aggregation << "(" << inputCol << ")";
      title = titleStream.str();
    }
    DataBuilder.setTitle(title);

    // Use ScalarFactory to directly convert epoch_frame::Scalar to epoch_proto::Scalar
    epoch_proto::Scalar scalarValue = epoch_tearsheet::ScalarFactory::create(result);
    DataBuilder.setValue(scalarValue);

    // Set the group number
    DataBuilder.setGroup(GetGroup());

    // Add the single card
    cardBuilder.addCardData(DataBuilder.build());

    m_dashboard.addCard(cardBuilder.build());

  } catch (const std::exception&) {
    // Aggregation failed, return empty
    return;
  }
}

std::string CardsReport::GetAggregation() const {
  auto options = m_config.GetOptions();
  if (options.contains("agg") && options["agg"].IsType(epoch_core::MetaDataOptionType::Select)) {
    return options["agg"].GetSelectOption();
  }
  return "last";  // Default
}

std::string CardsReport::GetCategory() const {
  auto options = m_config.GetOptions();
  if (options.contains("category") && options["category"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["category"].GetString();
  }
  return "";
}

std::string CardsReport::GetTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";  // Empty means auto-generate
}

uint32_t CardsReport::GetGroup() const {
  auto options = m_config.GetOptions();
  if (options.contains("group") && (options["group"].IsType(epoch_core::MetaDataOptionType::Integer) || options["group"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    return static_cast<uint32_t>(options["group"].GetDecimal());
  }
  return 0;  // Default to group 0
}

uint32_t CardsReport::GetGroupSize() const {
  auto options = m_config.GetOptions();
  if (options.contains("group_size") && (options["group_size"].IsType(epoch_core::MetaDataOptionType::Integer) || options["group_size"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    return static_cast<uint32_t>(options["group_size"].GetDecimal());
  }
  return 1;  // Default to single card
}

epoch_proto::EpochFolioDashboardWidget CardsReport::GetWidgetType() const {
  auto options = m_config.GetOptions();
  if (options.contains("widget_type") && options["widget_type"].IsType(epoch_core::MetaDataOptionType::String)) {
    std::string type = options["widget_type"].GetString();
    // Map string to enum
    if (type == "CARD") {
      return epoch_proto::EpochFolioDashboardWidget::WidgetCard;
    } else if (type == "METRIC") {
      return epoch_proto::EpochFolioDashboardWidget::WidgetCard; // METRIC maps to WidgetCard
    }
    // Add more mappings as needed
  }
  return epoch_proto::EpochFolioDashboardWidget::WidgetCard;  // Default widget type
}

} // namespace epoch_metadata::reports