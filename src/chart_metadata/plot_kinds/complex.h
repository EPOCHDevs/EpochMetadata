#pragma once

#include "base.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief Ichimoku Cloud: 5 components
 */
struct IchimokuDataMapping {
  std::string tenkan;
  std::string kijun;
  std::string senkou_a;
  std::string senkou_b;
  std::string chikou;
};

/**
 * @brief Chande Kroll Stop: 2 stop levels
 */
struct ChandeKrollStopDataMapping {
  std::string long_stop;
  std::string short_stop;
};

/**
 * @brief Pivot Point Support/Resistance: 7 levels
 */
struct PivotPointSRDataMapping {
  std::string pivot;
  std::string resist_1;
  std::string support_1;
  std::string resist_2;
  std::string support_2;
  std::string resist_3;
  std::string support_3;
};

/**
 * @brief Previous High/Low: 4 outputs
 */
struct PreviousHighLowDataMapping {
  std::string previous_high;
  std::string previous_low;
  std::string broken_high;
  std::string broken_low;
};

/**
 * @brief Retracements: 3 outputs
 */
struct RetracementsDataMapping {
  std::string direction;
  std::string current_retracement;
  std::string deepest_retracement;
};

/**
 * @brief Gap indicator: 5 outputs
 */
struct GapDataMapping {
  std::string gap_filled;
  std::string gap_retrace;
  std::string gap_size;
  std::string psc;
  std::string psc_timestamp;
};

/**
 * @brief Swing Highs/Lows: 2 outputs
 */
struct SHLDataMapping {
  std::string high_low;
  std::string level;
};

/**
 * @brief Break of Structure / Change of Character: 4 outputs
 */
struct BosChochDataMapping {
  std::string bos;
  std::string choch;
  std::string level;
  std::string broken_index;
};

/**
 * @brief Order Blocks: 6 outputs
 */
struct OrderBlocksDataMapping {
  std::string ob;
  std::string top;
  std::string bottom;
  std::string ob_volume;
  std::string mitigated_index;
  std::string percentage;
};

/**
 * @brief Fair Value Gap: 4 outputs
 */
struct FVGDataMapping {
  std::string fvg;
  std::string top;
  std::string bottom;
  std::string mitigated_index;
};

/**
 * @brief Liquidity: 4 outputs
 */
struct LiquidityDataMapping {
  std::string liquidity;
  std::string level;
  std::string end;
  std::string swept;
};

/**
 * @brief Sessions: 5 outputs
 */
struct SessionsDataMapping {
  std::string active;
  std::string high;
  std::string low;
  std::string closed;
  std::string opened;
};

/**
 * @brief Trade Signal (from inputs, not outputs): maps input handles
 */
struct TradeSignalDataMapping {
  // Populated from cfg.GetInputs() instead of outputs
  std::unordered_map<std::string, std::string> input_mappings;
};

/**
 * @brief Flag (generic event markers): maps all outputs dynamically
 */
struct FlagDataMapping {
  // Dynamic mapping of all outputs for template substitution
  std::unordered_map<std::string, std::string> all_outputs;
};

/**
 * @brief Zone (boolean highlighting): single boolean value
 */
struct ZoneDataMapping {
  std::string value;  // Boolean column
};

/**
 * @brief Close Line: only maps 'c' (close) column
 */
struct CloseLineDataMapping {
  std::string c;  // Close price column
};

/**
 * @brief Pivot Point Detector: 3 outputs
 */
struct PivotPointDetectorDataMapping {
  std::string pivot_type;
  std::string pivot_level;
  std::string pivot_index;
};

/**
 * @brief Flag Pattern: 4 outputs
 */
struct FlagPatternDataMapping {
  std::string bull_flag;
  std::string bear_flag;
  std::string slmax;
  std::string slmin;
};

/**
 * @brief Pennant Pattern: 4 outputs
 */
struct PennantPatternDataMapping {
  std::string bull_pennant;
  std::string bear_pennant;
  std::string slmax;
  std::string slmin;
};

/**
 * @brief Triangle Patterns: 4 outputs
 */
struct TrianglePatternsDataMapping {
  std::string pattern_detected;
  std::string upper_slope;
  std::string lower_slope;
  std::string triangle_type;
};

/**
 * @brief Consolidation Box: 9 outputs
 */
struct ConsolidationBoxDataMapping {
  std::string box_detected;
  std::string box_top;
  std::string box_bottom;
  std::string box_height;
  std::string touch_count;
  std::string upper_slope;
  std::string lower_slope;
  std::string target_up;
  std::string target_down;
};

/**
 * @brief Double Top/Bottom: 3 outputs
 */
struct DoubleTopBottomDataMapping {
  std::string pattern_detected;
  std::string breakout_level;
  std::string target;
};

/**
 * @brief Head and Shoulders: 3 outputs
 */
struct HeadAndShouldersDataMapping {
  std::string pattern_detected;
  std::string neckline_level;
  std::string target;
};

/**
 * @brief Inverse Head and Shoulders: 3 outputs
 */
struct InverseHeadAndShouldersDataMapping {
  std::string pattern_detected;
  std::string neckline_level;
  std::string target;
};

/**
 * @brief Hidden Markov Model: variable outputs (state + probabilities)
 * HMM can have 2-5 states, dynamically mapped
 */
struct HMMDataMapping {
  std::string state;
  std::unordered_map<std::string, std::string> state_probabilities;  // state_0_prob, state_1_prob, etc.
};

} // namespace epoch_script::chart_metadata::plot_kinds
