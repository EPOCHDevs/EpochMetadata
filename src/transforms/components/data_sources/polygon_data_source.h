#pragma once

#include <epoch_script/core/constants.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_frame/dataframe.h>

namespace epoch_script::transform {

// Polygon data source transform
// Handles all Polygon data types (balance_sheet, income_statement, cash_flow, etc.)
// The specific data type is determined by the transform ID in the configuration
class PolygonDataSourceTransform final : public ITransform {
public:
  explicit PolygonDataSourceTransform(const TransformConfiguration &config)
      : ITransform(config) {
    // Build column rename mapping from Polygon API field names to output IDs
    for (auto const &outputMetaData : config.GetOutputs()) {
      m_replacements[outputMetaData.id] = config.GetOutputId(outputMetaData.id);
    }
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &data) const override {
    // External loader has already fetched data from Polygon API
    // and converted to DataFrame with expected column names.
    // We just rename columns to match the node's output IDs.
    return data.rename(m_replacements);
  }

private:
  std::unordered_map<std::string, std::string> m_replacements;
};

// Type aliases for each Polygon data source (for backward compatibility and clarity)
using PolygonBalanceSheetTransform = PolygonDataSourceTransform;
using PolygonIncomeStatementTransform = PolygonDataSourceTransform;
using PolygonCashFlowTransform = PolygonDataSourceTransform;
using PolygonFinancialRatiosTransform = PolygonDataSourceTransform;
// NOTE: Quotes and Trades not yet fully implemented - backend data loading disabled
// using PolygonQuotesTransform = PolygonDataSourceTransform;
// using PolygonTradesTransform = PolygonDataSourceTransform;
using PolygonCommonIndicesTransform = PolygonDataSourceTransform;
using PolygonIndicesTransform = PolygonDataSourceTransform;

// New data source transforms (using MetadataRegistry)
using PolygonNewsTransform = PolygonDataSourceTransform;
using PolygonDividendsTransform = PolygonDataSourceTransform;
using PolygonSplitsTransform = PolygonDataSourceTransform;
using PolygonTickerEventsTransform = PolygonDataSourceTransform;
using PolygonShortInterestTransform = PolygonDataSourceTransform;
using PolygonShortVolumeTransform = PolygonDataSourceTransform;

} // namespace epoch_script::transform
