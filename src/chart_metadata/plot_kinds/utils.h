#pragma once

#include "base.h"
#include <unordered_map>
#include <string>

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Convert PlotKindDataMapping variant to string map for SeriesInfo
 * This bridges the typed PlotKind system with the existing SeriesInfo structure
 */
inline std::unordered_map<std::string, std::string> ConvertToStringMap(
  const PlotKindDataMapping &variant
) {
  return std::visit([](const auto &mapping) -> std::unordered_map<std::string, std::string> {
    using T = std::decay_t<decltype(mapping)>;
    std::unordered_map<std::string, std::string> result;

    // Always include index
    result["index"] = mapping.index;

    // Handle specific mapping types
    if constexpr (std::is_same_v<T, MACDDataMapping>) {
      result["macd"] = mapping.macd;
      result["macd_signal"] = mapping.macd_signal;
      result["macd_histogram"] = mapping.macd_histogram;
    }
    else if constexpr (std::is_same_v<T, BbandsDataMapping>) {
      result["bbands_upper"] = mapping.bbands_upper;
      result["bbands_middle"] = mapping.bbands_middle;
      result["bbands_lower"] = mapping.bbands_lower;
    }
    else if constexpr (std::is_same_v<T, AroonDataMapping>) {
      result["aroon_up"] = mapping.aroon_up;
      result["aroon_down"] = mapping.aroon_down;
    }
    else if constexpr (std::is_same_v<T, StochDataMapping>) {
      result["stoch_k"] = mapping.stoch_k;
      result["stoch_d"] = mapping.stoch_d;
    }
    else if constexpr (std::is_same_v<T, FisherDataMapping>) {
      result["fisher"] = mapping.fisher;
      result["fisher_signal"] = mapping.fisher_signal;
    }
    else if constexpr (std::is_same_v<T, QQEDataMapping>) {
      result["result"] = mapping.result;
      result["rsi_ma"] = mapping.rsi_ma;
      result["long_line"] = mapping.long_line;
      result["short_line"] = mapping.short_line;
    }
    else if constexpr (std::is_same_v<T, EldersDataMapping>) {
      result["result"] = mapping.result;
      result["ema"] = mapping.ema;
      result["buy_signal"] = mapping.buy_signal;
      result["sell_signal"] = mapping.sell_signal;
    }
    else if constexpr (std::is_same_v<T, FOscDataMapping>) {
      result["result"] = mapping.result;
    }
    else if constexpr (std::is_same_v<T, VortexDataMapping>) {
      result["plus_indicator"] = mapping.plus_indicator;
      result["minus_indicator"] = mapping.minus_indicator;
    }
    else if constexpr (std::is_same_v<T, IchimokuDataMapping>) {
      result["tenkan"] = mapping.tenkan;
      result["kijun"] = mapping.kijun;
      result["senkou_a"] = mapping.senkou_a;
      result["senkou_b"] = mapping.senkou_b;
      result["chikou"] = mapping.chikou;
    }
    else if constexpr (std::is_same_v<T, ChandeKrollStopDataMapping>) {
      result["long_stop"] = mapping.long_stop;
      result["short_stop"] = mapping.short_stop;
    }
    else if constexpr (std::is_same_v<T, PivotPointSRDataMapping>) {
      result["pivot"] = mapping.pivot;
      result["resist_1"] = mapping.resist_1;
      result["support_1"] = mapping.support_1;
      result["resist_2"] = mapping.resist_2;
      result["support_2"] = mapping.support_2;
      result["resist_3"] = mapping.resist_3;
      result["support_3"] = mapping.support_3;
    }
    else if constexpr (std::is_same_v<T, PreviousHighLowDataMapping>) {
      result["previous_high"] = mapping.previous_high;
      result["previous_low"] = mapping.previous_low;
      result["broken_high"] = mapping.broken_high;
      result["broken_low"] = mapping.broken_low;
    }
    else if constexpr (std::is_same_v<T, RetracementsDataMapping>) {
      result["direction"] = mapping.direction;
      result["current_retracement"] = mapping.current_retracement;
      result["deepest_retracement"] = mapping.deepest_retracement;
    }
    else if constexpr (std::is_same_v<T, GapDataMapping>) {
      result["gap_filled"] = mapping.gap_filled;
      result["gap_retrace"] = mapping.gap_retrace;
      result["gap_size"] = mapping.gap_size;
      result["psc"] = mapping.psc;
      result["psc_timestamp"] = mapping.psc_timestamp;
    }
    else if constexpr (std::is_same_v<T, SHLDataMapping>) {
      result["high_low"] = mapping.high_low;
      result["level"] = mapping.level;
    }
    else if constexpr (std::is_same_v<T, BosChochDataMapping>) {
      result["bos"] = mapping.bos;
      result["choch"] = mapping.choch;
      result["level"] = mapping.level;
      result["broken_index"] = mapping.broken_index;
    }
    else if constexpr (std::is_same_v<T, OrderBlocksDataMapping>) {
      result["ob"] = mapping.ob;
      result["top"] = mapping.top;
      result["bottom"] = mapping.bottom;
      result["ob_volume"] = mapping.ob_volume;
      result["mitigated_index"] = mapping.mitigated_index;
      result["percentage"] = mapping.percentage;
    }
    else if constexpr (std::is_same_v<T, FVGDataMapping>) {
      result["fvg"] = mapping.fvg;
      result["top"] = mapping.top;
      result["bottom"] = mapping.bottom;
      result["mitigated_index"] = mapping.mitigated_index;
    }
    else if constexpr (std::is_same_v<T, LiquidityDataMapping>) {
      result["liquidity"] = mapping.liquidity;
      result["level"] = mapping.level;
      result["end"] = mapping.end;
      result["swept"] = mapping.swept;
    }
    else if constexpr (std::is_same_v<T, SessionsDataMapping>) {
      result["active"] = mapping.active;
      result["high"] = mapping.high;
      result["low"] = mapping.low;
      result["closed"] = mapping.closed;
      result["opened"] = mapping.opened;
    }
    else if constexpr (std::is_same_v<T, PivotPointDetectorDataMapping>) {
      result["pivot_type"] = mapping.pivot_type;
      result["pivot_level"] = mapping.pivot_level;
      result["pivot_index"] = mapping.pivot_index;
    }
    else if constexpr (std::is_same_v<T, FlagPatternDataMapping>) {
      result["bull_flag"] = mapping.bull_flag;
      result["bear_flag"] = mapping.bear_flag;
      result["slmax"] = mapping.slmax;
      result["slmin"] = mapping.slmin;
    }
    else if constexpr (std::is_same_v<T, PennantPatternDataMapping>) {
      result["bull_pennant"] = mapping.bull_pennant;
      result["bear_pennant"] = mapping.bear_pennant;
      result["slmax"] = mapping.slmax;
      result["slmin"] = mapping.slmin;
    }
    else if constexpr (std::is_same_v<T, TrianglePatternsDataMapping>) {
      result["pattern_detected"] = mapping.pattern_detected;
      result["upper_slope"] = mapping.upper_slope;
      result["lower_slope"] = mapping.lower_slope;
      result["triangle_type"] = mapping.triangle_type;
    }
    else if constexpr (std::is_same_v<T, ConsolidationBoxDataMapping>) {
      result["box_detected"] = mapping.box_detected;
      result["box_top"] = mapping.box_top;
      result["box_bottom"] = mapping.box_bottom;
      result["box_height"] = mapping.box_height;
      result["touch_count"] = mapping.touch_count;
      result["upper_slope"] = mapping.upper_slope;
      result["lower_slope"] = mapping.lower_slope;
      result["target_up"] = mapping.target_up;
      result["target_down"] = mapping.target_down;
    }
    else if constexpr (std::is_same_v<T, DoubleTopBottomDataMapping>) {
      result["pattern_detected"] = mapping.pattern_detected;
      result["breakout_level"] = mapping.breakout_level;
      result["target"] = mapping.target;
    }
    else if constexpr (std::is_same_v<T, HeadAndShouldersDataMapping>) {
      result["pattern_detected"] = mapping.pattern_detected;
      result["neckline_level"] = mapping.neckline_level;
      result["target"] = mapping.target;
    }
    else if constexpr (std::is_same_v<T, InverseHeadAndShouldersDataMapping>) {
      result["pattern_detected"] = mapping.pattern_detected;
      result["neckline_level"] = mapping.neckline_level;
      result["target"] = mapping.target;
    }
    else if constexpr (std::is_same_v<T, TradeSignalDataMapping>) {
      // Copy all input mappings
      for (const auto &[key, value] : mapping.input_mappings) {
        result[key] = value;
      }
    }
    else if constexpr (std::is_same_v<T, FlagDataMapping>) {
      // Copy all output mappings
      for (const auto &[key, value] : mapping.all_outputs) {
        result[key] = value;
      }
    }
    else if constexpr (std::is_same_v<T, ZoneDataMapping>) {
      result["value"] = mapping.value;
    }
    else if constexpr (std::is_same_v<T, CloseLineDataMapping>) {
      result["c"] = mapping.c;
    }
    else if constexpr (std::is_same_v<T, HMMDataMapping>) {
      result["state"] = mapping.state;
      for (const auto &[key, value] : mapping.state_probabilities) {
        result[key] = value;
      }
    }
    else if constexpr (std::is_same_v<T, SingleValueDataMapping>) {
      result["value"] = mapping.value;
    }

    return result;
  }, variant);
}

} // namespace epoch_script::chart_metadata::plot_kinds
