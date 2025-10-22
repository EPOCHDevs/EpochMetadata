#pragma once

#include "epoch_metadata/constants.h"
#include "epoch_metadata/transforms/itransform.h"
#include <epoch_frame/dataframe.h>

namespace epoch_metadata::transform {

// Template class for Polygon data sources
// Each specialization handles a specific Polygon data type with static output metadata
template <epoch_core::PolygonDataType DataType>
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

// Type aliases for each Polygon data source specialization
using PolygonBalanceSheetTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::BalanceSheet>;
using PolygonIncomeStatementTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::IncomeStatement>;
using PolygonCashFlowTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::CashFlow>;
using PolygonFinancialRatiosTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::FinancialRatios>;
using PolygonQuotesTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::Quotes>;
using PolygonTradesTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::Trades>;
using PolygonAggregatesTransform =
    PolygonDataSourceTransform<epoch_core::PolygonDataType::Aggregates>;

} // namespace epoch_metadata::transform
