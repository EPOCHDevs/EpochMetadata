#include "base_card_report.h"
#include <epoch_dashboard/tearsheet/scalar_converter.h>
#include <epoch_frame/dataframe.h>
#include <arrow/compute/api_aggregate.h>
#include <sstream>

namespace epoch_metadata::reports {

void BaseCardReport::generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const {
  // For single input transforms, get the inputId (which is what the column is renamed to)
  auto inputCol = m_config.GetInput();

  // Get the series for the input column
  auto series = normalizedDf[inputCol];

  // Get the single aggregation to apply
  auto aggregation = GetAggregation();

  try {
    // Apply aggregation - handle special cases that require specific options or return structs
    epoch_frame::Scalar result;

    if (aggregation == "stddev") {
      // stddev requires VarianceOptions - use ddof=1 for sample standard deviation (matches test expectations)
      arrow::compute::VarianceOptions options(1);  // ddof=1 for sample statistics
      result = series.stddev(options, epoch_frame::AxisType::Column);
    } else if (aggregation == "variance") {
      // variance requires VarianceOptions - use ddof=1 for sample variance (matches test expectations)
      arrow::compute::VarianceOptions options(1);  // ddof=1 for sample statistics
      result = series.variance(options, epoch_frame::AxisType::Column);
    } else if (aggregation == "skew") {
      // skew requires SkewOptions
      arrow::compute::SkewOptions options = arrow::compute::SkewOptions::Defaults();
      result = series.agg(epoch_frame::AxisType::Column, "skew", true, options);
    } else if (aggregation == "kurtosis") {
      // kurtosis requires SkewOptions (same as skew)
      arrow::compute::SkewOptions options = arrow::compute::SkewOptions::Defaults();
      result = series.agg(epoch_frame::AxisType::Column, "kurtosis", true, options);
    } else if (aggregation == "count_distinct") {
      // count_distinct requires CountOptions
      arrow::compute::CountOptions options = arrow::compute::CountOptions::Defaults();
      result = series.agg(epoch_frame::AxisType::Column, "count_distinct", true, options);
    } else if (aggregation == "quantile") {
      // quantile requires QuantileOptions - handle generically for QuantileCardReport
      auto options = m_config.GetOptions();
      std::cerr << "DEBUG: Processing quantile aggregation" << std::endl;

      // Get quantile value from options
      double quantileValue = 0.5;  // default median
      if (options.contains("quantile")) {
        std::cerr << "DEBUG: Found quantile option" << std::endl;
        if (options["quantile"].IsType(epoch_core::MetaDataOptionType::Decimal)) {
          quantileValue = options["quantile"].GetDecimal();
          std::cerr << "DEBUG: Parsed as Decimal: " << quantileValue << std::endl;
        } else if (options["quantile"].IsType(epoch_core::MetaDataOptionType::Integer)) {
          quantileValue = static_cast<double>(options["quantile"].GetInteger());
          std::cerr << "DEBUG: Parsed as Integer: " << quantileValue << std::endl;
        } else {
          std::cerr << "DEBUG: Quantile option exists but wrong type" << std::endl;
        }
        // Clamp to valid range [0.0, 1.0]
        if (quantileValue < 0.0) quantileValue = 0.0;
        if (quantileValue > 1.0) quantileValue = 1.0;
      } else {
        std::cerr << "DEBUG: No quantile option found, using default: " << quantileValue << std::endl;
      }

      std::cerr << "DEBUG: Final quantile value: " << quantileValue << std::endl;
      std::cerr << "DEBUG: Series dtype: " << series.dtype()->ToString() << std::endl;

      // Create QuantileOptions with custom quantile value and interpolation
      arrow::compute::QuantileOptions quantileOptions;
      quantileOptions.q = {quantileValue};  // Set the quantile value

      // Set default interpolation to LINEAR for consistent behavior
      quantileOptions.interpolation = arrow::compute::QuantileOptions::LINEAR;

      // Handle interpolation if specified
      if (options.contains("interpolation") && options["interpolation"].IsType(epoch_core::MetaDataOptionType::String)) {
        std::string interpMethod = options["interpolation"].GetString();
        std::cerr << "DEBUG: Using interpolation method: " << interpMethod << std::endl;
        if (interpMethod == "linear") {
          quantileOptions.interpolation = arrow::compute::QuantileOptions::LINEAR;
        } else if (interpMethod == "lower") {
          quantileOptions.interpolation = arrow::compute::QuantileOptions::LOWER;
        } else if (interpMethod == "higher") {
          quantileOptions.interpolation = arrow::compute::QuantileOptions::HIGHER;
        } else if (interpMethod == "midpoint") {
          quantileOptions.interpolation = arrow::compute::QuantileOptions::MIDPOINT;
        } else if (interpMethod == "nearest") {
          quantileOptions.interpolation = arrow::compute::QuantileOptions::NEAREST;
        }
      }

      result = series.agg(epoch_frame::AxisType::Column, "quantile", true, quantileOptions);

      // DEBUG: Print the result
      if (!result.is_null()) {
        std::cerr << "DEBUG: Quantile result available (non-null)" << std::endl;
      } else {
        std::cerr << "DEBUG: Quantile result: null" << std::endl;
      }
    } else if (aggregation == "tdigest") {
      // tdigest requires TDigestOptions
      arrow::compute::TDigestOptions options = arrow::compute::TDigestOptions::Defaults();
      result = series.agg(epoch_frame::AxisType::Column, "tdigest", true, options);
    } else if (aggregation == "index") {
      // index requires IndexOptions - handle generically for IndexCardReport
      auto options = m_config.GetOptions();
      std::shared_ptr<arrow::Scalar> arrowScalar;

      std::cerr << "DEBUG: Processing index aggregation" << std::endl;

      if (options.contains("target_value")) {
        // Get the data type of the series to determine the appropriate scalar type
        auto seriesType = series.dtype();
        std::cerr << "DEBUG INDEX: Series dtype: " << seriesType->ToString() << std::endl;

        if (options["target_value"].IsType(epoch_core::MetaDataOptionType::String)) {
          std::string valueStr = options["target_value"].GetString();
          std::cerr << "DEBUG INDEX: target_value as string: '" << valueStr << "'" << std::endl;

          // Convert string to the appropriate type based on series data type
          if (seriesType->id() == arrow::Type::DOUBLE || seriesType->id() == arrow::Type::FLOAT) {
            // Convert string to double for numeric columns
            try {
              double doubleValue = std::stod(valueStr);
              arrowScalar = arrow::MakeScalar(doubleValue);
              std::cerr << "DEBUG INDEX: Converted string to double: " << doubleValue << std::endl;
            } catch (const std::exception& e) {
              std::cerr << "Warning: Could not convert target_value '" << valueStr << "' to double, using 0.0" << std::endl;
              arrowScalar = arrow::MakeScalar(0.0);
            }
          } else if (seriesType->id() == arrow::Type::INT64 || seriesType->id() == arrow::Type::INT32) {
            // Convert string to integer for integer columns
            try {
              int64_t intValue = std::stoll(valueStr);
              arrowScalar = arrow::MakeScalar(intValue);
              std::cerr << "DEBUG INDEX: Converted string to integer: " << intValue << std::endl;
            } catch (const std::exception& e) {
              std::cerr << "Warning: Could not convert target_value '" << valueStr << "' to integer, using 0" << std::endl;
              arrowScalar = arrow::MakeScalar(static_cast<int64_t>(0));
            }
          } else {
            // Keep as string for string columns
            arrowScalar = arrow::MakeScalar(valueStr);
            std::cerr << "DEBUG INDEX: Using string value: '" << valueStr << "'" << std::endl;
          }
        } else if (options["target_value"].IsType(epoch_core::MetaDataOptionType::Integer)) {
          int64_t intValue = options["target_value"].GetInteger();
          std::cerr << "DEBUG INDEX: target_value as integer: " << intValue << std::endl;

          // Convert integer to match series type
          auto seriesType = series.dtype();
          if (seriesType->id() == arrow::Type::DOUBLE || seriesType->id() == arrow::Type::FLOAT) {
            arrowScalar = arrow::MakeScalar(static_cast<double>(intValue));
            std::cerr << "DEBUG INDEX: Converting integer to double for series type: " << seriesType->ToString() << std::endl;
          } else {
            arrowScalar = arrow::MakeScalar(intValue);
            std::cerr << "DEBUG INDEX: Using integer as-is for series type: " << seriesType->ToString() << std::endl;
          }
        } else if (options["target_value"].IsType(epoch_core::MetaDataOptionType::Decimal)) {
          double doubleValue = options["target_value"].GetDecimal();
          std::cerr << "DEBUG INDEX: target_value as decimal: " << doubleValue << std::endl;
          arrowScalar = arrow::MakeScalar(doubleValue);
        }
      } else {
        // Default value based on series type
        auto seriesType = series.dtype();
        if (seriesType->id() == arrow::Type::DOUBLE || seriesType->id() == arrow::Type::FLOAT) {
          arrowScalar = arrow::MakeScalar(0.0);
        } else if (seriesType->id() == arrow::Type::INT64 || seriesType->id() == arrow::Type::INT32) {
          arrowScalar = arrow::MakeScalar(static_cast<int64_t>(0));
        } else {
          arrowScalar = arrow::MakeScalar(std::string(""));
        }
        std::cerr << "DEBUG INDEX: Using default target value based on series type" << std::endl;
      }

      arrow::compute::IndexOptions indexOptions(arrowScalar);
      result = series.agg(epoch_frame::AxisType::Column, "index", true, indexOptions);
    } else if (aggregation == "product") {
      // product requires ScalarAggregateOptions
      arrow::compute::ScalarAggregateOptions options = arrow::compute::ScalarAggregateOptions::Defaults();
      result = series.agg(epoch_frame::AxisType::Column, "product", true, options);
    } else if (aggregation == "count_all") {
      // count_all requires no arguments - count all elements including nulls
      arrow::compute::CountOptions options = arrow::compute::CountOptions::Defaults();
      options.mode = arrow::compute::CountOptions::ALL;
      result = series.agg(epoch_frame::AxisType::Column, "count", true, options);
    } else if (aggregation == "first_last" || aggregation == "min_max" || aggregation == "mode") {
      // These functions return structs - handle them specially
      result = series.agg(epoch_frame::AxisType::Column, aggregation);
      // TODO: Struct handling - for now treat as scalar, but should extract struct fields
    } else {
      // Use generic agg method for other aggregations
      result = series.agg(epoch_frame::AxisType::Column, aggregation);
    }

    // Skip if result is null
    if (result.is_null()) {
      std::cerr << "Warning: Aggregation '" << aggregation << "' returned null for column '" << inputCol << "'" << std::endl;
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
    epoch_proto::Scalar scalarValue;
    try {
      scalarValue = epoch_tearsheet::ScalarFactory::create(result);
    } catch (const std::exception& e) {
      std::cerr << "Error: Failed to convert scalar to protobuf: " << e.what() << std::endl;
      // Try to handle integer case manually if ScalarFactory fails
      // For now, just return to avoid crash
      return;
    }
    DataBuilder.setValue(scalarValue);

    // Set the type based on the protobuf scalar type
    // NOTE: There's a known issue in ScalarFactory where false boolean values
    // don't set any value field in the protobuf, causing type detection to fail
    if (scalarValue.has_boolean_value()) {
      DataBuilder.setType(epoch_proto::TypeBoolean);
    } else if (scalarValue.has_integer_value()) {
      DataBuilder.setType(epoch_proto::TypeInteger);
    } else if (scalarValue.has_decimal_value()) {
      DataBuilder.setType(epoch_proto::TypeDecimal);
    } else if (scalarValue.has_string_value()) {
      DataBuilder.setType(epoch_proto::TypeString);
    }
    // If none of the above, leave type unset (will use default)

    // Set the group number
    DataBuilder.setGroup(GetGroup());

    // Add the single card
    cardBuilder.addCardData(DataBuilder.build());

    m_dashboard.addCard(cardBuilder.build());

  } catch (const std::exception& e) {
    // Aggregation failed, return empty
    std::cerr << "Error: Aggregation '" << aggregation << "' failed with exception: " << e.what() << std::endl;
    return;
  }
}

std::string BaseCardReport::GetCategory() const {
  auto options = m_config.GetOptions();
  if (options.contains("category") && options["category"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["category"].GetString();
  }
  return "";
}

std::string BaseCardReport::GetTitle() const {
  auto options = m_config.GetOptions();
  if (options.contains("title") && options["title"].IsType(epoch_core::MetaDataOptionType::String)) {
    return options["title"].GetString();
  }
  return "";  // Empty means auto-generate
}

uint32_t BaseCardReport::GetGroup() const {
  auto options = m_config.GetOptions();
  if (options.contains("group") && (options["group"].IsType(epoch_core::MetaDataOptionType::Integer) || options["group"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    return static_cast<uint32_t>(options["group"].GetDecimal());
  }
  return 0;  // Default to group 0
}

uint32_t BaseCardReport::GetGroupSize() const {
  auto options = m_config.GetOptions();
  if (options.contains("group_size") && (options["group_size"].IsType(epoch_core::MetaDataOptionType::Integer) || options["group_size"].IsType(epoch_core::MetaDataOptionType::Decimal))) {
    return static_cast<uint32_t>(options["group_size"].GetDecimal());
  }
  return 1;  // Default to single card
}

epoch_proto::EpochFolioDashboardWidget BaseCardReport::GetWidgetType() const {
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