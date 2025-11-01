# Appendix: Full Transform Catalog

Complete reference of all 300+ available transforms.

---

## Overview

This appendix lists ALL transforms available in EpochFlow. For detailed documentation of the most commonly used transforms, see [Core Transforms](03-core-transforms.md).

**Organization:** Transforms are grouped by category and listed alphabetically within each category.

---

## Data Sources (9 transforms)

| Transform | Description |
|-----------|-------------|
| `market_data_source` | OHLCV market data (required for all strategies) |
| `balance_sheet` | Fundamental balance sheet data (Polygon) |
| `income_statement` | Income statement fundamentals |
| `cash_flow` | Cash flow statement data |
| `financial_ratios` | Calculated financial ratios (P/E, ROE, etc.) |
| `quotes` | Tick-level quote data (bid/ask) |
| `trades` | Tick-level trade data |
| `aggregates` | Custom aggregated OHLCV |
| `economic_indicator` | FRED economic data (GDP, CPI, unemployment) |

---

## Trend Indicators (25+ transforms)

| Transform | Description |
|-----------|-------------|
| `sma` | Simple Moving Average |
| `ema` | Exponential Moving Average |
| `wma` | Weighted Moving Average |
| `dema` | Double Exponential MA |
| `tema` | Triple Exponential MA |
| `hma` | Hull Moving Average (low-lag) |
| `trima` | Triangular MA |
| `kama` | Kaufman Adaptive MA (volatility-adjusted) |
| `vidya` | Variable Index Dynamic Average |
| `zlema` | Zero-Lag EMA |
| `wilders` | Wilders Smoothing |
| `vwma` | Volume Weighted MA |
| `adx` | Average Directional Index (trend strength) |
| `adxr` | ADX Rating |
| `aroon` | Aroon Indicator (time since highs/lows) |
| `aroonosc` | Aroon Oscillator |
| `di` | Directional Indicator (+DI, -DI) |
| `dm` | Directional Movement |
| `dx` | Directional Movement Index |
| `vhf` | Vertical Horizontal Filter (trend vs range) |
| `psar` | Parabolic SAR |
| `linreg` | Linear Regression |
| `linregintercept` | Linear Regression Intercept |
| `linregslope` | Linear Regression Slope |
| `tsf` | Time Series Forecast |

---

## Momentum Oscillators (30+ transforms)

| Transform | Description |
|-----------|-------------|
| `rsi` | Relative Strength Index (0-100 scale) |
| `stoch` | Stochastic Oscillator (%K and %D) |
| `stochrsi` | Stochastic RSI (more sensitive) |
| `macd` | MACD (trend + momentum) |
| `ppo` | Percentage Price Oscillator |
| `apo` | Absolute Price Oscillator |
| `cci` | Commodity Channel Index (unbounded) |
| `cmo` | Chande Momentum Oscillator |
| `willr` | Williams %R (-100 to 0 scale) |
| `ao` | Awesome Oscillator |
| `bop` | Balance of Power |
| `mom` | Momentum |
| `roc` | Rate of Change (percentage) |
| `rocr` | Rate of Change Ratio |
| `trix` | Triple Exponential Oscillator |
| `ultosc` | Ultimate Oscillator |
| `fisher` | Fisher Transform |
| `fosc` | Forecast Oscillator |
| `dpo` | Detrended Price Oscillator |
| `qstick` | Qstick |
| `msw` | Mesa Sine Wave |
| `mfi` | Money Flow Index (volume-weighted RSI) |

---

## Volatility Indicators (15+ transforms)

| Transform | Description |
|-----------|-------------|
| `atr` | Average True Range |
| `natr` | Normalized ATR (percentage) |
| `tr` | True Range |
| `bbands` | Bollinger Bands (3 outputs) |
| `bband_percent` | Bollinger %B (position within bands) |
| `bband_width` | Bollinger Band Width (squeeze detection) |
| `mass` | Mass Index |
| `cvi` | Chaikins Volatility |
| `volatility` | Historical Volatility (annualized) |
| `ulcer_index` | Ulcer Index (downside volatility) |
| `acceleration_bands` | Acceleration Bands |
| `garman_klass` | Garman-Klass volatility estimator |
| `hodges_tompkins` | Hodges-Tompkins volatility |
| `keltner_channels` | Keltner Channels |
| `parkinson` | Parkinson volatility |
| `yang_zhang` | Yang-Zhang volatility |

---

## Volume Indicators (15+ transforms)

| Transform | Description |
|-----------|-------------|
| `obv` | On Balance Volume |
| `ad` | Accumulation/Distribution Line |
| `adosc` | A/D Oscillator |
| `mfi` | Money Flow Index (volume-weighted RSI) |
| `emv` | Ease of Movement |
| `kvo` | Klinger Volume Oscillator |
| `vosc` | Volume Oscillator |
| `nvi` | Negative Volume Index |
| `pvi` | Positive Volume Index |
| `wad` | Williams Accumulation/Distribution |
| `marketfi` | Market Facilitation Index |

---

## Price Action (10 transforms)

| Transform | Description |
|-----------|-------------|
| `avgprice` | Average Price (OHLC/4) |
| `medprice` | Median Price ((H+L)/2) |
| `typprice` | Typical Price ((H+L+C)/3) |
| `wcprice` | Weighted Close ((H+L+C+C)/4) |

---

## Smart Money Concepts - SMC (10 transforms)

| Transform | Description |
|-----------|-------------|
| `sessions` | Trading session detection (active, high, low, opened, closed) |
| `session_gap` | Overnight gap detection and analysis |
| `bar_gap` | Intraday bar-to-bar gap detection (halts, liquidity) |
| `order_blocks` | SMC order blocks (institutional supply/demand zones) |
| `fair_value_gap` | SMC Fair Value Gaps (FVG, imbalances) |
| `swing_highs_lows` | Swing point detection (local extrema) |
| `bos_choch` | Break of Structure & Change of Character |
| `liquidity` | Liquidity zones (sweeps, grabs) |
| `previous_high_low` | Previous session/period highs/lows |
| `session_time_window` | Session boundary timing detection |

---

## Chart Formations (8 transforms)

| Transform | Description |
|-----------|-------------|
| `head_and_shoulders` | Bearish H&S reversal pattern |
| `inverse_head_and_shoulders` | Bullish inverted H&S |
| `double_top_bottom` | Double tops and bottoms |
| `consolidation_box` | Horizontal rectangles (Bulkowski) |
| `flag` | Bull and bear flags |
| `triangles` | Ascending, descending, symmetrical |
| `pennant` | Brief consolidation patterns |

---

## Candlestick Patterns (25+ transforms)

**Reversal Patterns:**
- `abandoned_baby_bull` / `abandoned_baby_bear` - Rare reversal with gaps
- `hammer` - Bullish reversal (long lower shadow)
- `shooting_star` - Bearish reversal (long upper shadow)
- `hanging_man` - Bearish warning in uptrend
- `inverted_hammer` - Bullish after downtrend
- `dragonfly_doji` - Bullish doji (T-shape)
- `gravestone_doji` - Bearish doji (inverted T)
- `engulfing_bull` / `engulfing_bear` - Strong reversal
- `morning_star` / `evening_star` - 3-candle reversal
- `morning_doji_star` / `evening_doji_star` - Stronger variant with doji
- `three_white_soldiers` / `three_black_crows` - Strong 3-candle patterns

**Indecision Patterns:**
- `doji` - Indecision candle
- `four_price_doji` - Extreme indecision (all prices equal)
- `long_legged_doji` - High volatility indecision
- `spinning_top` - Small body with wicks

**Momentum Patterns:**
- `big_white_candle` / `big_black_candle` - Strong directional move
- `white_marubozu` / `black_marubozu` - No wicks (full body)
- `marubozu` - Generic marubozu

---

## Statistical Analysis (10+ transforms)

| Transform | Description |
|-----------|-------------|
| `rolling_corr` | Rolling Pearson correlation (pairs trading) |
| `rolling_cov` | Rolling covariance (beta calculation) |
| `ewm_corr` | Exponentially weighted correlation |
| `ewm_cov` | Exponentially weighted covariance |
| `zscore` | Z-score (standard deviations from mean) |
| `stddev` | Standard deviation |
| `stderr` | Standard error |
| `var` | Variance |
| `md` | Mean deviation |
| `regr_slope` | Regression slope |
| `hurst_exponent` | Hurst Exponent (trending vs mean-reverting) |
| `rolling_hurst_exponent` | Rolling Hurst |

---

## Mathematical Functions (20+ transforms)

**Trigonometric:**
- `sin`, `cos`, `tan` - Basic trigonometry
- `asin`, `acos`, `atan`, `atan2` - Inverse trig

**Exponential/Logarithmic:**
- `exp` - Exponential
- `ln` - Natural logarithm
- `log10` - Base-10 logarithm

**Rounding:**
- `ceil` - Round up
- `floor` - Round down
- `round` - Round to nearest
- `trunc` - Truncate

**Other:**
- `abs` - Absolute value
- `sqrt` - Square root
- `todeg` - Radians to degrees
- `torad` - Degrees to radians

---

## Comparison Operators (24 transforms)

**Basic Comparisons:**
- `gt` (>), `gte` (>=), `lt` (<), `lte` (<=), `eq` (==), `neq` (!=)

**Temporal Comparisons (vs previous value):**
- `previous_gt`, `previous_gte`, `previous_lt`, `previous_lte`, `previous_eq`, `previous_neq`

**Highest Value Comparisons:**
- `highest_gt`, `highest_gte`, `highest_lt`, `highest_lte`, `highest_eq`, `highest_neq`

**Lowest Value Comparisons:**
- `lowest_gt`, `lowest_gte`, `lowest_lt`, `lowest_lte`, `lowest_eq`, `lowest_neq`

---

## Logical Operators (5 transforms)

| Transform | Description |
|-----------|-------------|
| `logical_and` | Logical AND |
| `logical_or` | Logical OR |
| `logical_xor` | Logical XOR (exclusive OR) |
| `logical_and_not` | AND with negation |
| `logical_not` | Logical NOT |

---

## Control Flow (8 transforms)

| Transform | Description |
|-----------|-------------|
| `boolean_select` | If/else (two-way conditional) |
| `conditional_select` | SQL-style CASE WHEN (multi-way) |
| `first_non_null` | Coalesce (first non-null value) |
| `select_2` | 2-way switch by index |
| `select_3` | 3-way switch by index |
| `select_4` | 4-way switch by index |
| `select_5` | 5-way switch by index |

---

## Aggregate Operators (9 transforms)

| Transform | Description |
|-----------|-------------|
| `agg_sum` | Sum of inputs |
| `agg_mean` | Average of inputs |
| `agg_min` | Minimum value |
| `agg_max` | Maximum value |
| `agg_all_of` | All inputs true (logical AND) |
| `agg_any_of` | At least one input true (logical OR) |
| `agg_none_of` | No inputs true |
| `agg_all_equal` | All inputs equal |
| `agg_all_unique` | All inputs unique |

---

## Crossover Detection (3 transforms)

| Transform | Description |
|-----------|-------------|
| `crossover` | First crosses above second |
| `crossunder` | First crosses below second |
| `crossany` | First crosses second (any direction) |

---

## Cross-Sectional (Multi-Asset) (5 transforms)

| Transform | Description |
|-----------|-------------|
| `cs_momentum` | Cross-sectional momentum ranking |
| `top_k` | Select top K assets |
| `bottom_k` | Select bottom K assets |
| `top_k_percent` | Select top K% of assets |
| `bottom_k_percent` | Select bottom K% of assets |

---

## Calendar Effects (6 transforms)

| Transform | Description |
|-----------|-------------|
| `turn_of_month` | Turn-of-month effect (equity inflows) |
| `day_of_week` | Day-of-week effects (Monday, Friday) |
| `month_of_year` | Seasonal patterns (January, Sell in May) |
| `quarter` | Quarterly patterns (Q1-Q4) |
| `holiday` | Holiday effects (pre/post holiday) |
| `week_of_month` | Week-of-month patterns |

---

## String Operations (8 transforms)

| Transform | Description |
|-----------|-------------|
| `string_case` | Convert case (upper, lower, capitalize, title) |
| `string_trim` | Remove whitespace |
| `string_pad` | Pad strings (left, right, center) |
| `string_contains` | Check containment (contains, starts_with, ends_with) |
| `string_check` | Type checking (is_alpha, is_digit, is_numeric) |
| `string_replace` | Replace substrings |
| `string_length` | Get string length |
| `string_reverse` | Reverse string |

---

## Reporting & Visualization (14 transforms)

**Card Reports:**
- `numeric_card_report` - Display numeric metrics
- `boolean_card_report` - Display boolean metrics
- `any_card_report` - Display any type metrics
- `index_card_report` - Display index/categorical metrics
- `quantile_card_report` - Display quantile statistics

**Chart Reports:**
- `bar_chart_report` - Bar charts
- `pie_chart_report` - Pie charts
- `nested_pie_chart_report` - Hierarchical pie charts
- `histogram_chart_report` - Histograms
- `lines_chart_report` - Line charts

**Table Reports:**
- `table_report` - Tabular data with SQL queries

**Specialized Reports:**
- `gap_report` - Comprehensive gap analysis (cards + charts + tables)

**Selectors:**
- `card_selector_filter` - Interactive UI card selector

---

## Execution (2 transforms)

| Transform | Description |
|-----------|-------------|
| `trade_signal_executor` | Execute trades from boolean signals (REQUIRED) |
| `trade_executor_adapter` | Adapter for trade execution |

---

## Scalars & Constants (15+ transforms)

| Transform | Description |
|-----------|-------------|
| `number` | User-defined numeric constant |
| `text` | User-defined string constant |
| `bool_true` | Boolean True |
| `bool_false` | Boolean False |
| `zero`, `one`, `negative_one` | Common values |
| `pi`, `e`, `phi` | Mathematical constants |
| `sqrt2`, `sqrt3`, `sqrt5` | Square roots |
| `ln2`, `ln10` | Natural log constants |
| `log2e`, `log10e` | Log conversion factors |
| `null` | Null/undefined value |

---

## Utility Transforms (10+ transforms)

| Transform | Description |
|-----------|-------------|
| `lag` | Shift series backward by N periods |
| `forward_returns` | Calculate future returns (research only) |
| `decay` | Linear decay |
| `edecay` | Exponential decay |
| `cum_prod` | Cumulative product (compounding returns) |

---

## Advanced Indicators (15+ transforms)

| Transform | Description |
|-----------|-------------|
| `chande_kroll_stop` | Chande Kroll Stop |
| `donchian_channel` | Donchian Channels |
| `elders_thermometer` | Elder's Thermometer |
| `ichimoku` | Ichimoku Cloud (5 outputs) |
| `pivot_point_sr` | Pivot Points Support/Resistance |
| `price_distance` | Price Distance from MA |
| `psl` | PSL Indicator |
| `qqe` | Quantitative Qualitative Estimation |
| `vortex` | Vortex Indicator |
| `retracements` | Fibonacci Retracements |
| `hmm` | Hidden Markov Model |

---

## Summary Statistics

**Total Transforms: 300+**

**By Category:**
- Data Sources: 9
- Trend: 25+
- Momentum: 30+
- Volatility: 15+
- Volume: 15+
- SMC/Price Action: 10
- Chart Formations: 8
- Candlesticks: 25+
- Statistical: 10+
- Mathematical: 20+
- Comparisons: 24
- Logical: 5
- Control Flow: 8
- Aggregates: 9
- Cross-Sectional: 5
- Calendar: 6
- Strings: 8
- Reporting: 14
- Execution: 2
- Scalars: 15+
- Utility: 10+
- Advanced: 15+

---

## Transform Catalog Location

The complete transform metadata (options, inputs, outputs, descriptions) is available in:
- **File:** `/home/adesola/EpochLab/EpochAI/common/data_utils/catalogs/transforms_catalog.xml`
- **Format:** XML with full specifications for each transform

---

**For detailed documentation of the 80 most commonly used transforms, see [Core Transforms](03-core-transforms.md).**
