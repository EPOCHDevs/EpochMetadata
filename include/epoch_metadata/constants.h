//
// Created by adesola on 4/8/25.
//

#pragma once
#include <filesystem>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <epoch_core/enum_wrapper.h>

// Data source enums (defined here to avoid circular dependencies)
CREATE_ENUM(PolygonDataType,
            BalanceSheet,     // Balance sheet fundamental data
            IncomeStatement,  // Income statement fundamental data
            CashFlow,         // Cash flow statement fundamental data
            FinancialRatios,  // Financial ratios and metrics
            Quotes,           // Quote (NBBO) data
            Trades,           // Trade tick data
            Aggregates);      // Aggregate bars (OHLCV)

// Card selector enums (defined here to avoid circular dependencies)
CREATE_ENUM(CardRenderType,
            Text,       // Generic text/label
            Number,     // Numeric value
            Badge,      // Badge/pill element
            Timestamp,  // Date/time display
            Boolean,    // True/False indicator
            Icon,       // Icon display
            Navigator); // Chart navigation column (timestamp/index)

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

using FileLoaderInterface = std::function<YAML::Node(std::string const &)>;
using AIGeneratedStrategiesLoader = std::function<std::vector<std::string>()>;
} // namespace epoch_metadata