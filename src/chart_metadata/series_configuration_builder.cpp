#include "series_configuration_builder.h"
#include "epoch_script/chart_metadata/chart_metadata_provider.h"
#include "data_column_resolver.h"
#include "epoch_script/data/common/constants.h"
#include "plot_kinds/registry.h"
#include <algorithm>
#include <functional>
#include <regex>
#include <spdlog/spdlog.h> // For SPDLOG_DEBUG

namespace epoch_script::chart_metadata {

namespace {
// Constants for chart types
constexpr const char *CANDLESTICK_CHART = "candlestick";
constexpr const char *VOLUME_CHART = "column";

} // anonymous namespace

std::string SeriesConfigurationBuilder::BuildDescriptiveName(
    const epoch_script::transform::TransformConfiguration &cfg) {
  std::stringstream ss;

  // Get the ID and convert to uppercase like TradingView
  const auto &metadata = cfg.GetTransformDefinition().GetMetadata();
  std::string baseName = metadata.id;

  // Convert to uppercase for display
  std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::toupper);
  ss << baseName;

  // Use the actual configured options, not the default ones
  const auto &configuredOptions = cfg.GetOptions();
  for (const auto &[optionId, optionValue] : configuredOptions) {
    ss << " ";
    std::visit(
        [&ss]<typename T>(const T &arg) {
          if constexpr (std::is_same_v<T, std::string>) {
            ss << arg;
          } else if constexpr (std::is_same_v<T, double>) {
            bool isInteger = std::floor(arg) == arg;
            if (isInteger) {
              ss << static_cast<int64_t>(arg);
            } else {
              ss << std::fixed << std::setprecision(2) << arg;
            }
          } else if constexpr (std::is_same_v<T, bool>) {
            ss << std::boolalpha << arg;
          } else if constexpr (std::is_same_v<T,
                                              epoch_script::MetaDataArgRef>) {
            ss << "$" << arg.refName;
          }
        },
        optionValue.GetVariant());
  }
  return ss.str();
}

SeriesInfo SeriesConfigurationBuilder::BuildSeries(
    const epoch_script::transform::TransformConfiguration &cfg, uint8_t chosenAxis,
    const std::optional<std::string> &linkedTo, const std::string &seriesId) {

  // Get plot kind from metadata
  const auto &metadata = cfg.GetTransformDefinition().GetMetadata();
  auto plotKind = metadata.plotKind; // Use enum directly

  // Get registry instance and build data mapping
  auto &registry = plot_kinds::PlotKindBuilderRegistry::Instance();
  auto dataMapping = registry.Build(plotKind, cfg);

  // Build descriptive name with parameters
  std::string displayName = BuildDescriptiveName(cfg);

  SeriesInfo series;
  series.id = seriesId;

  // Convert PlotKind enum to string using wrapper
  series.type = epoch_core::TransformPlotKindWrapper::ToString(plotKind);

  // Get metadata directly from builder via registry
  series.zIndex = registry.GetZIndex(plotKind);
  series.name = displayName;
  series.dataMapping = dataMapping;
  series.yAxis = chosenAxis;
  series.linkedTo = linkedTo;

  // Build config options for annotations and thresholds
  series.configOptions = BuildConfigOptions(cfg);

  return series;
}

SeriesInfo SeriesConfigurationBuilder::BuildCandlestickSeries(
    const std::string &timeframe) {
  const auto &C = epoch_script::EpochStratifyXConstants::instance();

  SeriesInfo series;
  series.id = std::format("{}_candlestick", timeframe);
  series.type = CANDLESTICK_CHART;
  series.name = "";
  series.dataMapping = {{"index", INDEX_COLUMN},
                        {"open", C.OPEN()},
                        {"high", C.HIGH()},
                        {"low", C.LOW()},
                        {"close", C.CLOSE()}};
  series.zIndex = 0; // Candlestick always has z-index 0
  series.yAxis = 0;
  series.linkedTo = std::nullopt;

  // Initialize empty config options for candlestick series (no transform
  // config)
  series.configOptions = {};

  return series;
}

SeriesInfo
SeriesConfigurationBuilder::BuildVolumeSeries(const std::string &timeframe) {
  const auto &C = epoch_script::EpochStratifyXConstants::instance();

  SeriesInfo series;
  series.id = std::format("{}_volume", timeframe);
  series.type = VOLUME_CHART;
  series.name = "Volume";
  series.dataMapping = {{"index", INDEX_COLUMN}, {"value", C.VOLUME()}};
  series.zIndex = 0; // Volume always has z-index 0
  series.yAxis = 1;
  series.linkedTo = std::nullopt;

  // Initialize empty config options for volume series (no transform config)
  series.configOptions = {};

  return series;
}

// SeriesInfo SeriesConfigurationBuilder::BuildExitLevelsSeries(
//     const std::string &timeframe) {
//
//   SeriesInfo exitLevels;
//   exitLevels.id = std::format("{}_exit_levels", timeframe);
//   exitLevels.type = "exit_levels";
//   exitLevels.name = "Exit Levels";
//   exitLevels.dataMapping = {{"index", INDEX_COLUMN},
//                             {"stop_loss", "stop_loss"},
//                             {"take_profit", "take_profit"}};
//   exitLevels.zIndex = 100; // Stop Loss always has z-index 100
//   exitLevels.yAxis = 0;
//   exitLevels.linkedTo = std::format("{}_candlestick", timeframe);
//
//   return exitLevels;
// }
//
// SeriesInfo SeriesConfigurationBuilder::BuildPositionQuantitySeries(
//     const std::string &timeframe) {
//   SeriesInfo series;
//   series.id = std::format("{}_qty", timeframe);
//   series.type = "position";
//   series.name = "Position Size";
//   series.dataMapping = {{"index", INDEX_COLUMN}, {"value", "quantity"}};
//   series.zIndex = 0; // Equity always has z-index 0
//   series.yAxis = 2;
//   series.linkedTo = std::nullopt;
//
//   return series;
// }

bool SeriesConfigurationBuilder::IsIntradayTimeframe(
    const std::string &timeframe) {
  // Check for minute-based timeframes (e.g., 1Min, 5Min, 15Min, 30Min)
  if (std::regex_match(timeframe, std::regex(R"(\d+Min)"))) {
    return true;
  }

  // Check for hour-based timeframes (e.g., 1H, 2H, 4H)
  if (std::regex_match(timeframe, std::regex(R"(\d+H)"))) {
    return true;
  }

  // Check for second-based timeframes (e.g., 30S, 1S)
  if (std::regex_match(timeframe, std::regex(R"(\d+S)"))) {
    return true;
  }

  return false;
}

epoch_script::MetaDataArgDefinitionMapping
SeriesConfigurationBuilder::BuildConfigOptions(
    const epoch_script::transform::TransformConfiguration &cfg) {

  epoch_script::MetaDataArgDefinitionMapping configOptions;

  try {
    // Extract only the configured options from cfg.GetOptions()
    // Return them as-is with their original variant types
    configOptions = cfg.GetOptions();

    // Add default options based on PlotKind
    const auto &metadata = cfg.GetTransformDefinition().GetMetadata();

    // Zone PlotKind defaults
    if (metadata.plotKind == epoch_core::TransformPlotKind::zone) {
      // Add default 'name' if not present (use transform display name)
      if (configOptions.find("name") == configOptions.end()) {
        configOptions["name"] = epoch_script::MetaDataOptionDefinition{metadata.name};
      }

      // Add default 'position' if not present (center)
      if (configOptions.find("position") == configOptions.end()) {
        configOptions["position"] = epoch_script::MetaDataOptionDefinition{std::string("center")};
      }
    }

    // Flag PlotKind defaults - add flag display metadata
    if (metadata.plotKind == epoch_core::TransformPlotKind::flag) {
      // Set flag title to transform name
      if (configOptions.find("flagTitle") == configOptions.end()) {
        configOptions["flagTitle"] = epoch_script::MetaDataOptionDefinition{metadata.name};
      }

      // Set flagText and flagTextIsTemplate based on transform ID
      if (configOptions.find("flagText") == configOptions.end()) {
        std::string flagText;
        bool isTemplate = false;
        std::string icon = "Info"; // Default icon

        // Determine flag text template and icon based on transform ID
        if (metadata.id == "news") {
          flagText = "<b>{title}</b><br/>{description}<br/><i>By {author}</i>";
          isTemplate = true;
          icon = "Info";
        } else if (metadata.id == "dividends") {
          flagText = "${cash_amount} {currency} dividend<br/>Ex-Date: {record_date}<br/>Pay Date: {pay_date}";
          isTemplate = true;
          icon = "Dollar";
        } else if (metadata.id == "splits") {
          flagText = "Split: {split_from}:{split_to} (Ratio: {split_ratio})";
          isTemplate = true;
          icon = "Split";
        } else if (metadata.id == "ticker_events") {
          flagText = "{event_type}: {name}";
          isTemplate = true;
          icon = "Alert";
        } else if (metadata.id == "short_interest") {
          flagText = "Short Interest: {short_interest}<br/>Days to Cover: {days_to_cover}";
          isTemplate = true;
          icon = "TrendDown";
        } else if (metadata.id == "short_volume") {
          flagText = "Short Volume: {short_volume} ({short_volume_ratio}%)<br/>Total: {total_volume}";
          isTemplate = true;
          icon = "TrendDown";
        } else if (metadata.id == "balance_sheet") {
          flagText = "Q{fiscal_quarter} {fiscal_year} Balance Sheet<br/>Cash: ${cash}<br/>Debt: ${long_term_debt}";
          isTemplate = true;
          icon = "Dollar";
        } else if (metadata.id == "cash_flow") {
          flagText = "Q{fiscal_quarter} {fiscal_year} Cash Flow<br/>CFO: ${cfo}<br/>FCF: ${fcf}";
          isTemplate = true;
          icon = "Dollar";
        } else if (metadata.id == "income_statement") {
          flagText = "Q{fiscal_quarter} {fiscal_year} Earnings<br/>Revenue: ${revenue}<br/>Net Income: ${net_income}<br/>EPS: ${diluted_eps}";
          isTemplate = true;
          icon = "Dollar";
        } else if (metadata.id == "financial_ratios") {
          flagText = "P/E: {pe}<br/>P/B: {pb}<br/>ROE: {roe}%<br/>Debt/Equity: {debt_equity}";
          isTemplate = true;
          icon = "Chart";
        } else if (metadata.id == "economic_indicator") {
          flagText = "Value: {value}";
          isTemplate = true;
          icon = "TrendUp";
        } else {
          // Default for other flags (candlestick patterns, etc.)
          flagText = metadata.name;
          isTemplate = false;
          icon = "Signal";
        }

        configOptions["flagText"] = epoch_script::MetaDataOptionDefinition{flagText};
        configOptions["flagTextIsTemplate"] = epoch_script::MetaDataOptionDefinition{isTemplate};
        configOptions["flagIcon"] = epoch_script::MetaDataOptionDefinition{icon};
      }
    }

  } catch (...) {
    // If any error occurs, return empty options (which is fine)
    SPDLOG_DEBUG("Failed to build config options for transform: {}",
                 cfg.GetId());
  }

  return configOptions;
}

} // namespace epoch_script::chart_metadata