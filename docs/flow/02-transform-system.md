# Transform System

Understanding how transforms work, timeframes, sessions, and cross-sectional analysis.

---

## Table of Contents

1. [Transform Anatomy](#transform-anatomy)
2. [Calling Transforms](#calling-transforms)
3. [Transform Outputs](#transform-outputs)
4. [Chaining Transforms](#chaining-transforms)
5. [TimeFrame System](#timeframe-system)
6. [Session System](#session-system)
7. [Cross-Sectional Analysis](#cross-sectional-analysis)

---

## Transform Anatomy

Transforms are the core building blocks of EpochFlow. Each transform has:

- **Name**: Identifier (e.g., `ema`, `rsi`, `crossover`)
- **Options**: Configuration parameters (e.g., `period=20`)
- **Inputs**: Data to process (e.g., `src.c`)
- **Outputs**: Results (e.g., `result`, `upper`, `lower`)

**Structure:**
```python
result = transform_name(option1=value, option2=value)(input1, input2)
```

---

## Calling Transforms

### Standard Syntax

```python
# Single option, single input
ema_20 = ema(period=20)(src.c)

# Multiple options, single input
bb = bbands(period=20, stddev=2)(src.c)

# No options, multiple inputs
cross_up = crossover()(fast, slow)

# No inputs (data source)
src = market_data_source()
```

### Shorthand (No Options)

When a transform has no options, you can omit the first parentheses:

```python
# Full form
cross = crossover()(fast, slow)

# Shorthand (same thing)
cross = crossover(fast, slow)

# Both are valid
```

### Named Inputs

Some transforms use named inputs for clarity:

```python
# Correlation requires x and y
corr = rolling_corr(window=20)(x=series1, y=series2)

# Regression
slope = regr_slope(period=20)(x=time_series, y=price)
```

---

## Transform Outputs

### Single Output

Most transforms return a single value:

```python
# Direct usage
ema_val = ema(period=20)(src.c)
signal = ema_val > 100

# Can also access .result explicitly (optional)
signal = ema_val.result > 100
```

### Multiple Outputs

Some transforms return multiple values:

**Tuple Unpacking (Recommended):**
```python
# Bollinger Bands: lower, middle, upper
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# MACD: macd_line, signal_line
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)

# Discard unwanted with underscore
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

**Attribute Access:**
```python
# Store result, then access handles
bb = bbands(period=20, stddev=2)(src.c)
upper = bb.bbands_upper
middle = bb.bbands_middle
lower = bb.bbands_lower
```

**⚠️ You cannot call attributes as functions:**
```python
# WRONG - this will fail
bb_upper = bbands(period=20, stddev=2).bbands_upper(src.c)  # ERROR

# CORRECT
bb = bbands(period=20, stddev=2)(src.c)
bb_upper = bb.bbands_upper
```

### Common Multi-Output Transforms

| Transform | Outputs | Description |
|-----------|---------|-------------|
| `bbands` | lower, middle, upper | Bollinger Bands |
| `macd` | macd, signal | MACD and signal line |
| `stoch` | stoch_k, stoch_d | Stochastic %K and %D |
| `di` | plus_di, minus_di | Directional Indicators |
| `aroon` | aroon_up, aroon_down | Aroon up and down |
| `ichimoku` | tenkan, kijun, senkou_a, senkou_b, chikou | Ichimoku Cloud (5 outputs) |
| `sessions` | active, high, low, opened, closed | Session detection |
| `session_gap` | gap_filled, gap_retrace, gap_size, psc, psc_timestamp | Gap analysis |

---

## Chaining Transforms

Transforms can be chained together:

```python
# EMA of close
ema_20 = ema(period=20)(src.c)

# EMA of EMA (double smoothing)
ema_of_ema = ema(period=10)(ema_20)

# Inline chaining
smoothed = ema(period=10)(ema(period=20)(src.c))

# Use in expressions
signal = src.c > ema(period=20)(src.c)
```

---

## TimeFrame System

The `timeframe` parameter allows transforms to operate on data at a different resolution than the strategy's base timeframe.

### How It Works

The framework automatically resamples/aggregates data to the specified timeframe BEFORE the transform executes.

```python
# Strategy runs on 15-minute bars
src = market_data_source()

# Calculate daily EMA (resampled to 1 day)
daily_ema = ema(period=50, timeframe="1D")(src.c)

# Intraday price vs daily trend
is_above_daily = src.c > daily_ema
```

### Standard TimeFrames

#### Intraday (Minute/Hour)
```python
"1Min"    # 1 minute
"5Min"    # 5 minutes
"15Min"   # 15 minutes
"30Min"   # 30 minutes
"1H"      # 1 hour
"4H"      # 4 hours
```

#### Daily and Above
```python
"1D"      # 1 day
"1W"      # 1 week
"1M"      # 1 month
```

**⚠️ Use short forms only:**
- ✓ `"1D"`, `"1H"`, `"1W"`
- ✗ `"1Day"`, `"1Hour"`, `"1Week"` (not supported)

### Advanced TimeFrames

#### Month Anchoring
```python
"1MS"     # Month start
"1ME"     # Month end

# Example: Month-end closing price
month_end = market_data_source(timeframe="1ME")().c
```

#### Week Anchoring
```python
"1W-MON"  # Week starting Monday (default)
"1W-FRI"  # Week starting Friday
"1W-SUN"  # Week starting Sunday

# Example: Forex weekly (Friday start)
weekly_fri = ema(period=10, timeframe="1W-FRI")(src.c)
```

#### Business Days
```python
"1B"      # 1 business day
"5B"      # 5 business days (1 week)
"21B"     # ~1 month of business days
```

### Multi-TimeFrame Strategy Pattern

**Pattern:** Higher timeframe for trend, lower timeframe for entry

```python
src = market_data_source()

# Daily trend filter (strategy runs on 15Min)
daily_trend = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_trend

# Intraday entry signals
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
entry = crossover(fast, slow)

# Only enter with daily trend
buy = entry and uptrend
trade_signal_executor()(enter_long=buy)
```

---

## Session System

### Two Session Features

| Feature | Purpose | Usage |
|---------|---------|-------|
| **`session` parameter** | Filter data to specific session | `ema(period=20, session="London")(src.c)` |
| **`sessions()` transform** | Detect session boundaries | `london = sessions(session_type="London")()` |

### Session Parameter (Data Filtering)

**Purpose:** Execute transform on ONLY the bars within a specific trading session.

```python
# EMA calculated ONLY on London session bars
london_ema = ema(period=20, session="London")(src.c)

# RSI calculated ONLY on New York session bars
ny_rsi = rsi(period=14, session="NewYork")(src.c)

# Gap detection for Asian session only
asian_gaps = session_gap(fill_percent=100, session="AsianKillZone")()
```

**Use Cases:**
- Session-specific indicators (London volatility, NY volume)
- Isolate trading to high-liquidity periods
- Compare behavior across sessions

### sessions() Transform (Session Detection)

**Purpose:** Mark session boundaries and track session high/low.

**Syntax:**
```python
session = sessions(session_type="session_name")()
```

**Outputs:**
- `active` - Boolean: True during session
- `high` - Decimal: Session high price
- `low` - Decimal: Session low price
- `opened` - Boolean: True on session open bar
- `closed` - Boolean: True on session close bar

**Example:**
```python
src = market_data_source()
london = sessions(session_type="London")()

# Only trade during London session
in_session = london.active
entry = bullish_condition and in_session

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Exit at session close
exit_signal = london.closed
```

### Available Sessions

#### Geographic Sessions (Major Trading Centers)
```python
"Sydney"    # Sydney, Australia (10 PM - 7 AM GMT)
"Tokyo"     # Tokyo, Japan (12 AM - 9 AM GMT)
"London"    # London, UK (8 AM - 5 PM GMT)
"NewYork"   # New York, USA (1 PM - 10 PM GMT)
```

#### Kill Zones (ICT - High Liquidity Periods)
```python
"AsianKillZone"         # 19:00-23:00 ET
"LondonOpenKillZone"    # 02:00-05:00 ET
"NewYorkKillZone"       # 07:00-10:00 ET
"LondonCloseKillZone"   # 10:00-12:00 ET
```

### Session + TimeFrame Combined

```python
# Daily bars, but only from New York session data
daily_ny = market_data_source(
    timeframe="1D",
    session="NewYork"
)().c

# Hourly EMA, London session only
hourly_london = ema(
    period=20,
    timeframe="1H",
    session="London"
)(src.c)
```

**Execution Order:**
1. Filter by `session` (if specified)
2. Resample to `timeframe` (if specified)
3. Execute transform

---

## Cross-Sectional Analysis

Cross-sectional transforms operate on **multiple assets simultaneously** to generate rankings, factors, or universe filters.

### Requirements

1. **Multiple assets**: Minimum 10+ recommended
2. **Same time period**: All assets evaluated at the same timestamp
3. **Universe specification**: External configuration (not in script)

### Cross-Sectional Transforms

```python
# Momentum ranking across universe
returns = roc(period=20)(close)
momentum = cs_momentum()(returns)

# Select top K assets
top10 = top_k(k=10)(momentum)
top20_percent = top_k_percent(k=20)(momentum)

# Select bottom K assets
bottom10 = bottom_k(k=10)(momentum)

# Trade top performers
trade_signal_executor()(enter_long=top10)
```

### Available Cross-Sectional Transforms

| Transform | Description |
|-----------|-------------|
| `cs_momentum` | Rank assets by momentum (percentile) |
| `top_k` | Select top K assets |
| `bottom_k` | Select bottom K assets |
| `top_k_percent` | Select top K% of assets |
| `bottom_k_percent` | Select bottom K% of assets |

### Cross-Sectional Rules

1. **Cannot mix single-asset and multi-asset strategies**
2. **Requires external universe definition** (not in script)
3. **All transforms must support cross-sectional mode**
4. **Rankings are relative within the universe**

### Example: Factor Strategy

```python
# Calculate multiple factors
momentum = roc(period=20)(close)
value = earnings_yield()  # From fundamental data
quality = roe()

# Combine factors (equal-weight)
combined = (momentum + value + quality) / 3

# Select top 20 stocks
top_stocks = top_k(k=20)(combined)

trade_signal_executor()(enter_long=top_stocks)
```

---

## Transform Categories

Transforms are organized into categories:

| Category | Description | Examples |
|----------|-------------|----------|
| **DataSource** | Market data access | market_data_source, balance_sheet |
| **Trend** | Trend indicators | sma, ema, adx, aroon |
| **Momentum** | Oscillators | rsi, macd, stoch, cci |
| **Volatility** | Volatility measures | atr, bbands, volatility estimators |
| **Volume** | Volume-based | obv, ad, mfi, vwap |
| **Statistical** | Statistical analysis | rolling_corr, zscore, regression |
| **PriceAction** | Chart patterns & SMC | order_blocks, fair_value_gap, swing_highs_lows |
| **Math** | Comparisons & operators | gt, lt, crossover, add, sub |
| **ControlFlow** | Conditional logic | conditional_select, first_non_null |
| **Reporter** | Visualization | gap_report, table_report, charts |
| **Executor** | Trade execution | trade_signal_executor |

---

## Transform Metadata

Every transform has rich metadata defining its behavior:

```yaml
name: "Display Name"
desc: "Technical description"
usageContext: "When to use this"
category: Momentum | Trend | Volatility | ...
plotKind: line | panel_line | rsi | macd | ...

options:
  - id: period
    type: Integer
    default: 14
    min: 1
    max: 500
    desc: "Lookback period"

inputs:
  - { type: Decimal, id: close, name: "Close Price" }

outputs:
  - { type: Decimal, id: result, name: "RSI Value" }
```

---

## Special Transform Types

### Sinks (No Outputs)

Sinks consume data and produce side effects (trades, reports):

```python
# Trade execution
trade_signal_executor()(enter_long=buy)

# Report generation
gap_report(...)(inputs)

# UI selector
card_selector_filter(...)(inputs)
```

### Sources (No Inputs)

Sources produce data without consuming other transforms:

```python
# Market data
src = market_data_source()

# Session gap detection (uses internal data)
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Economic indicator
gdp = economic_indicator(indicator="GDP")()
```

### Hybrid (Inputs + Internal Data)

Some transforms use both:

```python
# Sessions transform uses internal timestamp data + OHLC
london = sessions(session_type="London")()
# Implicitly uses: open, high, low, close, timestamp
```

---

## Best Practices

### 1. Use Descriptive Variable Names
```python
# Good
fast_ema = ema(period=12)(src.c)
slow_ema = ema(period=26)(src.c)

# Avoid
e1 = ema(period=12)(src.c)
e2 = ema(period=26)(src.c)
```

### 2. Group Related Calculations
```python
# Indicators
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
rsi_val = rsi(period=14)(src.c)

# Conditions
trend_up = fast > slow
oversold = rsi_val < 30

# Signal
entry = trend_up and oversold
```

### 3. Comment Complex Logic
```python
# Daily trend filter (50-period EMA on 1D timeframe)
daily_trend = ema(period=50, timeframe="1D")(src.c)

# Only trade during NY session kill zone (07:00-10:00 ET)
ny_kz = sessions(session_type="NewYorkKillZone")()
```

### 4. Use Multi-Timeframe for Context
```python
# Higher timeframe = trend
# Lower timeframe = entry timing
daily_ema = ema(period=50, timeframe="1D")(src.c)
intraday_signal = crossover(fast, slow)
entry = intraday_signal and (src.c > daily_ema)
```

### 5. Leverage Session Filtering
```python
# Calculate volatility only during main trading hours
london_vol = atr(period=14, session="London")(src.c)
ny_vol = atr(period=14, session="NewYork")(src.c)

# Use appropriate volatility based on current session
```

---

## Summary

### Key Concepts

1. **Transforms**: Building blocks with options, inputs, outputs
2. **TimeFrames**: Multi-resolution analysis (`timeframe` parameter)
3. **Sessions**: Intraday filtering and detection
4. **Cross-Sectional**: Multi-asset ranking and selection
5. **Chaining**: Compose transforms together
6. **Categories**: Organized by trading purpose

### Common Patterns

```python
# Single output
result = transform(option=val)(input)

# Multiple outputs
a, b, c = transform(...)(input)

# Multi-timeframe
higher_tf = transform(period=50, timeframe="1D")(input)

# Session filtering
session_calc = transform(period=20, session="London")(input)

# Session detection
session = sessions(session_type="London")()

# Cross-sectional
top = top_k(k=10)(factor)
```

---

**Next:** [Core Transforms →](03-core-transforms.md)
