#pragma once

#include "base.h"

namespace epoch_script::chart_metadata::plot_kinds {

/**
 * @brief MACD indicator: 3 outputs (macd, signal, histogram)
 */
struct MACDDataMapping {
  std::string macd;
  std::string macd_signal;
  std::string macd_histogram;
};

/**
 * @brief Aroon indicator: 2 outputs (aroon_up, aroon_down)
 */
struct AroonDataMapping {
  std::string aroon_up;
  std::string aroon_down;
};

/**
 * @brief Stochastic oscillator: 2 outputs (stoch_k, stoch_d)
 */
struct StochDataMapping {
  std::string stoch_k;
  std::string stoch_d;
};

/**
 * @brief Fisher Transform: 2 outputs (fisher, fisher_signal)
 */
struct FisherDataMapping {
  std::string fisher;
  std::string fisher_signal;
};

/**
 * @brief QQE indicator: 4 outputs
 */
struct QQEDataMapping {
  std::string result;
  std::string rsi_ma;
  std::string long_line;
  std::string short_line;
};

/**
 * @brief Elder's Thermometer: 4 outputs
 */
struct EldersDataMapping {
  std::string result;
  std::string ema;
  std::string buy_signal;
  std::string sell_signal;
};

/**
 * @brief Forecast Oscillator: single output
 */
struct FOscDataMapping {
  std::string result;
};

/**
 * @brief Vortex Indicator: 2 outputs (plus_indicator, minus_indicator)
 */
struct VortexDataMapping {
  std::string plus_indicator;
  std::string minus_indicator;
};

} // namespace epoch_script::chart_metadata::plot_kinds
