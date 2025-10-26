//
// Created by adesola on 4/8/25.
//

#pragma once
#include <filesystem>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <epoch_core/enum_wrapper.h>
#include <string>
#include <unordered_set>

// Card selector enums (defined here to avoid circular dependencies)
CREATE_ENUM(CardRenderType,
            Text,        // Generic text/label
            Integer,     // Integer numeric value
            Decimal,     // Decimal/floating point numeric value
            Percent,     // Percentage value
            Monetary,    // Currency/money value
            Duration,    // Duration in nanoseconds
            Badge,       // Badge/pill element
            Timestamp,   // Date/time display
            Boolean);    // True/False indicator

CREATE_ENUM(CardSlot,
            PrimaryBadge,   // Top-left badge
            SecondaryBadge, // Top-right badge
            Hero,           // Center large element
            Subtitle,       // Below hero
            Footer,         // Bottom
            Details);       // "Show More" expandable section

CREATE_ENUM(CardColor,
            Default,  // Neutral/gray
            Primary,  // Brand color
            Info,     // Blue
            Success,  // Green
            Warning,  // Yellow/orange
            Error);   // Red

namespace epoch_metadata {
constexpr auto ARG = "SLOT";
constexpr auto ARG0 = "SLOT0";
constexpr auto ARG1 = "SLOT1";
constexpr auto ARG2 = "SLOT2";
constexpr auto ARG3 = "SLOT3";

// Polygon data source transform IDs
namespace polygon {
constexpr auto BALANCE_SHEET = "balance_sheet";
constexpr auto INCOME_STATEMENT = "income_statement";
constexpr auto CASH_FLOW = "cash_flow";
constexpr auto FINANCIAL_RATIOS = "financial_ratios";
constexpr auto QUOTES = "quotes";
constexpr auto TRADES = "trades";
constexpr auto AGGREGATES = "aggregates";

// Set of all Polygon transform IDs for easy contains checks
inline const std::unordered_set<std::string> ALL_POLYGON_TRANSFORMS = {
    BALANCE_SHEET,
    INCOME_STATEMENT,
    CASH_FLOW,
    FINANCIAL_RATIOS,
    QUOTES,
    TRADES,
    AGGREGATES
};
} // namespace polygon

using FileLoaderInterface = std::function<YAML::Node(std::string const &)>;
using AIGeneratedStrategiesLoader = std::function<std::vector<std::string>()>;
} // namespace epoch_metadata