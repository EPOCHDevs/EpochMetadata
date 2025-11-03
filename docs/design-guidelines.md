---
page_type: concept
layout: default
order: 4
category: Language
description: Essential design patterns, executor requirements, Python incompatibilities, and common mistakes
parent: ./index.md
---

# EpochScript Design Guidelines

Essential design patterns and constraints for writing EpochScript strategies.

---


## Strategy Executor Requirements

### Every Trading Strategy Must Have Exactly One Executor

Trading strategies require exactly one `trade_signal_executor()` call. This is not an error you encounter randomly - it's a fundamental design requirement.

**Why?** The executor is the bridge between your strategy logic and actual trade execution. Without it, your strategy is just calculations with no trades.

#### Pattern: Single Executor

```epochscript
# Correct: - One executor with all signals
src = market_data_source()

fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

buy_signal = crossover(fast, slow)
sell_signal = crossover(slow, fast)

trade_signal_executor()(
    enter_long=buy_signal,
    enter_short=sell_signal
)
```

#### Anti-Pattern: Missing Executor

```epochscript
# Incorrect: - No executor
src = market_data_source()
signal = src.c > src.c[1]
# Strategy ends here - no trades will execute!
```

**Error message:** `No trade_signal_executor found in strategy`

#### Anti-Pattern: Multiple Executors

```epochscript
# Incorrect: - Multiple executors (conflicting logic)
trade_signal_executor()(enter_long=buy)
trade_signal_executor()(enter_short=sell)  # Second executor conflicts!
```

**Error message:** `Multiple trade_signal_executor calls found`

**Fix:** Combine all signals into single executor call.

#### Anti-Pattern: Empty Executor

```epochscript
# Incorrect: - Executor with no signals
trade_signal_executor()()  # What should it execute?
```

**Error message:** `trade_signal_executor requires at least one input`

**Fix:** Provide at least one signal (enter_long, enter_short, exit_long, or exit_short).

### Executor Signal Inputs

The executor accepts four optional boolean inputs (but requires at least one):

- `enter_long` - Signal to open long position
- `enter_short` - Signal to open short position
- `exit_long` - Signal to close long position
- `exit_short` - Signal to close short position

**Execution order:** Exits are processed BEFORE entries on the same bar.

**Example: Long-only strategy with exit logic**

```epochscript
src = market_data_source()
ema_20 = ema(period=20)(src.c)
atr_val = atr(period=14)(src.h, src.l, src.c)

# Entry signal
buy = crossover(src.c, ema_20)

# Exit signal (stop-loss or take-profit)
stop_distance = 2 * atr_val
exit = src.c < (ema_20 - stop_distance)

trade_signal_executor()(
    enter_long=buy,
    exit_long=exit
)
```

---

## Python Features Not Supported

EpochScript looks like Python but is **NOT Python**. It's a domain-specific language for quantitative strategies with strict constraints.

### Control Flow Statements

**Not supported:**
- `if`/`elif`/`else` statements
- `for` loops
- `while` loops
- `def` function definitions
- `class` definitions
- `try`/`except` error handling

**Why?** EpochScript operates on time series data where every expression evaluates for all time points simultaneously. Control flow would break this model.

#### Wrong: If Statement

```epochscript
# Incorrect: - if/else not allowed
if price > 100:
    signal = True
else:
    signal = False
```

**Error message:** `If statements not supported`

#### Correct: Boolean Expression or Ternary

```epochscript
# FIX 1: Direct boolean expression
signal = price > 100

# FIX 2: Ternary operator (for value selection)
signal = True if price > 100 else False

# FIX 3: conditional_select for multi-way branching
regime = conditional_select(
    rsi_val < 30, "oversold",
    rsi_val > 70, "overbought",
    "neutral"
)
```

### Variable Reassignment

Variables are **immutable** (single-assignment only). Once bound, they cannot be reassigned.

**Why?** Prevents temporal logic bugs and enables parallel evaluation.

#### Wrong: Reassignment

```epochscript
# Incorrect: - Cannot reassign variables
price = src.c
price = src.o  # ERROR!
```

**Error message:** `Variable 'price' already bound`

#### Correct: Unique Names

```epochscript
# FIX - Use distinct variable names
close_price = src.c
open_price = src.o
```

### Chained Comparisons

Python's chained comparison syntax is not supported.

**Why?** Ambiguous semantics for time series data.

#### Wrong: Chained Comparison

```epochscript
# Incorrect: - Chained comparison not supported
valid = 30 < rsi_val < 70
```

**Error message:** `Chained comparison not supported`

#### Correct: Explicit Boolean Logic

```epochscript
# FIX - Break into explicit conditions
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70

# Or inline:
valid = (rsi_val > 30) and (rsi_val < 70)
```

### Negative Indexing

Python's negative list indexing (`[-1]` for last element) is not supported.

**Why?** Lag semantics are forward-looking by default. Negative indices would be confusing.

#### Wrong: Negative Index

```epochscript
# Incorrect: - Negative lag not allowed
yesterday = src.c[-1]  # Python lists allow this
```

**Error message:** `Negative lag indices not supported`

#### Correct: Positive Lag

```epochscript
# FIX - Use positive lag (historical lookback)
yesterday = src.c[1]  # 1 bar ago
last_week = src.c[5]  # 5 bars ago
```

### Zero Indexing

Python's `[0]` for current element is not needed.

#### Wrong: Zero Index

```epochscript
# Incorrect: - Zero lag is redundant
current = src.c[0]
```

**Error message:** `invalid_lag_zero`

#### Correct: Omit Index for Current Value

```epochscript
# FIX - Omit index for current value
current = src.c
```

---

## Transform Usage Patterns

### Multi-Output Transform Access

Transforms that return multiple outputs (like `bbands`, `macd`, `stoch`) cannot be used directly in expressions without specifying which output.

**Why?** Ambiguous which output you want.

#### Anti-Pattern: Ambiguous Multi-Output

```epochscript
# Incorrect: - Which output?
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # bb has 3 outputs: lower, middle, upper
```

**Error message:** `ambiguous_multi_output: Cannot use multi-output transform in expression context`

#### Pattern 1: Attribute Access

```epochscript
# FIX - Access specific output by name
bb = bbands(period=20, stddev=2)(src.c)
signal = bb.bbands_upper > 100
resistance = bb.bbands_upper
support = bb.bbands_lower
```

#### Pattern 2: Tuple Unpacking

```epochscript
# FIX - Unpack all outputs
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
signal = upper > 100

# Discard unwanted outputs with underscore
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

### Tuple Unpacking Count Matching

When unpacking, variable count must match output count exactly (or use `_` for discards).

#### Wrong: Count Mismatch

```epochscript
# Incorrect: - bbands has 3 outputs, but only 2 variables
lower, upper = bbands(period=20, stddev=2)(src.c)
```

**Error message:** `Tuple unpacking count mismatch: expected 3 values, got 2 variables`

#### Correct: Matching Count

```epochscript
# FIX 1: Match all outputs
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# FIX 2: Discard unwanted with underscore
lower, _, upper = bbands(period=20, stddev=2)(src.c)

# FIX 3: Use attribute access instead
bb = bbands(period=20, stddev=2)(src.c)
lower = bb.bbands_lower
upper = bb.bbands_upper
```

### Only Unpack Transform Calls

You can only unpack direct transform constructor calls, not intermediate variables or expressions.

#### Wrong: Unpacking Non-Constructor

```epochscript
# Incorrect: - Cannot unpack variable
bb = bbands(period=20, stddev=2)(src.c)
lower, middle, upper = bb  # Not a constructor call!
```

**Error message:** `Right-hand side must be a constructor call`

#### Correct: Unpack Constructor Directly

```epochscript
# FIX - Unpack the constructor call directly
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
```

### event_marker Color Map Pattern

The `event_marker` transform requires a `color_map` dictionary that maps color categories to event names.

#### Requirements:

1. **Color categories** must be: `Success` (green), `Warning` (yellow), `Error` (red), or `Info` (blue)
2. **Event names** in color_map must match input parameter names
3. **All inputs** must be boolean series

#### Wrong: Color Mismatch

```epochscript
# Incorrect: - Using color names instead of categories
event_marker(color_map={
    "Green": ["oversold"],   # Should be Success, not "Green"
    "Red": ["overbought"]    # Should be Error, not "Red"
})(oversold=rsi_val < 30, overbought=rsi_val > 70)
```

**Error message:** `Unknown color category 'Green'`

#### Wrong: Name Mismatch

```epochscript
# Incorrect: - 'breakout' in color_map but not in inputs
event_marker(color_map={
    Success: ["breakout"]
})(
    signal=src.c > upper  # Input name doesn't match!
)
```

**Error message:** `Event 'breakout' referenced in color_map but not provided as input`

#### Wrong: Non-Boolean Input

```epochscript
# Incorrect: Passing numeric values instead of boolean conditions
rsi_val = rsi(period=14)(src.c)

event_marker(color_map={
    Success: ["signal"]
})(
    signal=rsi_val  # ERROR: Decimal, not Boolean
)
```

**Error message:** `event_marker expects Boolean input, got Decimal`

**Correct:**
```epochscript
# Pass boolean condition, not raw value
rsi_val = rsi(period=14)(src.c)

event_marker(color_map={
    Success: ["oversold"]
})(
    oversold=rsi_val < 30  # ✅ Boolean condition
)
```

:::note
All event_marker inputs must be boolean series (True/False values). If you have a numeric indicator, convert it to a condition using comparison operators (`<`, `>`, `==`, etc.).
:::

#### Correct: Proper Color Map

```epochscript
# Correct: - Categories and names match
src = market_data_source()
rsi_val = rsi(period=14)(src.c)
lower, _, upper = bbands(period=20, stddev=2)(src.c)

event_marker(color_map={
    Success: ["oversold", "support_break"],    # Green
    Error: ["overbought", "resistance_break"],  # Red
    Warning: ["neutral"],                       # Yellow
    Info: ["crossover"]                         # Blue
})(
    oversold=rsi_val < 30,
    overbought=rsi_val > 70,
    neutral=(rsi_val >= 30) and (rsi_val <= 70),
    support_break=src.c > lower,
    resistance_break=src.c > upper,
    crossover=crossover(src.c, upper)
)
```

---

## Timeframe and Session Formats

### Valid Timeframe Formats

EpochScript uses specific string formats for timeframes. Common abbreviations from Python or other languages won't work.

#### Valid Formats:

**Intraday:**
- `"1Min"`, `"5Min"`, `"15Min"`, `"30Min"`
- `"1H"`, `"2H"`, `"4H"`

**Daily and Higher:**
- `"1D"` (daily)
- `"1W"` (weekly)
- `"1M"` (monthly)

#### Wrong Formats:

```epochscript
# Incorrect: - These don't work
daily = ema(period=50, timeframe="1Day")(src.c)   # Use "1D"
hourly = ema(period=20, timeframe="1Hour")(src.c)  # Use "1H"
minute = ema(period=10, timeframe="1Minute")(src.c) # Use "1Min"
```

**Error message:** `Invalid timeframe 'BAD_TF'`

#### Correct Formats:

```epochscript
# Correct:
daily = ema(period=50, timeframe="1D")(src.c)
hourly = ema(period=20, timeframe="1H")(src.c)
minute = ema(period=10, timeframe="1Min")(src.c)
```

### Valid Session Names

Session-based transforms use predefined session names.

#### Valid Sessions:

**Geographic:**
- `"Sydney"`, `"Tokyo"`, `"London"`, `"NewYork"`

**Kill Zones (ICT):**
- `"AsianKillZone"`
- `"LondonOpenKillZone"`
- `"NewYorkKillZone"`
- `"LondonCloseKillZone"`

#### Wrong Sessions:

```epochscript
# Incorrect: - These don't exist
london = sessions(session_type="LondonKillZone")()  # Doesn't exist
ny = sessions(session_type="NY")()                   # Use "NewYork"
```

**Error message:** `Invalid session 'BAD_SESSION'`

#### Correct Sessions:

```epochscript
# Correct:
london = sessions(session_type="London")()
london_open_kz = sessions(session_type="LondonOpenKillZone")()
ny = sessions(session_type="NewYork")()
```

### Session-Based Transforms Require Intraday Data

Transforms like `sessions()`, `bar_gap()`, and `session_gap()` only work on intraday timeframes.

**Why?** Sessions are meaningful only for intraday data (London session, NY session, etc.). Daily bars don't have session boundaries.

#### Wrong: Sessions on Daily Data

```epochscript
# Incorrect: - If strategy runs on daily (1D) timeframe
london = sessions(session_type="London")()
```

**Error message:** `Transform 'sessions' requires intraday data`

#### Correct: Sessions on Intraday Data

```epochscript
# Correct: - Strategy configured for intraday (1Min, 5Min, 1H, etc.)
london = sessions(session_type="London")()
breakout = src.c > london.high
```

**Configuration:** Ensure your strategy/backtest is configured to run on intraday timeframe.

---

## Common Design Mistakes

### 1. Trying to Filter Data with If Statements

**Wrong approach:**
```epochscript
# Users often try this (Python thinking)
if london_session:
    signal = ema_cross
```

**Correct approach:**
```epochscript
# Use boolean logic
london = sessions(session_type="London")()
signal = ema_cross and london.active
```

### 2. Calling Output Attributes as Functions

**Wrong:**
```epochscript
# Trying to call output handle
bb = bbands(period=20, stddev=2)(src.c)
upper_val = bb.bbands_upper()  # Don't call it!
```

**Error message:** `Cannot call attribute as function`

**Correct:**
```epochscript
# Access attribute directly
bb = bbands(period=20, stddev=2)(src.c)
upper_val = bb.bbands_upper
```

### 3. Using Transform Name as Variable

**Confusing but allowed:**
```epochscript
# Note: Legal but confusing
ema = market_data_source()
ema_20 = ema(period=20)(ema.c)  # 'ema' is the source, not the transform!
```

**Better:**
```epochscript
# Clear naming
src = market_data_source()
ema_20 = ema(period=20)(src.c)
```

### 4. Forgetting Data Source

**Wrong:**
```epochscript
# Where does 'c' come from?
ema_20 = ema(period=20)(c)
```

**Error message:** `Undefined variable 'c'`

**Correct:**
```epochscript
# Explicitly fetch data source
src = market_data_source()
ema_20 = ema(period=20)(src.c)
```

### 5. Inconsistent Variable Naming in Unpacking

**Confusing:**
```epochscript
# Note: Legal but confusing naming
x, y, z = bbands(period=20, stddev=2)(src.c)
# Which is upper? Which is lower?
```

**Better:**
```epochscript
# Descriptive names
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
# Or:
bb_lower, bb_middle, bb_upper = bbands(period=20, stddev=2)(src.c)
```

---

## Summary

**Key Principles:**

1. **One executor per strategy** - Required design pattern
2. **No Python control flow** - Use boolean logic and ternary operators
3. **Immutable variables** - Single assignment only
4. **Explicit multi-output access** - Unpack or use attribute access
5. **Strict format strings** - Timeframes and sessions have exact formats
6. **Intraday-only transforms** - Some transforms require intraday data

**When in doubt:**
- Check [Error Reference](./error-reference.md) for specific error messages
- See [Language Fundamentals](./language/index.md) for detailed syntax rules
- Review [Strategy Patterns](../strategies/patterns/index.md) for working examples

---

**Related Documentation:**
- [Language Fundamentals](./language/index.md) - Syntax and types
- [Error Reference](./error-reference.md) - Troubleshooting specific errors
- [Core Concepts](./concepts/index.md) - Timeframes, sessions, cross-sectional
- [Strategy Patterns](../strategies/patterns/index.md) - Complete strategy examples
