#pragma once
//
// Data Category Mapper
// Central mapping between EpochScript transform IDs and data_sdk DataCategory enums
//

#include <epoch_data_sdk/common/enums.hpp>
#include <epoch_data_sdk/dataloader/metadata_registry.hpp>
#include <epoch_script/core/time_frame.h>
#include <optional>
#include <string>

namespace epoch_script::data_sources {

using data_sdk::DataCategory;

/**
 * Maps an EpochScript transform ID to its corresponding AUXILIARY DataCategory
 *
 * NOTE: This function does NOT map time-series transforms (market_data_source, vwap,
 * trade_count, indices, us_reference_stocks). Those represent the PRIMARY category
 * (MinuteBars/DailyBars) which is determined by IsIntradayCampaign() in the strategy
 * analysis, not by individual transforms.
 *
 * This function only maps auxiliary data categories like news, dividends, financials, etc.
 *
 * @param transformId The transform ID (e.g., "balance_sheet", "news", "dividends")
 * @return DataCategory if the transform maps to an auxiliary category, nullopt otherwise
 *
 * Returns nullopt for:
 * - Time-series transforms (market_data_source, vwap, indices, etc.)
 * - FRED transforms (economic_indicator) - not in DataCategory enum
 * - SEC transforms (form13f_holdings, insider_trading) - not in DataCategory enum
 * - Indicator/operator transforms - don't require specific data categories
 */
std::optional<DataCategory> GetDataCategoryForTransform(
    std::string const& transformId);

/**
 * Determines if a DataCategory requires intraday-only data
 *
 * The intradayOnly flag is automatically computed from the MetadataRegistry:
 * intradayOnly = !index_normalized
 *
 * - index_normalized = false → data has time-of-day (intraday)
 * - index_normalized = true → data normalized to midnight UTC (EOD/daily)
 *
 * @param category The DataCategory to check
 * @return true if the category requires intraday data (index_normalized = false)
 */
bool IsIntradayOnlyCategory(DataCategory category);

/**
 * Gets the category prefix for a DataCategory from MetadataRegistry
 *
 * Prefixes are used when merging different data categories:
 * - "" (empty) for timeseries (MinuteBars, DailyBars)
 * - "N:" for News
 * - "D:" for Dividends
 * - etc.
 *
 * @param category The DataCategory
 * @return The prefix string for this category
 */
std::string GetCategoryPrefix(DataCategory category);

} // namespace epoch_script::data_sources
