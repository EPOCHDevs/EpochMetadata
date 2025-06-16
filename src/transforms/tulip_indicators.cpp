#include "common.h"
#include "epoch_core/common_utils.h"
#include "epoch_metadata/transforms/metadata.h"
#include "indicators.h"
#include <epoch_core/ranges_to.h>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata::transforms {

struct IndicatorMetaData {
  std::vector<std::string> tags;
  std::string desc;
  epoch_core::TransformCategory category{epoch_core::TransformCategory::Math};
  epoch_core::TransformNodeRenderKind renderKind{
      epoch_core::TransformNodeRenderKind::Simple};
  epoch_core::TransformPlotKind plotKind{epoch_core::TransformPlotKind::Null};
};

std::unordered_map<std::string, IndicatorMetaData>
MakeTulipIndicatorMetaData() {
  std::unordered_map<std::string, IndicatorMetaData> indicatorMetaData;

  // Vector operations and math functions
  indicatorMetaData["abs"] =
      IndicatorMetaData{.tags = {"simple", "abs", "math", "vector"},
                        .desc = "Vector Absolute Value. Returns the absolute "
                                "value of each element in the input."};

  indicatorMetaData["acos"] = IndicatorMetaData{
      .tags = {"simple", "acos", "math", "trigonometric", "vector"},
      .desc = "Vector Arccosine. Calculates the arccosine (inverse cosine) for "
              "each element in the input."};

  indicatorMetaData["add"] = IndicatorMetaData{
      .tags = {"simple", "add", "math", "arithmetic", "vector"},
      .desc = "Vector Addition. Adds two vectors element by element."};

  indicatorMetaData["asin"] = IndicatorMetaData{
      .tags = {"simple", "asin", "math", "trigonometric", "vector"},
      .desc = "Vector Arcsine. Calculates the arcsine (inverse sine) for each "
              "element in the input."};

  indicatorMetaData["atan"] = IndicatorMetaData{
      .tags = {"simple", "atan", "math", "trigonometric", "vector"},
      .desc = "Vector Arctangent. Calculates the arctangent (inverse tangent) "
              "for each element in the input."};

  indicatorMetaData["ceil"] = IndicatorMetaData{
      .tags = {"simple", "ceil", "math", "rounding", "vector"},
      .desc = "Vector Ceiling. Rounds each element up to the nearest integer."};

  indicatorMetaData["cos"] = IndicatorMetaData{
      .tags = {"simple", "cos", "math", "trigonometric", "vector"},
      .desc = "Vector Cosine. Calculates the cosine for each element in the "
              "input."};

  indicatorMetaData["cosh"] = IndicatorMetaData{
      .tags = {"simple", "cosh", "math", "hyperbolic", "vector"},
      .desc = "Vector Hyperbolic Cosine. Calculates the hyperbolic cosine for "
              "each element in the input."};

  indicatorMetaData["crossany"] = IndicatorMetaData{
      .tags = {"math", "crossany", "crossover", "signal"},
      .desc = "Crossany. Returns 1 when the first input "
              "crosses the second input in any direction.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::flag};

  indicatorMetaData["crossover"] = IndicatorMetaData{
      .tags = {"math", "crossover", "signal", "trend"},
      .desc = "Crossover. Returns 1 when the first input "
              "crosses above the second input.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::flag};

  indicatorMetaData["decay"] = IndicatorMetaData{
      .tags = {"math", "decay", "linear"},
      .desc = "Linear Decay. Applies linear decay to each element in the input "
              "over the specified period.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["div"] = IndicatorMetaData{
      .tags = {"simple", "div", "math", "arithmetic", "vector"},
      .desc = "Vector Division. Divides the first vector by the second element "
              "by element."};

  indicatorMetaData["edecay"] = IndicatorMetaData{
      .tags = {"math", "edecay", "exponential"},
      .desc = "Exponential Decay. Applies exponential decay to each element in "
              "the input over the specified period.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["exp"] = IndicatorMetaData{
      .tags = {"simple", "exp", "math", "exponential", "vector"},
      .desc = "Vector Exponential. Calculates e raised to the power of each "
              "element in the input.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::Null};

  indicatorMetaData["floor"] = IndicatorMetaData{
      .tags = {"simple", "floor", "math", "rounding", "vector"},
      .desc = "Vector Floor. Rounds each element down to the nearest integer."};

  indicatorMetaData["lag"] = IndicatorMetaData{
      .tags = {"math", "lag", "delay", "shift"},
      .desc = "Lag. Shifts each element in the input by the "
              "specified period, creating a lagged series.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["ln"] =
      IndicatorMetaData{.tags = {"simple", "ln", "math", "logarithm", "vector"},
                        .desc = "Vector Natural Log. Calculates the natural "
                                "logarithm for each element in the input."};

  indicatorMetaData["log10"] = IndicatorMetaData{
      .tags = {"simple", "log10", "math", "logarithm", "vector"},
      .desc = "Vector Base-10 Log. Calculates the base-10 logarithm for each "
              "element in the input."};

  indicatorMetaData["max"] = IndicatorMetaData{
      .tags = {"math", "max", "maximum", "highest"},
      .desc = "Maximum In Period. Finds the maximum value in the specified "
              "period for each element position.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["md"] = IndicatorMetaData{
      .tags = {"math", "md", "mean-deviation", "statistics"},
      .desc = "Mean Deviation Over Period. Calculates the "
              "mean deviation over the specified period.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["min"] = IndicatorMetaData{
      .tags = {"math", "min", "minimum", "lowest"},
      .desc = "Minimum In Period. Finds the minimum value in the specified "
              "period for each element position.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["mul"] = IndicatorMetaData{
      .tags = {"simple", "mul", "math", "arithmetic", "vector"},
      .desc =
          "Vector Multiplication. Multiplies two vectors element by element."};

  indicatorMetaData["round"] = IndicatorMetaData{
      .tags = {"simple", "round", "math", "rounding", "vector"},
      .desc = "Vector Round. Rounds each element to the nearest integer."};

  indicatorMetaData["sin"] = IndicatorMetaData{
      .tags = {"simple", "sin", "math", "trigonometric", "vector"},
      .desc =
          "Vector Sine. Calculates the sine for each element in the input."};

  indicatorMetaData["sinh"] = IndicatorMetaData{
      .tags = {"simple", "sinh", "math", "hyperbolic", "vector"},
      .desc = "Vector Hyperbolic Sine. Calculates the hyperbolic sine for each "
              "element in the input."};

  indicatorMetaData["sqrt"] =
      IndicatorMetaData{.tags = {"simple", "sqrt", "math", "vector"},
                        .desc = "Vector Square Root. Calculates the square "
                                "root for each element in the input."};

  indicatorMetaData["stddev"] = IndicatorMetaData{
      .tags = {"math", "stddev", "standard-deviation", "statistics",
               "volatility"},
      .desc = "Standard Deviation Over Period. Calculates the standard "
              "deviation over the specified period.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["stderr"] = IndicatorMetaData{
      .tags = {"math", "stderr", "standard-error", "statistics"},
      .desc = "Standard Error Over Period. Calculates the standard error over "
              "the specified period.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["sub"] = IndicatorMetaData{
      .tags = {"simple", "sub", "math", "arithmetic", "vector"},
      .desc = "Vector Subtraction. Subtracts the second vector from the first "
              "element by element."};

  indicatorMetaData["sum"] = IndicatorMetaData{
      .tags = {"math", "sum", "cumulative", "total"},
      .desc = "Sum Over Period. Calculates the sum over the "
              "specified period for each element position.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::Null};

  indicatorMetaData["tan"] = IndicatorMetaData{
      .tags = {"simple", "tan", "math", "trigonometric", "vector"},
      .desc = "Vector Tangent. Calculates the tangent for each element in the "
              "input."};

  indicatorMetaData["tanh"] = IndicatorMetaData{
      .tags = {"simple", "tanh", "math", "hyperbolic", "vector"},
      .desc = "Vector Hyperbolic Tangent. Calculates the hyperbolic tangent "
              "for each element in the input."};

  indicatorMetaData["todeg"] = IndicatorMetaData{
      .tags = {"simple", "todeg", "math", "conversion", "vector"},
      .desc = "Vector Degree Conversion. Converts radian values to degrees for "
              "each element in the input."};

  indicatorMetaData["torad"] = IndicatorMetaData{
      .tags = {"simple", "torad", "math", "conversion", "vector"},
      .desc = "Vector Radian Conversion. Converts degree values to radians for "
              "each element in the input."};

  indicatorMetaData["trunc"] = IndicatorMetaData{
      .tags = {"simple", "trunc", "math", "rounding", "vector"},
      .desc = "Vector Truncate. Truncates the decimal part of each element in "
              "the input."};

  indicatorMetaData["var"] = IndicatorMetaData{
      .tags = {"math", "var", "variance", "statistics", "volatility"},
      .desc = "Variance Over Period. Calculates the variance over the "
              "specified period.",
      .category = epoch_core::TransformCategory::Math,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  // Technical indicators
  indicatorMetaData["ad"] = IndicatorMetaData{
      .tags = {"indicator", "ad", "volume", "accumulation-distribution"},
      .desc =
          "Accumulation/Distribution Line. Volume-based indicator designed to "
          "measure cumulative flow of money into and out of a security.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["adosc"] = IndicatorMetaData{
      .tags = {"indicator", "adosc", "volume", "oscillator"},
      .desc = "Accumulation/Distribution Oscillator. Indicates momentum in the "
              "Accumulation/Distribution Line using two moving averages.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["adx"] = IndicatorMetaData{
      .tags = {"indicator", "adx", "trend", "directional-movement"},
      .desc = "Average Directional Movement Index. Measures the strength of a "
              "trend, regardless of its direction.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["adxr"] = IndicatorMetaData{
      .tags = {"indicator", "adxr", "trend", "directional-movement"},
      .desc = "Average Directional Movement Rating. Smoothed version of ADX, "
              "provides trend direction information.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["ao"] = IndicatorMetaData{
      .tags = {"indicator", "ao", "momentum", "oscillator"},
      .desc = "Awesome Oscillator. Measures market momentum by comparing a "
              "5-period and 34-period simple moving average.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::ao};

  indicatorMetaData["apo"] = IndicatorMetaData{
      .tags = {"indicator", "apo", "moving-average", "oscillator", "momentum"},
      .desc = "Absolute Price Oscillator. Shows the difference between two "
              "exponential moving averages as an absolute value.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["aroon"] = IndicatorMetaData{
      .tags = {"indicator", "aroon", "trend", "oscillator"},
      .desc = "Aroon. Measures the time between highs and lows over a time "
              "period, identifying trends and corrections.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::aroon};

  indicatorMetaData["aroonosc"] = IndicatorMetaData{
      .tags = {"indicator", "aroonosc", "trend", "oscillator"},
      .desc = "Aroon Oscillator. Subtracts Aroon Down from Aroon Up, measuring "
              "the strength of a prevailing trend.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["atr"] = IndicatorMetaData{
      .tags = {"indicator", "atr", "volatility", "average-true-range"},
      .desc = "Average True Range. Measures market volatility by calculating "
              "the average range between price points.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["avgprice"] = IndicatorMetaData{
      .tags = {"overlay", "avgprice", "price", "average"},
      .desc = "Average Price. Calculates the average of "
              "open, high, low, and close prices.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["bbands"] = IndicatorMetaData{
      .tags = {"overlay", "bbands", "volatility", "bands", "bollinger"},
      .desc = "Bollinger Bands. Volatility bands placed above and below a "
              "moving average, adapting to market conditions.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::bbands};

  indicatorMetaData["bop"] = IndicatorMetaData{
      .tags = {"indicator", "bop", "price", "balance-of-power", "momentum"},
      .desc = "Balance of Power. Measures buying and selling pressure by "
              "comparing closing price to trading range.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["cci"] = IndicatorMetaData{
      .tags = {"indicator", "cci", "momentum", "commodity-channel-index"},
      .desc = "Commodity Channel Index. Identifies cyclical turns in price and "
              "measures variations from the statistical mean.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::cci};

  indicatorMetaData["cmo"] = IndicatorMetaData{
      .tags = {"indicator", "cmo", "momentum", "oscillator"},
      .desc = "Chande Momentum Oscillator. Momentum oscillator calculating "
              "relative momentum of positive and negative price movements.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["cvi"] = IndicatorMetaData{
      .tags = {"indicator", "cvi", "volatility", "chaikins"},
      .desc = "Chaikins Volatility. Measures volatility by tracking the "
              "difference between high and low prices over a period.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["dema"] = IndicatorMetaData{
      .tags = {"overlay", "dema", "moving-average", "double-exponential"},
      .desc = "Double Exponential Moving Average. Moving average that reduces "
              "lag with a double smoothing mechanism.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["di"] = IndicatorMetaData{
      .tags = {"indicator", "di", "trend", "directional-indicator"},
      .desc = "Directional Indicator. Components of ADX that measure positive "
              "and negative price movement strength.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["dm"] = IndicatorMetaData{
      .tags = {"indicator", "dm", "trend", "directional-movement"},
      .desc = "Directional Movement. Identifies whether prices are trending by "
              "comparing consecutive highs and lows.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["dpo"] = IndicatorMetaData{
      .tags = {"indicator", "dpo", "trend", "detrended-oscillator"},
      .desc = "Detrended Price Oscillator. Eliminates long-term trends to "
              "focus on short to medium-term cycles.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["dx"] = IndicatorMetaData{
      .tags = {"indicator", "dx", "trend", "directional-movement"},
      .desc = "Directional Movement Index. Measures trending strength by "
              "comparing +DI and -DI indicators.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["ema"] = IndicatorMetaData{
      .tags = {"overlay", "ema", "moving-average", "exponential"},
      .desc = "Exponential Moving Average. Moving average that gives more "
              "weight to recent prices, reducing lag.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["emv"] = IndicatorMetaData{
      .tags = {"indicator", "emv", "volume", "ease-of-movement"},
      .desc = "Ease of Movement. Relates price change to volume, identifying "
              "whether price changes are easy or difficult.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["fisher"] = IndicatorMetaData{
      .tags = {"indicator", "fisher", "transform", "oscillator"},
      .desc = "Fisher Transform. Converts prices to a Gaussian normal "
              "distribution to identify extreme price movements.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::fisher};

  indicatorMetaData["fosc"] = IndicatorMetaData{
      .tags = {"indicator", "fosc", "oscillator", "forecast"},
      .desc = "Forecast Oscillator. Compares price to linear regression "
              "forecast value, indicating when price deviates from trend.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::fosc};

  indicatorMetaData["hma"] = IndicatorMetaData{
      .tags = {"overlay", "hma", "moving-average", "hull"},
      .desc = "Hull Moving Average. Moving average designed to reduce lag and "
              "improve smoothness by using weighted averages.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["kama"] = IndicatorMetaData{
      .tags = {"overlay", "kama", "moving-average", "adaptive", "kaufman"},
      .desc = "Kaufman Adaptive Moving Average. Adjusts sensitivity "
              "automatically based on market volatility.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["kvo"] = IndicatorMetaData{
      .tags = {"indicator", "kvo", "volume", "oscillator", "klinger"},
      .desc = "Klinger Volume Oscillator. Compares volume to price trends to "
              "identify reversals and divergence.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["linreg"] = IndicatorMetaData{
      .tags = {"overlay", "linreg", "linear-regression", "trend"},
      .desc = "Linear Regression. Plots a best-fit line through price data, "
              "showing overall direction of price movement.",
      .category = epoch_core::TransformCategory::Statistical,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["linregintercept"] = IndicatorMetaData{
      .tags = {"indicator", "linregintercept", "linear-regression", "trend",
               "statistics"},
      .desc = "Linear Regression Intercept. Calculates the y-intercept values "
              "for linear regression analysis.",
      .category = epoch_core::TransformCategory::Statistical,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["linregslope"] = IndicatorMetaData{
      .tags = {"indicator", "linregslope", "linear-regression", "trend",
               "statistics"},
      .desc = "Linear Regression Slope. Measures the rate of change in linear "
              "regression values, indicating trend strength.",
      .category = epoch_core::TransformCategory::Statistical,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["macd"] = IndicatorMetaData{
      .tags = {"indicator", "macd", "moving-average", "trend", "momentum"},
      .desc = "Moving Average Convergence/Divergence. Trend-following momentum "
              "indicator showing relationship between two moving averages.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::macd};

  indicatorMetaData["marketfi"] = IndicatorMetaData{
      .tags = {"indicator", "marketfi", "volume", "market-facilitation-index"},
      .desc = "Market Facilitation Index. Measures market readiness to move "
              "prices with minimal volume.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::column};

  indicatorMetaData["mass"] = IndicatorMetaData{
      .tags = {"indicator", "mass", "volatility", "index"},
      .desc = "Mass Index. Identifies potential reversals by examining "
              "high-low range expansion and contraction.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["medprice"] = IndicatorMetaData{
      .tags = {"overlay", "medprice", "price", "average"},
      .desc = "Median Price. Simple average of the high and "
              "low prices for each period.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["mfi"] = IndicatorMetaData{
      .tags = {"indicator", "mfi", "volume", "money-flow-index", "oscillator"},
      .desc = "Money Flow Index. Volume-weighted RSI that measures buying and "
              "selling pressure based on price and volume.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::rsi};

  indicatorMetaData["mom"] = IndicatorMetaData{
      .tags = {"indicator", "mom", "momentum", "rate-of-change"},
      .desc = "Momentum. Measures rate of change in prices by comparing "
              "current price to a previous price.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["msw"] = IndicatorMetaData{
      .tags = {"indicator", "msw", "cycle", "sine-wave"},
      .desc = "Mesa Sine Wave. Identifies market cycles "
              "using sine waves derived from price data.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["natr"] = IndicatorMetaData{
      .tags = {"indicator", "natr", "volatility",
               "normalized-average-true-range"},
      .desc = "Normalized Average True Range. ATR expressed as a percentage of "
              "closing price, allowing comparison across securities.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["nvi"] = IndicatorMetaData{
      .tags = {"indicator", "nvi", "volume", "negative-volume-index"},
      .desc = "Negative Volume Index. Shows price movements on days when "
              "volume decreases, highlighting smart money activity.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["obv"] = IndicatorMetaData{
      .tags = {"indicator", "obv", "volume", "on-balance-volume"},
      .desc = "On Balance Volume. Running total of volume that adds when price "
              "rises and subtracts when price falls.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["ppo"] = IndicatorMetaData{
      .tags = {"indicator", "ppo", "momentum", "percentage-price-oscillator"},
      .desc = "Percentage Price Oscillator. Shows relationship between two "
              "moving averages as a percentage, similar to MACD.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["psar"] = IndicatorMetaData{
      .tags = {"overlay", "psar", "trend", "parabolic-sar"},
      .desc = "Parabolic SAR. Identifies potential reversals in price "
              "movement, providing entry and exit signals.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::psar};

  indicatorMetaData["pvi"] = IndicatorMetaData{
      .tags = {"indicator", "pvi", "volume", "positive-volume-index"},
      .desc = "Positive Volume Index. Shows price movements on days when "
              "volume increases, highlighting public participation.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["qstick"] = IndicatorMetaData{
      .tags = {"indicator", "qstick", "candlestick", "trend"},
      .desc = "Qstick. Measures the ratio of black to white candlesticks, "
              "indicating buying and selling pressure.",
      .category = epoch_core::TransformCategory::PriceAction,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::qstick};

  indicatorMetaData["roc"] = IndicatorMetaData{
      .tags = {"indicator", "roc", "momentum", "rate-of-change"},
      .desc = "Rate of Change. Measures percentage change between current "
              "price and price n periods ago.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["rocr"] = IndicatorMetaData{
      .tags = {"indicator", "rocr", "momentum", "rate-of-change-ratio"},
      .desc = "Rate of Change Ratio. Calculates the ratio of current price to "
              "price n periods ago, measuring momentum.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["rsi"] = IndicatorMetaData{
      .tags = {"indicator", "rsi", "momentum", "oscillator",
               "relative-strength"},
      .desc = "Relative Strength Index. Momentum oscillator measuring speed "
              "and change of price movements, indicating overbought/oversold "
              "conditions.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::rsi};

  indicatorMetaData["sma"] = IndicatorMetaData{
      .tags = {"overlay", "sma", "moving-average", "simple"},
      .desc = "Simple Moving Average. Unweighted mean of previous n data "
              "points, smoothing price data to identify trends.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["stoch"] = IndicatorMetaData{
      .tags = {"indicator", "stoch", "momentum", "oscillator", "stochastic"},
      .desc =
          "Stochastic Oscillator. Compares closing price to price range over a "
          "period, indicating momentum and overbought/oversold conditions.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::stoch};

  indicatorMetaData["stochrsi"] = IndicatorMetaData{
      .tags = {"indicator", "stochrsi", "momentum", "oscillator", "stochastic"},
      .desc = "Stochastic RSI. Applies stochastic formula to RSI values, "
              "creating a more sensitive oscillator.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::rsi};

  indicatorMetaData["tema"] = IndicatorMetaData{
      .tags = {"overlay", "tema", "moving-average", "triple-exponential"},
      .desc = "Triple Exponential Moving Average. Moving average designed to "
              "smooth price fluctuations and reduce lag.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["tr"] = IndicatorMetaData{
      .tags = {"indicator", "tr", "volatility", "true-range"},
      .desc = "True Range. Measures market volatility by comparing current "
              "price range to previous close.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["trima"] = IndicatorMetaData{
      .tags = {"overlay", "trima", "moving-average", "triangular"},
      .desc = "Triangular Moving Average. Weighted moving average that places "
              "more weight on middle portion of calculation period.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["trix"] = IndicatorMetaData{
      .tags = {"indicator", "trix", "momentum", "oscillator"},
      .desc = "Trix. Triple exponentially smoothed moving average oscillator, "
              "showing percentage rate of change.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["tsf"] = IndicatorMetaData{
      .tags = {"overlay", "tsf", "trend", "time-series-forecast"},
      .desc = "Time Series Forecast. Linear regression projection that extends "
              "the regression line to predict future values.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["typprice"] = IndicatorMetaData{
      .tags = {"overlay", "typprice", "price", "average", "typical"},
      .desc = "Typical Price. Average of high, low, and close prices for each "
              "period, representing a balanced price.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["ultosc"] = IndicatorMetaData{
      .tags = {"indicator", "ultosc", "oscillator", "ultimate-oscillator"},
      .desc = "Ultimate Oscillator. Multi-timeframe momentum oscillator that "
              "uses weighted average of three oscillators.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["vhf"] = IndicatorMetaData{
      .tags = {"indicator", "vhf", "trend", "vertical-horizontal-filter",
               "volatility"},
      .desc = "Vertical Horizontal Filter. Identifies trending and ranging "
              "markets by measuring price direction versus volatility.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["vidya"] = IndicatorMetaData{
      .tags = {"overlay", "vidya", "moving-average", "variable-index"},
      .desc = "Variable Index Dynamic Average. Adapts to volatility by "
              "modifying the smoothing constant used in calculations.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["volatility"] = IndicatorMetaData{
      .tags = {"indicator", "volatility", "risk", "annualized"},
      .desc = "Annualized Historical Volatility. Measures price dispersion "
              "around the mean, expressed as an annualized percentage.",
      .category = epoch_core::TransformCategory::Volatility,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["vosc"] = IndicatorMetaData{
      .tags = {"indicator", "vosc", "volume", "oscillator"},
      .desc = "Volume Oscillator. Shows difference between two volume moving "
              "averages as percentage, indicating volume trends.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["vwma"] = IndicatorMetaData{
      .tags = {"overlay", "vwma", "moving-average", "volume-weighted"},
      .desc =
          "Volume Weighted Moving Average. Moving average that weights price "
          "by volume, giving more importance to high-volume price moves.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["wad"] = IndicatorMetaData{
      .tags = {"indicator", "wad", "volume",
               "williams-accumulation-distribution"},
      .desc = "Williams Accumulation/Distribution. Measures buying/selling "
              "pressure by comparing closing price to midpoint of range.",
      .category = epoch_core::TransformCategory::Volume,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::panel_line};

  indicatorMetaData["wcprice"] = IndicatorMetaData{
      .tags = {"overlay", "wcprice", "price", "weighted-close"},
      .desc = "Weighted Close Price. Average of OHLC prices with extra weight "
              "given to close: (H+L+C+C)/4.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["wilders"] = IndicatorMetaData{
      .tags = {"overlay", "wilders", "moving-average", "smoothing"},
      .desc = "Wilders Smoothing. Specialized moving average using a 1/n "
              "smoothing factor, commonly used in RSI calculations.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["willr"] = IndicatorMetaData{
      .tags = {"indicator", "willr", "momentum", "oscillator", "williams"},
      .desc = "Williams %R. Momentum oscillator that indicates "
              "overbought/oversold conditions relative to high-low range.",
      .category = epoch_core::TransformCategory::Momentum,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::rsi};

  indicatorMetaData["wma"] = IndicatorMetaData{
      .tags = {"overlay", "wma", "moving-average", "weighted"},
      .desc = "Weighted Moving Average. Moving average that assigns more "
              "weight to recent data and less to older data.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  indicatorMetaData["zlema"] = IndicatorMetaData{
      .tags = {"overlay", "zlema", "moving-average", "zero-lag"},
      .desc = "Zero-Lag Exponential Moving Average. EMA variant that removes "
              "lag by using linear extrapolation.",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Standard,
      .plotKind = epoch_core::TransformPlotKind::line};

  return indicatorMetaData;
}

inline MetaDataOption MakeTulipOptions(std::string const &option) {
  static std::unordered_set<std::string> skip{"open", "high", "low", "close",
                                              "volume"};
  MetaDataOption optionMetaData{.id = option,
                                .name = beautify(option),
                                .type = epoch_core::MetaDataOptionType::Decimal,
                                .defaultValue = std::nullopt,
                                .isRequired = true,
                                .selectOption = {}};

  if (option.starts_with("period") || option.ends_with("period")) {
    optionMetaData.type = epoch_core::MetaDataOptionType::Integer;
    optionMetaData.min = 1; // Period must be at least 1
    optionMetaData.max = 10000;
  } else if (option == "stddev") {
    optionMetaData.type = epoch_core::MetaDataOptionType::Integer;
    optionMetaData.min =
        1; // Standard deviation multiplier should be at least 1
    optionMetaData.max = 10;
  }
  return optionMetaData;
};

inline std::vector<IOMetaData> MakeTulipInputs(auto const &inputs) {
    static std::unordered_set<std::string> skip{"open", "high", "low", "close",
                                                "volume"};
    std::vector<IOMetaData> ioMetaDataList;
    bool useSingleWildCard = inputs.size() == 1;
    for (auto const &[i, input] : std::views::enumerate(inputs)) {
        std::string_view inputStr{input};

        IOMetaData ioMetaData;
        ioMetaData.allowMultipleConnections = false;

        if (inputStr == "real") {
            ioMetaData.id = useSingleWildCard ? ARG : std::format("{}{}", ARG, i);
        } else {
            // skip ohlcv inputs
            continue;
        }
        ioMetaDataList.emplace_back(ioMetaData);
    }

    return ioMetaDataList;
}

inline std::vector<IOMetaData> MakeTulipOutputs(auto const &outputs) {
  std::vector<IOMetaData> ioMetaDataList;
  if (outputs.size() == 1) {
    std::string output{outputs[0]};
    ioMetaDataList.emplace_back(
        IOMetaData{.type = (output == "crossany" || output == "crossover")
                               ? epoch_core::IODataType::Boolean
                               : epoch_core::IODataType::Decimal,
                   .id = "result",
                   .name = "",
                    .allowMultipleConnections = true});
  } else {
    for (auto const &output_view : outputs) {
      std::string output{output_view};
      ioMetaDataList.emplace_back(
          IOMetaData{.type = epoch_core::IODataType::Decimal,
                     .id = output,
                     .name = beautify(output),
                     .allowMultipleConnections = true});
    }
  }

  return ioMetaDataList;
}

std::vector<TransformsMetaData> MakeTulipIndicators() {
  static const std::unordered_map<std::string, IndicatorMetaData>
      indicatorMetaData = MakeTulipIndicatorMetaData();

  std::vector<TransformsMetaData> allIndicators(TI_INDICATOR_COUNT);
  static std::unordered_set<std::string> dataSources{"open", "high", "low",
                                                     "close", "volume"};
  std::ranges::transform(
      std::span{ti_indicators, ti_indicators + TI_INDICATOR_COUNT},
      allIndicators.begin(), [&](const ti_indicator_info &tiIndicatorInfo) {
        const std::span optionSpan{tiIndicatorInfo.option_names,
                                   tiIndicatorInfo.option_names +
                                       tiIndicatorInfo.options};
        const std::span inputSpan{tiIndicatorInfo.input_names,
                                  tiIndicatorInfo.input_names +
                                      tiIndicatorInfo.inputs};
        const std::span outputSpan{tiIndicatorInfo.output_names,
                                   tiIndicatorInfo.output_names +
                                       tiIndicatorInfo.outputs};

        auto metadata = epoch_core::lookupDefault(
            indicatorMetaData, tiIndicatorInfo.name, IndicatorMetaData{});

        std::vector<std::string> requiredDataSources;
        for (auto const &inputPtr : inputSpan) {
          std::string input{inputPtr};
          if (dataSources.contains(input)) {
            requiredDataSources.emplace_back(1, input.front());
          }
        }

        return TransformsMetaData{
            .id = tiIndicatorInfo.name,
            .category = metadata.category,
            .renderKind = metadata.renderKind,
            .plotKind = metadata.plotKind,
            .name = tiIndicatorInfo.full_name,
            .options = epoch_core::ranges::to<std::vector>(
                optionSpan | std::views::transform(MakeTulipOptions)),
            .isCrossSectional = false,
            .desc = metadata.desc,
            .inputs = MakeTulipInputs(inputSpan),
            .outputs = MakeTulipOutputs(outputSpan),
            .tags = metadata.tags,
            .requiresTimeFrame = requiredDataSources.size() > 0,
            .requiredDataSources = requiredDataSources};
      });
  return allIndicators;
}
} // namespace epoch_metadata::transforms