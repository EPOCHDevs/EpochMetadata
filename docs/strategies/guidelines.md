---
page_type: implementation
layout: default
order: 3
category: Strategies
description: Common pitfalls to avoid and best practices for writing maintainable, robust EpochScript code
parent: ./index.md
---

# Guidelines & Best Practices

Critical pitfalls to avoid and best practices for writing maintainable, robust EpochScript strategies.

---

## Session Filtering Best Practices

:::note
**Compiler Optimizations:** The EpochScript compiler automatically handles most performance optimizations (Common Subexpression Elimination, transform deduplication, lag caching). Focus on writing clear, readable code.
:::

### Session Filtering Pattern

Only apply session filters when necessary, and apply them to signals rather than every indicator.

```epochscript
# Get session data
london = sessions(session_type="London")()
in_london = london.active

# Calculate indicators normally (once)
rsi_val = rsi(period=14)(src.c)

# Apply session filter to final signal
signal = (rsi_val < 30) and in_london
```

:::tip
Session filtering at the signal level is more efficient than applying session parameters to every transform.
:::

---

## Common Pitfalls

Critical mistakes to avoid when writing EpochScript strategies.

### 1. Look-Ahead Bias

:::warning
**Critical Error:** Using future data in trading strategies creates unrealistic backtest results.
:::

**Wrong (uses future data):**

```epochscript
# Negative lag indices access future data
future_return = (src.c[-5] - src.c) / src.c
entry = future_return > 0.02  # Cheating!
```

**Correct (uses only past/current data):**

```epochscript
# Positive lag indices access historical data
momentum = (src.c - src.c[10]) / src.c[10]
entry = momentum > 0.02
```

:::note
**Exception:** Look-ahead is acceptable in research scripts for creating target variables or labels.
:::

### 2. Overfitting Parameters

Hyper-optimized parameters rarely generalize to live trading.

**Wrong (overfit):**

```epochscript
# Suspiciously precise parameters
ema_fast = ema(period=17)(src.c)  # Why exactly 17?
ema_slow = ema(period=43)(src.c)  # Why exactly 43?
rsi_threshold = 28.3              # Why 28.3?
```

**Correct (standard parameters):**

```epochscript
# Round, widely-used parameters
ema_fast = ema(period=20)(src.c)
ema_slow = ema(period=50)(src.c)
rsi_threshold = 30
```

:::tip
Use standard parameter values (20, 50, 200 for EMAs; 14 for RSI; 30/70 for thresholds). If optimization finds unusual values, be skeptical.
:::

### 3. Ignoring Transaction Costs

High-frequency signals generate excessive trading costs.

**Problem:**

```epochscript
# Very fast EMAs create frequent crossovers
fast = ema(period=5)(src.c)
slow = ema(period=10)(src.c)
entry = crossover(fast, slow)  # Too many trades!
```

**Solution:**

```epochscript
# Add confirmation filter to reduce trade frequency
fast = ema(period=5)(src.c)
slow = ema(period=10)(src.c)
cross = crossover(fast, slow)

# Require volume confirmation
vol_confirm = src.v > sma(period=20)(src.v) * 1.5
entry = cross and vol_confirm  # Fewer, higher-quality trades
```

### 4. Not Accounting for Regime Changes

Static strategies fail when market conditions change.

**Fragile (same logic always):**

```epochscript
# Always uses RSI < 30 regardless of market state
entry = rsi(period=14)(src.c) < 30
```

**Robust (adapts to regime):**

```epochscript
# Detect trending vs ranging market
adx_val = adx(period=14)(src.h, src.l, src.c)
trending = adx_val > 25

rsi_val = rsi(period=14)(src.c)

# Adjust thresholds by regime
entry = conditional_select(
    trending, rsi_val < 40,   # Trending: less extreme threshold
    rsi_val < 30              # Ranging: more extreme threshold
)
```

:::note
Markets alternate between trending and ranging states. Adapt your strategy logic accordingly.
:::

### 5. False Precision

Excessive decimal places suggest false precision and overfitting.

**Wrong:**

```epochscript
threshold = 0.0237894   # Meaningless precision
multiplier = 1.4732891  # Likely overfit
```

**Correct:**

```epochscript
threshold = 0.024   # or 2.4% - readable and reasonable
multiplier = 1.5    # Round number
```

---

## Best Practices

Coding standards for maintainable, robust strategies.

### 1. Start Simple, Add Complexity Gradually

:::tip
Only add complexity if it measurably improves performance. More complexity ≠ better results.
:::

```epochscript
# Phase 1: Simple MA crossover
entry_v1 = crossover(ema(period=20)(src.c), ema(period=50)(src.c))

# Phase 2: Add volume filter (test if this improves results)
entry_v2 = entry_v1 and (src.v > sma(period=20)(src.v))

# Phase 3: Add higher timeframe trend filter (test again)
daily_trend = ema(period=50, timeframe="1D")(src.c)
entry_v3 = entry_v2 and (src.c > daily_trend)

# Only use v3 if it outperforms v2, and v2 outperforms v1
```

### 2. Use Meaningful Variable Names

**Bad:**

```epochscript
src = market_data_source()
x1 = ema(period=20)(src.c)
x2 = ema(period=50)(src.c)
s = x1 > x2
```

**Good:**

```epochscript
src = market_data_source()
fast_ema = ema(period=20)(src.c)
slow_ema = ema(period=50)(src.c)
bullish_trend = fast_ema > slow_ema
```

### 3. Comment Complex Logic

Add comments explaining *why*, not just *what*.

```epochscript
# Calculate session-specific volatility
# Rationale: London session typically has 2x volatility of Asian session
london = sessions(session_type="London")()
asian = sessions(session_type="Tokyo")()

london_atr = atr(period=14, session="London")(src.h, src.l, src.c)
asian_atr = atr(period=14, session="Tokyo")(src.h, src.l, src.c)

# Use appropriate volatility for current session
current_atr = conditional_select(
    london.active, london_atr,
    asian.active, asian_atr,
    atr(period=14)(src.h, src.l, src.c)  # Default for other sessions
)
```

### 4. Group Related Calculations

Organize code into logical sections.

```epochscript
# ============ DATA SOURCE ============
src = market_data_source()

# ============ INDICATORS ============
fast_ema = ema(period=20)(src.c)
slow_ema = ema(period=50)(src.c)
rsi_val = rsi(period=14)(src.c)
atr_val = atr(period=14)(src.h, src.l, src.c)

# ============ FILTERS ============
trend_up = fast_ema > slow_ema
not_overbought = rsi_val < 70
vol_expanding = atr_val > atr_val[5]

# ============ SIGNALS ============
entry = trend_up and not_overbought and vol_expanding

# ============ EXECUTION ============
trade_signal_executor()(enter_long=entry)
```

### 5. Test Edge Cases

Consider how your strategy handles unusual conditions.

**Edge cases to test:**
- Missing data (null values from indicators)
- Extreme values (50% price spikes)
- Zero volume
- Large gaps
- Sudden volatility changes

```epochscript
# Handle missing data with defaults
rsi_val = rsi(period=14)(src.c)
safe_rsi = first_non_null(rsi_val, 50.0)  # Default to neutral

# Cap extreme returns
returns = (src.c - src.c[1]) / src.c[1]
capped_returns = conditional_select(
    returns > 0.1, 0.1,      # Cap at +10%
    returns < -0.1, -0.1,    # Floor at -10%
    returns                   # Otherwise use actual
)
```

### 6. Document Assumptions

Make explicit the conditions under which your strategy is designed to work.

```epochscript
# ==================== ASSUMPTIONS ====================
# - Instrument: SPY (S&P 500 ETF)
# - Timeframe: 1-hour bars
# - Trading hours: US market hours only (09:30-16:00 ET)
# - Market regime: Normal volatility (VIX < 30)
# - Gap threshold: 1.5% is considered significant for SPY
# ====================================================

# Track S&P 500 index (SPX) for market context
spx = indices(ticker="SPX")()

src = market_data_source()
gaps = session_gap(fill_percent=100, min_gap_size=1.5, timeframe="1H")()
```

### 7. Keep It Maintainable

Favor readability over brevity.

**Good (clear logic flow):**

```epochscript
# Step 1: Identify trend
trend = ema(period=50)(src.c)
uptrend = src.c > trend

# Step 2: Find entry point
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30

# Step 3: Confirm with volume
avg_vol = sma(period=20)(src.v)
high_volume = src.v > (avg_vol * 1.5)

# Step 4: Combine all conditions
entry = uptrend and oversold and high_volume
```

**Bad (unreadable one-liner):**

```epochscript
entry = (src.c > ema(period=50)(src.c)) and (rsi(period=14)(src.c) < 30) and (src.v > (sma(period=20)(src.v) * 1.5))
```

:::tip
If you can't understand your code after a week away, it needs refactoring.
:::

---

## Summary

### Session Filtering
- Apply session filters to signals, not every indicator
- Let users control whether they want session-specific indicators

### Critical Pitfalls
1. **Look-ahead bias** - Never use future data (negative lags)
2. **Overfitting** - Stick to standard, round parameters
3. **Transaction costs** - Filter signals to reduce trade frequency
4. **Regime blindness** - Adapt logic to market conditions
5. **False precision** - Use 2-3 significant digits

### Best Practices
1. **Incremental complexity** - Start simple, validate each addition
2. **Readable names** - `bullish_trend` not `s`
3. **Explain complex logic** - Comment the "why"
4. **Group sections** - Data, indicators, filters, signals, execution
5. **Handle edge cases** - Missing data, extremes, gaps
6. **Document assumptions** - Instrument, timeframe, regime
7. **Favor clarity** - Break complex expressions into steps

---

**See Also:**
- [Advanced Techniques](./techniques.md) - Conditional logic, dynamic parameters, signal scoring
- [Error Reference](../error-reference.md) - Common errors and solutions
- [Design Guidelines](../design-guidelines.md) - Required patterns and common mistakes
