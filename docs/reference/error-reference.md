# Error Reference

Troubleshooting guide with common errors and solutions.

---

## Table of Contents

1. [Syntax Errors](#syntax-errors)
2. [Type Errors](#type-errors)
3. [Transform Errors](#transform-errors)
4. [Lag Operator Errors](#lag-operator-errors)
5. [Multi-Output Errors](#multi-output-errors)
6. [Execution Errors](#execution-errors)
7. [Reporting Errors](#reporting-errors)
8. [Quick Troubleshooting](#quick-troubleshooting)

---

## Syntax Errors

### Variable Already Bound

**Error:** `Variable 'price' already bound`

**Cause:** Attempting to reassign a variable

```python
# ❌ WRONG
price = src.c
price = src.o  # ERROR: Cannot reassign

# ✅ FIX
close_price = src.c
open_price = src.o
```

### Undefined Variable

**Error:** `Undefined variable 'x'`

**Cause:** Using variable before definition

```python
# ❌ WRONG
signal = ema_val > 100
ema_val = ema(period=20)(src.c)  # Defined after use

# ✅ FIX
ema_val = ema(period=20)(src.c)
signal = ema_val > 100
```

### Chained Comparison Not Supported

**Error:** `Chained comparison not supported`

**Cause:** Using syntax like `a < b < c`

```python
# ❌ WRONG
valid = 30 < rsi_val < 70

# ✅ FIX
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70
```

### Control Flow Not Allowed

**Error:** `If statements not supported`

**Cause:** Using `if`, `for`, `while`, `def`, `class`

```python
# ❌ WRONG
if price > 100:
    signal = True
else:
    signal = False

# ✅ FIX
signal = price > 100
# Or: signal = True if price > 100 else False
```

---

## Type Errors

### Incompatible Type Cast

**Error:** `incompatible_type_cast`

**Cause:** Trying to mix incompatible types (e.g., String + Number)

```python
# ❌ WRONG
label = "PRICE"
result = label + 100  # Cannot add string to number

# ✅ FIX
value = 100
result = value + 100
```

### Type Mismatch for Option

**Error:** `Type mismatch for option 'period': expected Integer, got String`

**Cause:** Wrong type for transform parameter

```python
# ❌ WRONG
ema_val = ema(period="20")(src.c)  # String instead of Integer

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

### Boolean Operation on Non-Boolean

**Error:** `Expected Boolean type for logical operation`

**Cause:** Using `and`/`or`/`not` with non-boolean values incorrectly

```python
# ❌ WRONG (if not auto-cast)
value = 100
result = value and True  # May fail if no auto-cast

# ✅ FIX
is_positive = value > 0
result = is_positive and True
```

---

## Transform Errors

### Missing Required Option

**Error:** `Missing required option 'period'`

**Cause:** Transform requires an option that wasn't provided

```python
# ❌ WRONG
ema_val = ema()(src.c)  # Missing period

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

### Unknown Option

**Error:** `Unknown option 'periood'`

**Cause:** Typo in option name

```python
# ❌ WRONG
ema_val = ema(periood=20)(src.c)  # Typo

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

### Unknown Handle

**Error:** `Unknown handle 'result' for transform 'market_data_source'`

**Cause:** Accessing wrong output name

```python
# ❌ WRONG
src = market_data_source()
price = src.result  # market_data_source has no 'result'

# ✅ FIX
price = src.c  # Use correct handle: c, h, l, o, v
```

### Transform Not Found

**Error:** `Unknown transform 'ema_20'`

**Cause:** Typo in transform name or transform doesn't exist

```python
# ❌ WRONG
ema_val = ema_20()(src.c)  # No such transform

# ✅ FIX
ema_val = ema(period=20)(src.c)
```

### Wrong Input Count

**Error:** `Expected 2 inputs, got 1`

**Cause:** Transform requires specific number of inputs

```python
# ❌ WRONG
cross = crossover()(fast)  # Needs 2 inputs

# ✅ FIX
cross = crossover()(fast, slow)
```

### Wrong Input Type

**Error:** `Expected Decimal input, got Boolean`

**Cause:** Passing wrong type to transform input

```python
# ❌ WRONG
ema_val = ema(period=20)(is_bullish)  # Boolean, not Decimal

# ✅ FIX
ema_val = ema(period=20)(src.c)  # Decimal
```

---

## Lag Operator Errors

### Invalid Lag Zero

**Error:** `invalid_lag_zero`

**Cause:** Using `[0]` as lag index

```python
# ❌ WRONG
current = src.c[0]

# ✅ FIX
current = src.c  # Omit index for current value
```

### Invalid Lag Float

**Error:** `invalid_lag_float`

**Cause:** Using float as lag index

```python
# ❌ WRONG
lag = src.c[1.5]

# ✅ FIX
lag = src.c[2]  # Use integer
```

### Negative Lag Not Allowed

**Error:** `Negative lag indices not supported`

**Cause:** Using negative index (forward-looking)

```python
# ❌ WRONG
future = src.c[-1]

# ✅ FIX (for research only)
# Use forward_returns transform
future_ret = forward_returns(period=1)(src.c)
```

---

## Multi-Output Errors

### Ambiguous Multi-Output

**Error:** `ambiguous_multi_output`

**Cause:** Using multi-output transform in expression without specifying which output

```python
# ❌ WRONG
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # Which output? (lower, middle, or upper?)

# ✅ FIX - Option 1: Access specific handle
bb = bbands(period=20, stddev=2)(src.c)
signal = bb.bbands_upper > 100

# ✅ FIX - Option 2: Tuple unpacking
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
signal = upper > 100
```

### Tuple Unpacking Count Mismatch

**Error:** `Tuple unpacking count mismatch: expected 3, got 2`

**Cause:** Number of variables doesn't match number of outputs

```python
# ❌ WRONG
lower, upper = bbands(period=20, stddev=2)(src.c)  # 3 outputs, 2 variables

# ✅ FIX - Option 1: Match count
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# ✅ FIX - Option 2: Discard with underscore
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

### Right-Hand Side Must Be Constructor Call

**Error:** `Right-hand side of tuple unpacking must be a constructor call`

**Cause:** Trying to unpack something that isn't a transform call

```python
# ❌ WRONG
bb = bbands(period=20, stddev=2)(src.c)
lower, middle, upper = bb  # Cannot unpack after assignment

# ✅ FIX
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
```

### Calling Attribute as Function

**Error:** `Cannot call attribute as function`

**Cause:** Trying to call an output handle

```python
# ❌ WRONG
bb_upper = bbands(period=20, stddev=2).bbands_upper(src.c)

# ✅ FIX
bb = bbands(period=20, stddev=2)(src.c)
bb_upper = bb.bbands_upper  # Access, don't call
```

---

## Execution Errors

### No Trade Signal Executor Found

**Error:** `No trade_signal_executor found in strategy`

**Cause:** Trading strategy missing required executor

```python
# ❌ WRONG
src = market_data_source()
signal = src.c > src.c[1]
# Missing executor!

# ✅ FIX
src = market_data_source()
signal = src.c > src.c[1]
trade_signal_executor()(enter_long=signal)
```

### Multiple Trade Signal Executors

**Error:** `Multiple trade_signal_executor calls found`

**Cause:** More than one executor in strategy

```python
# ❌ WRONG
trade_signal_executor()(enter_long=buy)
trade_signal_executor()(enter_short=sell)  # Second executor

# ✅ FIX
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### Missing Required Executor Input

**Error:** `trade_signal_executor requires at least one input`

**Cause:** Executor called with no signals

```python
# ❌ WRONG
trade_signal_executor()()  # No inputs

# ✅ FIX
trade_signal_executor()(enter_long=signal)
```

---

## TimeFrame/Session Errors

### Invalid TimeFrame

**Error:** `Invalid timeframe 'BAD_TF'`

**Cause:** Using unsupported timeframe string

```python
# ❌ WRONG
daily = ema(period=50, timeframe="BAD_TF")(src.c)
daily = ema(period=50, timeframe="1Day")(src.c)  # Wrong format

# ✅ FIX
daily = ema(period=50, timeframe="1D")(src.c)
hourly = ema(period=20, timeframe="1H")(src.c)
```

**Valid formats:** `"1Min"`, `"5Min"`, `"15Min"`, `"1H"`, `"4H"`, `"1D"`, `"1W"`, `"1M"`

### Invalid Session

**Error:** `Invalid session 'BAD_SESSION'`

**Cause:** Using unsupported session name

```python
# ❌ WRONG
london_ema = ema(period=20, session="BAD_SESSION")(src.c)
london_ema = ema(period=20, session="LondonKillZone")(src.c)  # Doesn't exist

# ✅ FIX
london_ema = ema(period=20, session="London")(src.c)
lkz_ema = ema(period=20, session="LondonOpenKillZone")(src.c)
```

**Valid sessions:** `"Sydney"`, `"Tokyo"`, `"London"`, `"NewYork"`, `"AsianKillZone"`, `"LondonOpenKillZone"`, `"NewYorkKillZone"`, `"LondonCloseKillZone"`

### Intraday-Only Transform on Daily Data

**Error:** `Transform 'sessions' requires intraday data`

**Cause:** Using session-based transform on daily or higher timeframe

```python
# ❌ WRONG (if running on daily data)
london = sessions(session_type="London")()

# ✅ FIX
# Use intraday timeframe (1Min, 5Min, 15Min, etc.)
# Configure strategy to run on intraday data
```

---

## Reporting Errors

### event_marker: Missing color_map

**Error:** `Missing required option 'color_map'`

**Cause:** event_marker requires color_map option

```python
# ❌ WRONG
event_marker()(
    oversold=rsi_val < 30
)

# ✅ FIX
event_marker(color_map={
    Success: ["oversold"]
})(
    oversold=rsi_val < 30
)
```

### event_marker: Invalid Color Category

**Error:** `Unknown color category 'Green'`

**Cause:** Using wrong color name (must be Success, Warning, Error, or Info)

```python
# ❌ WRONG
event_marker(color_map={
    Green: ["oversold"],     # Wrong: 'Green' not valid
    Red: ["overbought"]      # Wrong: 'Red' not valid
})(
    oversold=oversold,
    overbought=overbought
)

# ✅ FIX
event_marker(color_map={
    Success: ["oversold"],   # Green color
    Error: ["overbought"]    # Red color
})(
    oversold=oversold,
    overbought=overbought
)
```

**Valid color categories:** `Success` (green), `Warning` (yellow), `Error` (red), `Info` (blue)

### event_marker: Event Name Not in Inputs

**Error:** `Event 'breakout' referenced in color_map but not provided as input`

**Cause:** Event name in color_map doesn't match any input

```python
# ❌ WRONG
event_marker(color_map={
    Success: ["breakout"]  # 'breakout' not in inputs
})(
    upper_break=src.c > upper  # Named 'upper_break', not 'breakout'
)

# ✅ FIX - Option 1: Match input name
event_marker(color_map={
    Success: ["upper_break"]  # Matches input name
})(
    upper_break=src.c > upper
)

# ✅ FIX - Option 2: Rename input
event_marker(color_map={
    Success: ["breakout"]
})(
    breakout=src.c > upper  # Renamed to match
)
```

### event_marker: Non-Boolean Input

**Error:** `Expected Boolean input for 'price', got Decimal`

**Cause:** event_marker inputs must be boolean series

```python
# ❌ WRONG
event_marker(color_map={
    Success: ["price"]
})(
    price=src.c  # Decimal, not Boolean
)

# ✅ FIX
event_marker(color_map={
    Success: ["high_price"]
})(
    high_price=src.c > 100  # Boolean condition
)
```

### gap_report: Missing Required Inputs

**Error:** `gap_report requires inputs: gap_filled, gap_retrace, gap_size, psc, psc_timestamp`

**Cause:** gap_report needs specific outputs from session_gap or bar_gap

```python
# ❌ WRONG
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report()(gaps.gap_filled)  # Missing other required inputs

# ✅ FIX
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

### table_report: SQL Syntax Error

**Error:** `SQL syntax error: unexpected token 'FORM'`

**Cause:** Typo in SQL query

```python
# ❌ WRONG
table_report(sql="""
    SELECT AVG(returns)
    FORM input  -- Typo: FORM instead of FROM
""")(returns=ret)

# ✅ FIX
table_report(sql="""
    SELECT AVG(returns)
    FROM input
""")(returns=ret)
```

### table_report: Column Not Found

**Error:** `Column 'return' not found in input`

**Cause:** SQL references column not provided in inputs

```python
# ❌ WRONG
table_report(sql="""
    SELECT AVG(return) FROM input  -- 'return' not in inputs
""")(returns=ret)  # Named 'returns', not 'return'

# ✅ FIX
table_report(sql="""
    SELECT AVG(returns) FROM input  -- Matches input name
""")(returns=ret)
```

---

## Quick Troubleshooting

### Compilation Errors

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `Variable 'x' already bound` | Reassignment | Use different variable name |
| `Undefined variable 'x'` | Used before definition | Move definition above usage |
| `Chained comparison` | `a < b < c` syntax | Break into `(a < b) and (b < c)` |
| `If statements not supported` | Control flow | Use ternary or `conditional_select` |
| `Unknown option 'x'` | Typo in option name | Check spelling |
| `Missing required option 'x'` | Required parameter missing | Add the option |

### Runtime Errors

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `incompatible_type_cast` | String + Number | Check types |
| `Unknown handle 'x'` | Wrong output name | Check transform outputs |
| `invalid_lag_zero` | Using `[0]` | Omit index |
| `invalid_lag_float` | Using `[1.5]` | Use integer |
| `ambiguous_multi_output` | Multi-output in expression | Access specific handle |
| `Tuple unpacking count mismatch` | Wrong number of variables | Match output count |

### Strategy Errors

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `No trade_signal_executor` | Missing executor | Add `trade_signal_executor()` |
| `Multiple executors` | Two `trade_signal_executor` calls | Combine into one |
| `Invalid timeframe` | Wrong format | Use `"1D"`, `"1H"`, etc. |
| `Invalid session` | Wrong name | Use valid session names |

---

## Debugging Steps

### 1. Read the Error Message

Error messages tell you:
- **What** went wrong
- **Where** it occurred (line number)
- **Why** it failed

### 2. Check Variable Names

```python
# Common typos
ema_val = ema(period=20)(src.c)
signal = emv_val > 100  # Typo: emv_val vs ema_val
```

### 3. Verify Transform Names and Options

```python
# Check spelling
rsi_val = rsi(period=14)(src.c)  # ✓
rsi_val = rsi(periood=14)(src.c)  # ✗ Typo

# Check required options
ema_val = ema()(src.c)  # ✗ Missing period
ema_val = ema(period=20)(src.c)  # ✓
```

### 4. Check Output Handles

```python
# Wrong handle
src = market_data_source()
price = src.result  # ✗ No 'result' handle

# Correct handles
price = src.c  # ✓ Close
high = src.h   # ✓ High
```

### 5. Verify Input/Output Counts

```python
# Crossover needs 2 inputs
cross = crossover()(fast)  # ✗ Only 1 input
cross = crossover()(fast, slow)  # ✓

# Bollinger Bands returns 3 outputs
lower, upper = bbands(period=20, stddev=2)(src.c)  # ✗ Only 2 variables
lower, middle, upper = bbands(period=20, stddev=2)(src.c)  # ✓
```

### 6. Check Data Types

```python
# Wrong type
ema_val = ema(period="20")(src.c)  # ✗ String
ema_val = ema(period=20)(src.c)    # ✓ Integer

# Type compatibility
label = "PRICE"
result = label + 100  # ✗ String + Number
```

---

## Common Patterns for Fixes

### Pattern 1: Variable Reassignment

```python
# Problem
x = src.c
x = src.o  # ERROR

# Solution
close = src.c
open = src.o
```

### Pattern 2: Chained Comparison

```python
# Problem
valid = 30 < rsi < 70  # ERROR

# Solution
valid = (rsi > 30) and (rsi < 70)
```

### Pattern 3: Multi-Output Access

```python
# Problem
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # ERROR

# Solution 1: Specific handle
signal = bb.bbands_upper > 100

# Solution 2: Tuple unpacking
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
signal = upper > 100
```

### Pattern 4: Missing Executor

```python
# Problem
src = market_data_source()
signal = src.c > 100  # ERROR: No executor

# Solution
src = market_data_source()
signal = src.c > 100
trade_signal_executor()(enter_long=signal)
```

### Pattern 5: Conditional Logic

```python
# Problem
if condition:  # ERROR
    x = 1

# Solution 1: Ternary
x = 1 if condition else 0

# Solution 2: Direct boolean
x = condition  # If you just need True/False
```

---

## Getting More Help

If error persists:

1. **Check transform catalog**: Verify transform name, options, inputs, outputs
2. **Review examples**: See working examples in [Strategy Patterns](../strategies/strategy-patterns.md)
3. **Simplify**: Remove parts until error disappears, then add back
4. **Test incrementally**: Build up strategy step-by-step

---

**Next:** [Appendix: Full Catalog →](appendix-full-catalog.md)
