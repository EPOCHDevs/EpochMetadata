# EpochFlow DSL Reference

**Complete Language Specification for Trading Strategy Development**

Version: 2.0
Last Updated: 2025-10-29

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Critical Limitations](#2-critical-limitations)
3. [Language Syntax](#3-language-syntax)
4. [Expressions and Operators](#4-expressions-and-operators)
5. [Transform System](#5-transform-system)
    - 5.4 Mathematical Functions
6. [TimeFrame System](#6-timeframe-system)
7. [Session System](#7-session-system)
8. [Type System](#8-type-system)
9. [Core Transforms](#9-core-transforms)
10. [Complete Examples](#10-complete-examples)
11. [Error Reference](#11-error-reference)
12. [AI Agent Quick Reference](#12-ai-agent-quick-reference)

---

## 1. Introduction

### 1.1 What is EpochFlow?

EpochFlow is a Python-like domain-specific language for algorithmic trading strategies and market analysis. It provides:

- **Market data access**: OHLCV data, economic indicators
- **Technical indicators**: 200+ built-in transforms (moving averages, oscillators, etc.)
- **Mathematical functions**: Standard Python math functions (sin, cos, log, sqrt, etc.)
- **Trading signals**: Buy/sell condition generation for backtesting
- **Research tools**: Gap analysis, session detection, custom reports
- **Multi-timeframe**: Combine daily, hourly, and intraday data
- **Session filtering**: Trade specific market sessions (London, New York, etc.)

### 1.2 Design Philosophy

EpochFlow is **declarative**, not imperative. You describe *what* you want, not *how* to compute it:

- No loops, no if statements, no functions
- Variables are immutable (single assignment)
- Transforms are composable building blocks
- Type-safe with automatic type inference

**Trading Strategy Example:**
```python
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
buy = crossover(fast, slow)
sell = crossover(slow, fast)
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

**Research Example (Overnight Gap Analysis):**
```python
# Detect overnight gaps and analyze fill behavior
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled, gaps.gap_retrace, gaps.gap_size,
    gaps.psc, gaps.psc_timestamp
)
```

**Research Example (Intraday Gap Analysis - Trading Halts & Liquidity Gaps):**
```python
# Detect intraday bar-to-bar gaps (trading halts, liquidity events)
# Use min_gap_size to filter noise: 2.0% for stocks, 0.04% for FX pips
intraday_gaps = bar_gap(fill_percent=100, min_gap_size=2.0, timeframe="1Min")()
gap_report(fill_time_pivot_hour=13, histogram_bins=10)(
    intraday_gaps.gap_filled, intraday_gaps.gap_retrace, intraday_gaps.gap_size,
    intraday_gaps.psc, intraday_gaps.psc_timestamp
)
```

**Research Example (FX Pip Gap Analysis):**
```python
# Detect 4-pip gaps in forex markets
fx_gaps = bar_gap(fill_percent=100, min_gap_size=0.04, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=8)(
    fx_gaps.gap_filled, fx_gaps.gap_retrace, fx_gaps.gap_size,
    fx_gaps.psc, fx_gaps.psc_timestamp
)
```

---

## 2. Critical Limitations

### 2.1 What is NOT Supported

EpochFlow is **not** full Python. These constructs will cause compilation errors:

#### Control Flow (NOT ALLOWED)
```python
# ❌ ERROR: If statements not supported
if price > 100:
    signal = True

# ✅ SOLUTION: Use ternary expressions
signal = True if price > 100 else False
```

#### Loops (NOT ALLOWED)
```python
# ❌ ERROR: Loops not supported
for i in range(10):
    calculate_something()

# ✅ SOLUTION: Use transforms (vectorized operations)
```

#### Functions/Classes (NOT ALLOWED)
```python
# ❌ ERROR: Cannot define functions
def my_indicator(period):
    return sma(period=period)(src.c)

# ✅ SOLUTION: Use direct assignments
short_ma = sma(period=20)(src.c)
long_ma = sma(period=50)(src.c)
```

#### Reassignment (NOT ALLOWED)
```python
# ❌ ERROR: Variables cannot be reassigned
price = src.c
price = src.o  # ERROR: Variable 'price' already bound

# ✅ SOLUTION: Use different names
close_price = src.c
open_price = src.o
```

#### Chained Comparisons (NOT ALLOWED)
```python
# ❌ ERROR: Chained comparisons not supported
valid = 30 < rsi_val < 70

# ✅ SOLUTION: Break into separate comparisons
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70
```

#### Unary Plus (Supported but No-Op)
```python
# ✅ Unary + is supported but does nothing (returns value unchanged)
positive = +value  # Same as: positive = value

# Note: The unary + operator is accepted by the compiler
# but has no effect - it simply returns the operand as-is
```

### 2.2 Lag Operator Constraints

#### Zero Index (NOT ALLOWED)
```python
# ❌ ERROR: Lag index cannot be 0
current = src.c[0]  # ERROR: invalid_lag_zero

# ✅ SOLUTION: Omit the index
current = src.c
```

#### Float Index (NOT ALLOWED)
```python
# ❌ ERROR: Lag index must be integer
lag = src.c[1.5]  # ERROR: invalid_lag_float

# ✅ SOLUTION: Use integer
lag = src.c[2]
```

### 2.3 Multi-Output Transform Rules

#### Expression Context (NOT ALLOWED)
```python
# ❌ ERROR: Multi-output transform in expression
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # ERROR: ambiguous_multi_output

# ✅ SOLUTION: Access specific output handle
bb = bbands(period=20, stddev=2)(src.c)
signal = bb.bbands_upper > 100
```

#### Tuple Unpacking Count Mismatch (NOT ALLOWED)
```python
# ❌ ERROR: Output count must match
lower, upper = bbands(period=20, stddev=2)(src.c)  # ERROR: 3 outputs, 2 variables

# ✅ SOLUTION: Match count or discard with _
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
lower, _, upper = bbands(period=20, stddev=2)(src.c)  # Discard middle
```

### 2.4 Required Components

Every **trading strategy** must have exactly one `trade_signal_executor`:

```python
# ❌ ERROR: No executor
src = market_data_source()
signal = src.c > src.c[1]
# Missing: trade_signal_executor()

# ✅ SOLUTION: Add executor
src = market_data_source()
signal = src.c > src.c[1]
trade_signal_executor()(enter_long=signal)
```

### 2.5 Constant Folding Limitations

Constants can ONLY be used in **subscript indices** (lag operations), not as general variables:

```python
# ✅ ALLOWED: Constant in subscript
lookback = 20
price_lag = src.c[lookback]  # Constant folded at compile-time

# ❌ NOT ALLOWED: Constant in transform option
period = 20
ma = sma(period=period)(src.c)  # ERROR: period must be literal

# ✅ SOLUTION: Use literal
ma = sma(period=20)(src.c)

# ❌ NOT ALLOWED: Constant in expression
multiplier = 1.5
volume_threshold = avg_vol * multiplier  # ERROR: multiplier not folded

# ✅ SOLUTION: Use literal or direct calculation
volume_threshold = avg_vol * 1.5
```

---

## 3. Language Syntax

### 3.1 Statements

EpochFlow has two statement types:

#### Assignment Statement
```python
variable = expression
```

**Rules:**
- Variables must start with letter or underscore: `ema_20`, `_temp`, `Signal`
- Can contain letters, numbers, underscores: `fast_ema_12`, `rsi_14`
- Case-sensitive: `Price` ≠ `price`
- **Single assignment only** - no reassignment

#### Expression Statement
```python
transform_call()(inputs)
```

**Rules:**
- Only allowed for "sink" transforms (no outputs): `trade_signal_executor`, `gap_report`, `card_selector_filter`
- Must be a function call, not a bare variable

**Example:**
```python
# Assignments
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
buy_signal = crossover(fast, slow)

# Expression statement (executor has no outputs)
trade_signal_executor()(enter_long=buy_signal)
```

### 3.2 Tuple Unpacking

Multi-output transforms require tuple unpacking:

```python
# Multi-output: bbands returns (lower, middle, upper)
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# MACD returns (macd, signal)
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)

# Discard unwanted outputs with _
lower, _, upper = bbands(period=20, stddev=2)(src.c)  # Discard middle
_, signal_line = macd(fast=12, slow=26, signal=9)(src.c)  # Discard macd line
```

### 3.3 Comments

```python
# Single-line comment

src = market_data_source()  # Inline comment

# Multi-line explanation:
# Calculate exponential moving average
# using 20-period lookback
```

### 3.4 Literals

#### Numbers
```python
integer = 42
decimal = 3.14
negative = -10
scientific = 1.5e-3  # 0.0015
```

#### Booleans
```python
enabled = True
disabled = False
```

#### Strings
```python
# Single or double quotes
label = "BULLISH"
symbol = 'AAPL'

# Triple-quoted strings (for JSON configs, multi-line text)
config = """
{
    "strategy": "momentum",
    "version": "1.0"
}
"""
```

#### None
```python
no_value = None
fallback = signal if condition else None
```

#### Lists (for transform options)
```python
# Used in certain transform options
colors = [0xFF0000, 0x00FF00, 0x0000FF]
labels = ["Buy", "Sell", "Hold"]
```

#### Dictionaries (for color maps, configurations)
```python
# Used in card_selector_filter and similar transforms
color_map = {
    Success: ["BULLISH", "BUY"],
    Error: ["BEARISH", "SELL"],
    Warning: ["NEUTRAL"]
}
```

---

## 4. Expressions and Operators

### 4.1 Arithmetic Operators

```python
# Binary operators
sum = a + b          # Addition
diff = a - b         # Subtraction
prod = a * b         # Multiplication
quot = a / b         # Division
mod = a % b          # Modulo (remainder)
power = a ** b       # Exponentiation

# Unary operators
neg = -a             # Negation (ONLY unary operator supported)
# pos = +a           # ❌ ERROR: Unary + not supported

# Examples
spread = fast_ema - slow_ema
ratio = close / open
percent_change = ((close - open) / open) * 100
distance_squared = dx ** 2 + dy ** 2
```

### 4.2 Comparison Operators

```python
greater = a > b
greater_equal = a >= b
less = a < b
less_equal = a <= b
equal = a == b
not_equal = a != b

# Examples
is_up = close > open
is_high_volume = volume >= avg_volume * 1.5
is_oversold = rsi_val < 30
price_unchanged = close == open
```

**⚠️ IMPORTANT:** Chained comparisons NOT supported:
```python
# ❌ ERROR
valid_range = 30 < rsi_val < 70

# ✅ CORRECT
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid_range = above_30 and below_70
```

### 4.3 Logical Operators

```python
# AND - all conditions must be true
both = a and b
all_three = a and b and c

# OR - at least one condition must be true
either = a or b
any_three = a or b or c

# NOT - negate condition
opposite = not a

# Examples
strong_buy = (price > ma) and (volume > avg_vol) and (rsi_val < 30)
any_signal = buy_signal or sell_signal
not_bearish = not is_bearish
```

### 4.4 Ternary Conditional

```python
# Syntax
result = value_if_true if condition else value_if_false

# Examples
direction = "UP" if bullish else "DOWN"
size = 100 if high_confidence else 50
label = "BUY" if oversold else ("SELL" if overbought else "HOLD")

# Can be nested
level = (
    "STRONG_BUY" if rsi_val < 20 else (
    "BUY" if rsi_val < 30 else (
    "NEUTRAL" if rsi_val < 70 else (
    "SELL" if rsi_val < 80 else "STRONG_SELL"
    ))))
```

### 4.5 Lag Operator (Time Series Indexing)

Access past or future values using bracket notation.

#### Backward Lag (Look Back)
```python
# Positive index = look backward in time
prev_close = src.c[1]      # Previous bar (1 bar ago)
prev_5 = src.c[5]          # 5 bars ago
week_ago = src.c[5]        # Assuming daily data, 1 week ago

# Use in calculations
price_change = src.c - src.c[1]
momentum = src.c - src.c[10]
rate_of_change = (src.c - src.c[10]) / src.c[10]
```

#### Forward Lag (Look Ahead) - For Forward Returns
```python
# Negative index = look forward in time
next_close = src.c[-1]     # Next bar (1 bar ahead)
future_5 = src.c[-5]       # 5 bars ahead

# Calculate forward returns (for target variables in ML)
forward_return_1d = (src.c[-1] - src.c) / src.c
forward_return_5d = (src.c[-5] - src.c) / src.c

# Binary classification target
future_up = src.c[-5] > src.c
```

#### Constant Variables in Lag

You can use constant variables to make lag indices more readable:

```python
# Define constant (compile-time evaluated)
lookback_period = 20
short_term = 5
long_term = 50

# Use in subscripts
price_lag = src.c[lookback_period]         # Same as src.c[20]
short_lag = src.v[short_term]              # Same as src.v[5]

# Arithmetic expressions allowed
combined = 10 + 5
mixed_lag = src.h[combined]                # Same as src.h[15]

# Use in calculations
momentum = src.c - src.c[lookback_period]
roc = (src.c - src.c[lookback_period]) / src.c[lookback_period]
```

**⚠️ CRITICAL:** Constants ONLY work in subscript indices, not elsewhere:
```python
# ✅ ALLOWED: In subscript
period = 20
lag = src.c[period]

# ❌ NOT ALLOWED: In transform options
period = 20
ma = sma(period=period)(src.c)  # ERROR: Must use literal

# ❌ NOT ALLOWED: In expressions
multiplier = 1.5
threshold = avg_vol * multiplier  # ERROR: Not folded
```

#### Lag Restrictions

```python
# ❌ ERROR: Zero index not allowed
current = src.c[0]  # ERROR: invalid_lag_zero

# ✅ CORRECT: Omit index
current = src.c

# ❌ ERROR: Float index not allowed
lag = src.c[1.5]  # ERROR: invalid_lag_float

# ✅ CORRECT: Integer only
lag = src.c[2]
```

### 4.6 Attribute Access

Access transform outputs using dot notation:

```python
# Market data outputs
src = market_data_source()
close = src.c      # Close price
high = src.h       # High price
volume = src.v     # Volume

# Indicator outputs
bb = bbands(period=20, stddev=2)(src.c)
upper_band = bb.bbands_upper
middle_band = bb.bbands_middle
lower_band = bb.bbands_lower

# Chained attribute access
smoothed = ema(period=10)(src.c).result  # Call then access
```

### 4.7 Operator Precedence

From **highest** to **lowest** priority:

1. **Subscript**: `x[1]`
2. **Attribute access**: `src.c`, `bb.bbands_upper`
3. **Function calls**: `ema(period=20)(src.c)`
4. **Exponentiation**: `x ** 2`
5. **Unary negation/not**: `-x`, `not x`
6. **Multiplication/Division/Modulo**: `*`, `/`, `%`
7. **Addition/Subtraction**: `+`, `-`
8. **Comparisons**: `<`, `>`, `<=`, `>=`, `==`, `!=`
9. **Logical AND**: `and`
10. **Logical OR**: `or`
11. **Ternary conditional**: `x if cond else y`

**Use parentheses for clarity:**
```python
# Ambiguous
result = a + b * c              # = a + (b * c)

# Clear
result = (a + b) * c            # Different result

# Complex expression
signal = (price > ma) and ((volume > avg * 1.5) or high_volatility)
```

---

## 5. Transform System

### 5.1 Transform Anatomy

Transforms are the core building blocks. Each transform has:

- **Name**: Identifier (e.g., `ema`, `sma`, `crossover`)
- **Options**: Configuration parameters (e.g., `period=20`)
- **Inputs**: Data to process (e.g., `src.c`)
- **Outputs**: Results (e.g., `result`, `upper`, `lower`)

### 5.2 Calling Transforms

**Standard Syntax:**
```python
result = transform_name(option1=value, option2=value)(input1, input2)
```

**Shorthand (no options):**
```python
# Full form
cross = crossover()(fast, slow)

# Shorthand (same thing)
cross = crossover(fast, slow)
```

**Examples:**
```python
# Single input, one option
ema_20 = ema(period=20)(src.c)

# Single input, multiple options
bb = bbands(period=20, stddev=2)(src.c)

# Multiple inputs, no options
cross_up = crossover(fast_ma, slow_ma)

# No inputs (data source)
src = market_data_source()

# No inputs, options
gaps = session_gap(fill_percent=100, timeframe="1Min")()
```

### 5.3 Transform Outputs

#### Single Output
```python
# Direct usage
ema_val = ema(period=20)(src.c)
signal = ema_val > 100

# Access .result explicitly (optional for single-output transforms)
signal = ema_val.result > 100
```

#### Multiple Outputs

**⚠️ CRITICAL: Transform outputs are NOT Python objects with arbitrary methods. They are special result handles with predefined attribute names. You CANNOT call these attributes as functions.**

```python
# ✅ CORRECT: Tuple unpacking (recommended)
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# ✅ CORRECT: Attribute access (no parentheses)
bb = bbands(period=20, stddev=2)(src.c)
upper_band = bb.bbands_upper
middle_band = bb.bbands_middle
lower_band = bb.bbands_lower

# ❌ WRONG: Cannot call attributes as functions
bb_upper = bbands(period=20, stddev=2).bbands_upper(src.c)  # ERROR!

# ✅ CORRECT: Discard unwanted outputs
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

**The only valid syntax patterns are:**
1. **Tuple unpacking**: `lower, middle, upper = bbands(...)(input)`
2. **Attribute access**: `bb = bbands(...)(input); upper = bb.bbands_upper`

**Note:** For the exact output handle names of any transform, refer to the transforms catalog at `/home/adesola/EpochLab/EpochAI/common/data_utils/catalogs/transforms_catalog.xml`.

### 5.4 Mathematical Functions

EpochFlow provides standard Python mathematical functions as transforms. These work like any other single-output transform:

**Trigonometric Functions:**
```python
# Basic trig (no options, shorthand syntax)
sine_val = sin(angle)
cosine_val = cos(angle)
tangent_val = tan(angle)

# Inverse trig
arc_sine = asin(value)
arc_cosine = acos(value)
arc_tangent = atan(value)
arc_tangent2 = atan2(y, x)  # Two-argument arctangent
```

**Exponential and Logarithmic:**
```python
# Exponential
exponential = exp(value)

# Logarithms
natural_log = log(value)      # Natural logarithm (ln)
log_base_10 = log10(value)    # Base-10 logarithm
```

**Rounding and Absolute Value:**
```python
# Rounding
rounded_up = ceil(value)      # Round up to nearest integer
rounded_down = floor(value)   # Round down to nearest integer

# Absolute value
absolute = abs(value)

# Square root
square_root = sqrt(value)
```

**Practical Examples:**
```python
src = market_data_source()

# Normalize angle for cyclical indicators
normalized_angle = atan2(src.c - src.o, src.h - src.l)

# Calculate volatility with logarithmic returns
log_return = log(src.c / src.c[1])

# Distance calculation
distance = sqrt(dx ** 2 + dy ** 2)

# Round position sizes
position_size = floor(capital / src.c)
```

**Note:** These mathematical functions are **not included** in the transforms catalog XML because they are Python built-ins. They are always available and work identically to other single-output transforms.

### 5.5 Chaining Transforms

```python
# Calculate EMA of close
ema_20 = ema(period=20)(src.c)

# EMA of another EMA (double smoothing)
ema_of_ema = ema(period=10)(ema_20)

# Inline chaining
smoothed = ema(period=10)(ema(period=20)(src.c))

# Use in expressions
signal = src.c > ema(period=20)(src.c)
```

---

## 6. TimeFrame System

### 6.1 Basic TimeFrames

The `timeframe` parameter allows transforms to process data at a different resolution than the strategy's base timeframe.

**Syntax:**
```python
result = transform(option=value, timeframe="resolution")(inputs)
```

**How it works:** The framework automatically resamples/aggregates data to the specified timeframe BEFORE the transform executes.

**Example:**
```python
# Strategy runs on 15-minute bars
src = market_data_source()

# Calculate daily EMA (resampled to 1 day)
daily_ema = ema(period=50, timeframe="1D")(src.c)

# Intraday price vs daily trend
is_above_daily = src.c > daily_ema
```

### 6.2 Standard TimeFrames

#### Intraday (Minute/Hour)
```python
"1Min"    # 1 minute
"2Min"    # 2 minutes
"3Min"    # 3 minutes
"5Min"    # 5 minutes
"10Min"   # 10 minutes
"15Min"   # 15 minutes
"30Min"   # 30 minutes
"45Min"   # 45 minutes
"1H"      # 1 hour
"2H"      # 2 hours
"3H"      # 3 hours
"4H"      # 4 hours
```

**Note:** Case-insensitive variants work too: `"1h"`, `"5min"`, etc.

#### Daily and Above
```python
"1D"      # 1 day
"1W"      # 1 week
"1M"      # 1 month (month-end by default)
```

**Note:** Aliases like `"1Day"`, `"1Hour"`, `"1Week"` are **NOT supported**. Use short forms: `"1D"`, `"1H"`, `"1W"`.

### 6.3 Advanced TimeFrames

#### Month Anchoring
```python
"1MS"     # Month start (first calendar day)
"1ME"     # Month end (last calendar day)

# Example: Month-end closing price
month_end_close = market_data_source(timeframe="1ME")().c
```

#### Quarter Anchoring
```python
"1QS"     # Quarter start (Jan 1, Apr 1, Jul 1, Oct 1)
"1QE"     # Quarter end (Mar 31, Jun 30, Sep 30, Dec 31)

# Example: Quarterly returns
quarterly_returns = (src.c - src.c[1]) / src.c[1]  # At QE resolution
```

#### Year Anchoring
```python
"1YS"     # Year start (January 1)
"1YE"     # Year end (December 31)

# Example: Year-end price
yearly_close = market_data_source(timeframe="1YE")().c
```

#### Week Anchoring (Day of Week)
```python
"1W-SUN"  # Week starting Sunday
"1W-MON"  # Week starting Monday (default)
"1W-TUE"  # Week starting Tuesday
"1W-WED"  # Week starting Wednesday
"1W-THU"  # Week starting Thursday
"1W-FRI"  # Week starting Friday
"1W-SAT"  # Week starting Saturday

# Example: Weekly bars starting Friday (Forex convention)
weekly_fri = ema(period=10, timeframe="1W-FRI")(src.c)
```

#### Week of Month
```python
"1W-MON-1st"   # First Monday of month
"1W-MON-2nd"   # Second Monday of month
"1W-MON-3rd"   # Third Monday of month
"1W-MON-4th"   # Fourth Monday of month
"1W-FRI-Last"  # Last Friday of month

# Example: Options expiration (3rd Friday)
expiration_weeks = market_data_source(timeframe="1W-FRI-3rd")()
```

#### Business Days
```python
"1B"      # 1 business day (excludes weekends/holidays)
"5B"      # 5 business days (1 week)
"21B"     # ~1 month of business days

# Example: 20-day business day moving average
bday_ma = sma(period=20, timeframe="1B")(src.c)
```

### 6.4 Multi-TimeFrame Strategies

**Pattern:** Higher timeframe for trend, lower timeframe for entry

```python
src = market_data_source()

# Daily trend filter
daily_trend = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_trend

# Intraday entry signals (15-min strategy)
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
entry = crossover(fast, slow)

# Combine: Only enter with daily trend
buy = entry and uptrend
trade_signal_executor()(enter_long=buy)
```

### 6.5 TimeFrame Specification Details

TimeFrames are based on pandas DateOffset strings with proprietary extensions:

**Supported Units:**
- Minute: `Min`, `T`
- Hour: `H`, `Hour`
- Day: `D`, `Day`
- Business Day: `B`, `BDay`
- Week: `W`
- Month: `M`, `MS` (start), `ME` (end), `BM` (business month)
- Quarter: `Q`, `QS` (start), `QE` (end)
- Year: `Y`, `YS` (start), `YE` (end)

**Anchoring:**
- Week anchor: `-SUN`, `-MON`, `-TUE`, `-WED`, `-THU`, `-FRI`, `-SAT`
- Week of month: `-1st`, `-2nd`, `-3rd`, `-4th`, `-Last`
- Month anchor: `jan`, `feb`, `mar`, etc.

**Examples:**
```python
"15Min"           # 15 minutes
"1H"              # 1 hour
"1D"              # 1 calendar day
"1B"              # 1 business day
"1W-FRI"          # Weekly, Friday anchor
"1W-FRI-Last"     # Last Friday of month
"1ME"             # Month end
"1QS-JAN"         # Quarter start, January anchor
```

---

## 7. Session System

### 7.1 Session Parameter vs sessions() Transform

**CRITICAL DISTINCTION:**

| Feature | `session` Parameter | `sessions()` Transform |
|---------|---------------------|------------------------|
| Purpose | **Filter** data to specific session | **Detect** session boundaries |
| Usage | `transform(session="London")(inputs)` | `sessions(session_type="London")()` |
| Effect | Only process bars within session | Mark which bars are in session |
| Output | Filtered transform result | `active`, `high`, `low`, `opened`, `closed` |

### 7.2 Session Parameter (Data Filtering)

**Purpose:** Execute transform on ONLY the bars that fall within a specific trading session.

**Syntax:**
```python
result = transform(option=value, session="session_name")(inputs)
```

**How it works:** Framework calls `SliceBySession()` internally to filter DataFrame BEFORE transform execution.

**Example:**
```python
src = market_data_source()

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
- Gap analysis by time zone

### 7.3 sessions() Transform (Session Detection)

**Purpose:** Mark session boundaries and track session high/low.

**Syntax:**
```python
session = sessions(session_type="session_name")()
```

**Outputs:**
- `active`: Boolean - True during session, False otherwise
- `high`: Decimal - Session high price
- `low`: Decimal - Session low price
- `opened`: Boolean - True on session open bar
- `closed`: Boolean - True on session close bar

**Example:**
```python
src = market_data_source()
london = sessions(session_type="London")()

# Only trade during London session
in_session = london.active
entry_signal = bullish_condition and in_session

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Exit at session close
exit_signal = london.closed
```

### 7.4 Available Sessions

#### Geographic Sessions (Major Trading Centers)
```python
"Sydney"    # Sydney, Australia (10 PM - 7 AM GMT)
"Tokyo"     # Tokyo, Japan (12 AM - 9 AM GMT)
"London"    # London, UK (8 AM - 5 PM GMT)
"NewYork"   # New York, USA (1 PM - 10 PM GMT)
```

#### Kill Zones (ICT Concept - High Liquidity Periods)
```python
"AsianKillZone"         # Asian session liquidity sweep (19:00-23:00 ET)
"LondonOpenKillZone"    # London open manipulation (02:00-05:00 ET)
"NewYorkKillZone"       # New York AM session (07:00-10:00 ET)
"LondonCloseKillZone"   # London close liquidity (10:00-12:00 ET)
```

**Note:** There is NO "LondonKillZone" session type - use "LondonOpenKillZone" or "LondonCloseKillZone".

### 7.5 Session Type Details

Sessions are defined in `session_variant.h` as:

```cpp
CREATE_ENUM(SessionType, Sydney, Tokyo, London, NewYork, AsianKillZone,
            LondonOpenKillZone, NewYorkKillZone, LondonCloseKillZone);
```

Each `SessionType` maps to a `SessionRange` (start time, end time) in the `SessionRegistry`.

### 7.6 Custom Session Ranges

**Advanced:** You can define custom `SessionRange` objects with arbitrary start/end times, but this is typically done at the framework level, not in DSL code.

**SessionRange Structure:**
```cpp
struct SessionRange {
    Time start;  // Session start time (HH:MM:SS)
    Time end;    // Session end time (HH:MM:SS)
};
```

### 7.7 Session-Anchored TimeFrames

**Advanced:** Combine sessions with timeframes for session-relative offsets.

**SessionAnchorType:**
- `AfterOpen`: Offset from session open
- `BeforeClose`: Offset before session close

**Example (Framework Level):**
```python
# 30 minutes after London open
london_open_plus_30 = ema(
    period=20,
    session="London",
    session_anchor="AfterOpen",
    time_offset=TimeDelta(minutes=30)
)(src.c)
```

**Note:** This is an advanced feature for custom timeframe construction, not commonly used in DSL code.

### 7.8 Combining TimeFrame and Session

Use both parameters together for fine-grained control:

```python
# Daily bars, but only from New York session data
daily_ny_close = market_data_source(
    timeframe="1D",
    session="NewYork"
)().c

# Hourly EMA, London session only
hourly_london_ema = ema(
    period=20,
    timeframe="1H",
    session="London"
)(src.c)
```

**Execution Order:**
1. Filter data by `session` (if specified)
2. Resample to `timeframe` (if specified)
3. Execute transform

---

## 8. Type System

### 8.1 Data Types

| Type | Description | Examples |
|------|-------------|----------|
| `Boolean` | True/False | `True`, `False` |
| `Integer` | Whole numbers | `42`, `-5`, `0` |
| `Decimal` | Floating-point | `3.14`, `0.5`, `-1.23` |
| `Number` | Integer or Decimal | Any numeric value |
| `String` | Text | `"BULLISH"`, `'AAPL'` |
| `Any` | Any type | - |

### 8.2 Type Compatibility

#### Numbers
```python
# Integers and decimals mix freely
int_val = 10
dec_val = 3.14
result = int_val + dec_val  # OK: 13.14 (Decimal)
```

#### Booleans and Numbers
```python
# Boolean → Number: True = 1, False = 0
is_up = True
score = is_up + 10  # OK: 11

# Number → Boolean: 0 = False, non-zero = True
volume = 1000
has_volume = volume and True  # OK: True (1000 != 0)
```

#### Strings (Isolated)
```python
# ❌ Strings don't mix with numbers
label = "PRICE"
result = label + 10  # ERROR: incompatible_type_cast

# ✅ Use strings only for labels
direction = "UP" if bullish else "DOWN"
```

### 8.3 Type Inference

Types are automatically inferred:

```python
src = market_data_source()
close = src.c              # Decimal (market data output)
is_high = close > 100      # Boolean (comparison)
sum = 10 + 20              # Integer (literal arithmetic)
ratio = close / 100        # Decimal (division with Decimal)
```

### 8.4 Automatic Type Casting

The compiler automatically inserts cast nodes when needed:

```python
# Boolean to Number
is_bullish = True
numeric_score = is_bullish + 1  # Auto-cast: True → 1

# Number to Boolean
volume = 500
has_volume = volume and True    # Auto-cast: 500 → True (non-zero)
```

**Cast Types:**
- `bool_to_num`: Boolean → Number
- `num_to_bool`: Number → Boolean

### 8.5 Type Errors

```python
# ❌ Incompatible types
string_label = "HIGH"
result = string_label + 100  # ERROR: incompatible_type_cast

# ❌ Wrong type for option
ema_val = ema(period="20")(src.c)  # ERROR: period expects Integer, got String
```

---

## 9. Core Transforms

This section covers **architectural transforms** essential for strategy structure. Indicator transforms (ema, sma, rsi, etc.) are documented in the indicator catalog appended to this reference.

### 9.1 Data Sources

#### market_data_source()

**Purpose:** Access OHLCV market data

**Usage:**
```python
src = market_data_source()
```

**Outputs:**
- `o`: Open price
- `h`: High price
- `l`: Low price
- `c`: Close price
- `v`: Volume
- `vw`: Volume-weighted average price
- `n`: Number of trades

**Example:**
```python
src = market_data_source()
close = src.c
high = src.h
volume = src.v

# Use in strategy
is_bullish = close > close[1]
has_volume = volume > 1000
```

### 9.2 Trade Execution

#### trade_signal_executor()

**Purpose:** Execute trades based on signals (**REQUIRED** in every trading strategy)

**Usage:**
```python
trade_signal_executor()(
    enter_long=buy_signal,
    enter_short=sell_signal,
    exit_long=exit_buy,
    exit_short=exit_sell
)
```

**Inputs (all Boolean, all optional but at least one required):**
- `enter_long`: Signal to open long position
- `enter_short`: Signal to open short position
- `exit_long`: Signal to close long position
- `exit_short`: Signal to close short position

**Execution Rules:**
1. Exits processed **before** entries
2. If both `enter_long` and `enter_short` are True: no trade
3. Can flip positions (close long + open short in one bar)
4. Conflicting signals (enter + exit same direction): exit takes precedence

**Example:**
```python
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

buy = crossover(fast, slow)
sell = crossover(slow, fast)

trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### 9.3 Comparison and Logic

#### crossover() / crossunder()

**Purpose:** Detect when one line crosses another

**Usage:**
```python
cross_up = crossover(a, b)    # a crosses above b
cross_down = crossunder(a, b) # a crosses below b
```

**Example:**
```python
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

golden_cross = crossover(fast, slow)
death_cross = crossunder(fast, slow)
```

### 9.4 Session Detection

#### sessions()

**Purpose:** Detect trading sessions and track session high/low

**Usage:**
```python
session = sessions(session_type="session_name")()
```

**Options:**
- `session_type`: One of the valid session types (see Section 7.4)

**Outputs:**
- `active`: Boolean - True during session
- `high`: Decimal - Session high price
- `low`: Decimal - Session low price
- `opened`: Boolean - True on session open bar
- `closed`: Boolean - True on session close bar

**Example:**
```python
london = sessions(session_type="London")()
ny = sessions(session_type="NewYork")()

# Trade during London session only
is_london = london.active
entry = bullish_signal and is_london

# Breakout from session range
breakout = src.c > london.high
```

---

## 10. Complete Examples

### 10.1 Moving Average Crossover

**Strategy:** Buy when fast MA crosses above slow MA, sell on reverse cross.

```python
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

buy = crossover(fast, slow)
sell = crossover(slow, fast)

trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### 10.2 RSI Mean Reversion

**Strategy:** Buy oversold, sell overbought, exit at midline.

```python
src = market_data_source()
rsi_val = rsi(period=14)(src.c)

oversold = rsi_val < 30
overbought = rsi_val > 70

exit_long = rsi_val > 50
exit_short = rsi_val < 50

trade_signal_executor()(
    enter_long=oversold,
    enter_short=overbought,
    exit_long=exit_long,
    exit_short=exit_short
)
```

### 10.3 Multi-TimeFrame Trend Filter

**Strategy:** Daily trend filter with intraday entries.

```python
src = market_data_source()

# Daily trend (strategy on 15Min bars)
daily_ema = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_ema
downtrend = src.c < daily_ema

# Intraday signals
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
intraday_buy = crossover(fast, slow)
intraday_sell = crossover(slow, fast)

# Combine: Only trade with daily trend
entry_long = intraday_buy and uptrend
entry_short = intraday_sell and downtrend

trade_signal_executor()(enter_long=entry_long, enter_short=entry_short)
```

### 10.4 Session Breakout

**Strategy:** Trade breakouts during London session.

```python
src = market_data_source()
london = sessions(session_type="London")()

# Session state
in_session = london.active

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Only trade during session
entry_long = in_session and breakout_high
entry_short = in_session and breakout_low

# Exit at session close
exit_both = london.closed

trade_signal_executor()(
    enter_long=entry_long,
    enter_short=entry_short,
    exit_long=exit_both,
    exit_short=exit_both
)
```

### 10.6 Volume-Confirmed Trend

**Strategy:** Enter trends only with high volume.

```python
src = market_data_source()

# Trend
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
trend_up = fast > slow
trend_down = slow > fast

# Volume filter
avg_volume = sma(period=20)(src.v)
high_volume = src.v > (avg_volume * 1.5)

# Crossover triggers
cross_up = crossover(fast, slow)
cross_down = crossover(slow, fast)

# Combine: Trend + Volume + Crossover
entry_long = trend_up and high_volume and cross_up
entry_short = trend_down and high_volume and cross_down

trade_signal_executor()(enter_long=entry_long, enter_short=entry_short)
```

### 10.7 Forward Return Calculation

**Strategy:** Calculate 5-day forward return for ML target.

```python
src = market_data_source()

# Forward return (5 days ahead)
future_close = src.c[-5]
forward_return = (future_close - src.c) / src.c

# Binary classification target
target = future_return > 0.02  # Positive if > 2% gain

# Note: This is for research/ML, not trading
# (You can't trade on future data)
```

### 10.8 Momentum with Dynamic Exits

**Strategy:** Enter on strong momentum, exit when it weakens.

```python
src = market_data_source()

# Rate of change
lookback = 10
roc = (src.c - src.c[lookback]) / src.c[lookback]

# Strong momentum
strong_bull = roc > 0.02   # +2% in 10 bars
strong_bear = roc < -0.02  # -2% in 10 bars

# Confirmation
current_up = src.c > src.c[1]
current_down = src.c < src.c[1]

# Entry
entry_long = strong_bull and current_up
entry_short = strong_bear and current_down

# Exit when momentum weakens
exit_long = roc < 0.005
exit_short = roc > -0.005

trade_signal_executor()(
    enter_long=entry_long,
    entry_short=entry_short,
    exit_long=exit_long,
    exit_short=exit_short
)
```

---

## 11. Error Reference

### 11.1 Syntax Errors

#### "Variable 'x' already bound"
```python
# ❌ ERROR: Reassignment
price = src.c
price = src.o  # ERROR

# ✅ FIX: Different names
close_price = src.c
open_price = src.o
```

#### "Undefined variable 'x'"
```python
# ❌ ERROR: Use before definition
signal = ema_val > 100
ema_val = ema(period=20)(src.c)  # ERROR

# ✅ FIX: Define before use
ema_val = ema(period=20)(src.c)
signal = ema_val > 100
```

#### "Chained comparison not supported"
```python
# ❌ ERROR
valid = 30 < rsi_val < 70

# ✅ FIX
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70
```

#### "Unary plus operator not supported"
```python
# ❌ ERROR
positive = +value

# ✅ FIX
positive = value
```

### 11.2 Lag Operator Errors

#### "invalid_lag_zero"
```python
# ❌ ERROR
current = src.c[0]

# ✅ FIX
current = src.c
```

#### "invalid_lag_float"
```python
# ❌ ERROR
lag = src.c[1.5]

# ✅ FIX
lag = src.c[2]
```

### 11.3 Type Errors

#### "incompatible_type_cast"
```python
# ❌ ERROR: String + Number
label = "PRICE"
result = label + 100

# ✅ FIX: Use correct types
value = 100
result = value + 100
```

#### "Type mismatch for option 'period'"
```python
# ❌ ERROR: String instead of Integer
ema_val = ema(period="20")(src.c)

# ✅ FIX: Use Integer
ema_val = ema(period=20)(src.c)
```

### 11.4 Transform Errors

#### "Missing required option 'period'"
```python
# ❌ ERROR
ema_val = ema()(src.c)

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

#### "Unknown handle 'result' for transform 'market_data_source'"
```python
# ❌ ERROR
src = market_data_source()
price = src.result  # market_data_source has no 'result' handle

# ✅ FIX: Use correct handle
price = src.c
```

#### "Unknown option 'periood'"
```python
# ❌ ERROR: Typo
ema_val = ema(periood=20)(src.c)

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

### 11.5 Multi-Output Errors

#### "ambiguous_multi_output"
```python
# ❌ ERROR: Multi-output in expression
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # ERROR: Which output?

# ✅ FIX: Access specific handle
bb = bbands(period=20, stddev=2)(src.c)
signal = bb.bbands_upper > 100
```

#### "Right-hand side must be a constructor call"
```python
# ❌ ERROR: Calling an attribute as a function
bb_upper = bbands(period=20, stddev=2).bbands_upper(src.c)  # WRONG!

# ✅ FIX: Use tuple unpacking (recommended)
bb_lower, bb_middle, bb_upper = bbands(period=20, stddev=2)(src.c)

# ✅ FIX: Or attribute access (no call)
bb = bbands(period=20, stddev=2)(src.c)
bb_upper = bb.bbands_upper  # Access attribute, don't call it
```

#### "Tuple unpacking count mismatch"
```python
# ❌ ERROR: 3 outputs, 2 variables
lower, upper = bbands(period=20, stddev=2)(src.c)

# ✅ FIX: Match count
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# Or discard
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

### 11.6 Executor Errors

#### "No trade_signal_executor found"
```python
# ❌ ERROR: Missing executor
src = market_data_source()
signal = src.c > src.c[1]
# Missing executor!

# ✅ FIX
src = market_data_source()
signal = src.c > src.c[1]
trade_signal_executor()(enter_long=signal)
```

#### "Multiple trade_signal_executor calls"
```python
# ❌ ERROR: Multiple executors
trade_signal_executor()(enter_long=buy)
trade_signal_executor()(enter_short=sell)  # ERROR

# ✅ FIX: Single executor
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### 11.7 TimeFrame/Session Errors

#### "Invalid timeframe 'BAD_TF'"
```python
# ❌ ERROR
daily = ema(period=50, timeframe="BAD_TF")(src.c)

# ✅ FIX: Use valid timeframe
daily = ema(period=50, timeframe="1D")(src.c)
```

#### "Invalid session 'BAD_SESSION'"
```python
# ❌ ERROR
london_ema = ema(period=20, session="BAD_SESSION")(src.c)

# ✅ FIX: Use valid session
london_ema = ema(period=20, session="London")(src.c)
```

---

## 12. AI Agent Quick Reference

### 12.1 Validation Checklist

Before generating code, verify:

- [ ] **No reassignment**: Each variable assigned exactly once
- [ ] **No control flow**: No `if`/`for`/`while`/`def`/`class`
- [ ] **No chained comparisons**: Break `a < b < c` into separate checks
- [ ] **Lag indices**: Non-zero integers only (`[1]`, `[5]`, `[-1]`, NOT `[0]`, `[1.5]`)
- [ ] **Multi-output unpacking**: Count matches (`lower, middle, upper = bbands(...)`)
- [ ] **No calling attributes**: Use `bb.bbands_upper`, NOT `bb.bbands_upper(src.c)`
- [ ] **Transform options**: All required options provided, correct types
- [ ] **Executor required**: Trading strategies have exactly one `trade_signal_executor()`
- [ ] **Valid timeframes**: Use supported timeframe strings (Section 6)
- [ ] **Valid sessions**: Use supported session types (Section 7.4)
- [ ] **Constants in subscripts only**: Don't use constant variables in options/expressions

### 12.2 Common Mistakes

| Mistake | Error | Fix |
|---------|-------|-----|
| `price = src.c; price = src.o` | Reassignment | Use `close_price`, `open_price` |
| `if cond: x = 1` | Control flow | Use `x = 1 if cond else 0` |
| `a < b < c` | Chained comparison | `(a < b) and (b < c)` |
| `src.c[0]` | Zero lag | `src.c` |
| `src.c[1.5]` | Float lag | `src.c[2]` |
| `lower, upper = bbands(...)` | Tuple count | `lower, _, upper = bbands(...)` |
| `bb.bbands_upper(src.c)` | Calling attribute | `bb = bbands(...)(src.c); upper = bb.bbands_upper` |
| `timeframe="1Day"` | Invalid timeframe | `timeframe="1D"` |
| `ema()(src.c)` | Missing option | `ema(period=20)(src.c)` |
| No executor | Missing executor | Add `trade_signal_executor()` |

### 12.3 Pattern Library

#### Entry + Exit
```python
entry_long = bullish_condition
exit_long = bearish_condition
trade_signal_executor()(enter_long=entry_long, exit_long=exit_long)
```

#### Multi-Condition Signal
```python
trend = fast > slow
volume = src.v > avg_vol * 1.5
momentum = src.c > src.c[10]
entry = trend and volume and momentum
```

#### Ternary Selector
```python
label = "BUY" if oversold else ("SELL" if overbought else "HOLD")
```

#### Multi-Output Unpacking (Bollinger Bands)
```python
# Recommended: Tuple unpacking
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# Alternative: Attribute access
bb = bbands(period=20, stddev=2)(src.c)
upper = bb.bbands_upper
middle = bb.bbands_middle
lower = bb.bbands_lower
```

#### Forward Return
```python
future_close = src.c[-5]
forward_return = (future_close - src.c) / src.c
```

#### Session Filtering
```python
london_ema = ema(period=20, session="London")(src.c)
```

#### Multi-TimeFrame
```python
daily_trend = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_trend
```

#### Mathematical Functions
```python
# Logarithmic returns
log_return = log(src.c / src.c[1])

# Normalize angles
angle = atan2(y_component, x_component)

# Distance/magnitude
distance = sqrt(dx ** 2 + dy ** 2)

# Round values
rounded = floor(position_size)
```

### 12.4 Critical Rules

1. **Single Assignment**: Variables are immutable
2. **No Zero Lag**: `src.c[0]` is invalid, use `src.c`
3. **Integer Lag Only**: `src.c[1.5]` is invalid, use `src.c[2]`
4. **Negative Lag for Forward**: `src.c[-1]` = next bar (future)
5. **Tuple Count Match**: Unpacking must match output count exactly
6. **Constants in Subscripts Only**: `period = 20; src.c[period]` ✅, but `sma(period=period)` ❌
7. **One Executor**: Trading strategies need exactly one `trade_signal_executor()`
8. **No Chained Comparisons**: `a < b < c` not supported, use `(a < b) and (b < c)`
9. **Valid TimeFrames**: Use Section 6 list (`"1D"`, `"1H"`, `"1W-FRI"`, etc.) - NOT `"1Day"` or `"1Hour"`
10. **Valid Sessions**: Use Section 7.4 list (`"London"`, `"NewYork"`, etc.)

---

**END OF EPOCHFLOW DSL REFERENCE**

*For indicator catalog, see appended documentation.*
