# Advanced Topics

Advanced techniques, optimization, and best practices.

---

## Table of Contents

1. [Complex Conditional Logic](#complex-conditional-logic)
2. [Dynamic Parameter Selection](#dynamic-parameter-selection)
3. [Multi-Condition Signal Scoring](#multi-condition-signal-scoring)
4. [Performance Optimization](#performance-optimization)
5. [Common Pitfalls](#common-pitfalls)
6. [Best Practices](#best-practices)

---

## Complex Conditional Logic

### Nested Ternary (Avoid When Possible)

```python
# Works but hard to read
label = (
    "STRONG_BUY" if rsi < 20 else (
    "BUY" if rsi < 30 else (
    "NEUTRAL" if rsi < 70 else (
    "SELL" if rsi < 80 else "STRONG_SELL"
    ))))
```

### Better: Use conditional_select

```python
# Much clearer
label = conditional_select(
    rsi < 20, "STRONG_BUY",
    rsi < 30, "BUY",
    rsi < 70, "NEUTRAL",
    rsi < 80, "SELL",
    "STRONG_SELL"
)
```

### Multi-Level Filters

```python
src = market_data_source()

# Level 1: Trend direction
ema_fast = ema(period=20)(src.c)
ema_slow = ema(period=50)(src.c)
trend_up = ema_fast > ema_slow

# Level 2: Momentum confirmation
rsi_val = rsi(period=14)(src.c)
momentum_ok = rsi_val < 70

# Level 3: Volatility filter
atr_val = atr(period=14)(src.h, src.l, src.c)
vol_expanding = atr_val > atr_val[5]

# Level 4: Volume confirmation
avg_vol = sma(period=20)(src.v)
volume_ok = src.v > (avg_vol * 1.2)

# Combine all levels
strong_signal = trend_up and momentum_ok and vol_expanding and volume_ok
weak_signal = trend_up and momentum_ok
no_signal = not trend_up

# Position sizing based on signal strength
position_size = conditional_select(
    strong_signal, 3,  # Full size
    weak_signal, 2,    # Half size
    1                  # Minimal/exit
)
```

### Regime-Based Logic Switching

```python
src = market_data_source()

# Detect regime
adx_val = adx(period=14)(src.h, src.l, src.c)
vol = volatility(period=20)(src.c)

# Regime classification
trending_low_vol = (adx_val > 25) and (vol < 15)
trending_high_vol = (adx_val > 25) and (vol >= 15)
ranging_low_vol = (adx_val < 20) and (vol < 15)
ranging_high_vol = (adx_val < 20) and (vol >= 15)

# Different strategies per regime
trend_signal = ema(period=20)(src.c) > ema(period=50)(src.c)
mean_revert_signal = rsi(period=14)(src.c) < 30

# Route to appropriate strategy
entry = conditional_select(
    trending_low_vol, trend_signal,      # Trend-follow in clean trends
    trending_high_vol, False,            # Stay out (too volatile)
    ranging_low_vol, mean_revert_signal, # Mean-revert in ranges
    ranging_high_vol, False,             # Stay out
    False                                # Default: no trade
)
```

---

## Dynamic Parameter Selection

### Adaptive Periods Based on Volatility

```python
src = market_data_source()

# Measure volatility
vol = volatility(period=20)(src.c)

# Adjust EMA period based on vol
# Low vol (< 10%): Use short period (fast adaptation)
# High vol (> 30%): Use long period (smooth noise)
ema_period = conditional_select(
    vol < 10, 12,   # Low vol: fast
    vol < 20, 20,   # Medium vol: standard
    vol < 30, 26,   # Higher vol: slower
    50              # High vol: very slow
)

# Note: ema_period is a value, not usable directly in ema()
# This demonstrates concept; actual implementation needs different approach
```

**Limitation:** Transform options must be literals, not variables.

**Workaround:** Pre-calculate multiple indicators, then select

```python
# Calculate multiple EMAs
ema_12 = ema(period=12)(src.c)
ema_20 = ema(period=20)(src.c)
ema_26 = ema(period=26)(src.c)
ema_50 = ema(period=50)(src.c)

# Select based on volatility
vol = volatility(period=20)(src.c)
adaptive_ema = conditional_select(
    vol < 10, ema_12,
    vol < 20, ema_20,
    vol < 30, ema_26,
    ema_50
)

# Use adaptive EMA
signal = src.c > adaptive_ema
```

### State-Dependent Exits

```python
src = market_data_source()

# Entry signal
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
entry = crossover(fast, slow)

# Multiple exit conditions
profit_target_hit = src.c > (entry_price * 1.05)  # 5% profit
stop_loss_hit = src.c < (entry_price * 0.98)      # 2% stop
trend_reversal = crossunder(fast, slow)
rsi_overbought = rsi(period=14)(src.c) > 80

# Exit on any condition
exit = profit_target_hit or stop_loss_hit or trend_reversal or rsi_overbought
```

**Note:** `entry_price` tracking requires external state management (not directly supported in DSL).

**Workaround:** Use percentage-based conditions relative to current price

```python
# Instead of tracking entry price
# Use current price vs moving average as proxy
ema_entry = ema(period=50)(src.c)  # Approximate entry level
profit_condition = src.c > (ema_entry * 1.05)
```

---

## Multi-Condition Signal Scoring

### Weighted Scoring System

```python
src = market_data_source()

# Individual signals (0 or 1)
trend_up = ema(period=20)(src.c) > ema(period=50)(src.c)
rsi_ok = rsi(period=14)(src.c) < 70
macd_bullish = macd(fast=12, slow=26, signal=9)(src.c)[0] > macd(fast=12, slow=26, signal=9)(src.c)[1]
volume_high = src.v > sma(period=20)(src.v) * 1.2

# Convert booleans to numbers (True = 1, False = 0)
trend_score = conditional_select(trend_up, 1, 0)
rsi_score = conditional_select(rsi_ok, 1, 0)
macd_score = conditional_select(macd_bullish, 1, 0)
volume_score = conditional_select(volume_high, 1, 0)

# Weighted sum (weights: trend=3, rsi=2, macd=2, volume=1)
total_score = (trend_score * 3) + (rsi_score * 2) + (macd_score * 2) + (volume_score * 1)
max_score = 8

# Position sizing by score
position_size = conditional_select(
    total_score >= 7, 3,  # Strong signal (7-8 points)
    total_score >= 5, 2,  # Medium signal (5-6 points)
    total_score >= 3, 1,  # Weak signal (3-4 points)
    0                     # No signal (0-2 points)
)

# Entry threshold
entry = total_score >= 5
```

### Confidence-Based Execution

```python
src = market_data_source()

# Signal components
technical_bullish = ema(period=20)(src.c) > ema(period=50)(src.c)
momentum_bullish = rsi(period=14)(src.c) < 60
volume_confirming = src.v > sma(period=20)(src.v)

# Confidence level
confidence_high = technical_bullish and momentum_bullish and volume_confirming
confidence_medium = technical_bullish and (momentum_bullish or volume_confirming)
confidence_low = technical_bullish

# Different thresholds by confidence
entry = conditional_select(
    confidence_high, True,      # Always enter
    confidence_medium, True,    # Enter with caution
    confidence_low, False,      # Skip
    False                       # Default
)

# Exits also depend on confidence
# High confidence: wider stops
# Low confidence: tighter stops
```

---

## Performance Optimization

### Avoid Redundant Calculations

**Bad:**
```python
# Calculating EMA multiple times
signal1 = ema(period=20)(src.c) > 100
signal2 = ema(period=20)(src.c) > ema(period=50)(src.c)
signal3 = src.c > ema(period=20)(src.c)
```

**Good:**
```python
# Calculate once, reuse
ema_20 = ema(period=20)(src.c)
ema_50 = ema(period=50)(src.c)

signal1 = ema_20 > 100
signal2 = ema_20 > ema_50
signal3 = src.c > ema_20
```

### Limit Multi-Output Unpacking

**Less efficient:**
```python
# Unpacking creates intermediate variables
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
lower_20, middle_20, upper_20 = bbands(period=20, stddev=2)(src.c)  # Duplicate calculation!
```

**More efficient:**
```python
# Calculate once
bb = bbands(period=20, stddev=2)(src.c)
lower = bb.bbands_lower
middle = bb.bbands_middle
upper = bb.bbands_upper
```

### Minimize Lag Operations

**Less efficient:**
```python
# Multiple lag accesses
mom1 = src.c - src.c[10]
mom2 = src.c[10] - src.c[20]
mom3 = src.c[20] - src.c[30]
```

**More efficient:**
```python
# Store lagged values
c_lag10 = src.c[10]
c_lag20 = src.c[20]
c_lag30 = src.c[30]

mom1 = src.c - c_lag10
mom2 = c_lag10 - c_lag20
mom3 = c_lag20 - c_lag30
```

### Session Filtering (When Needed)

Only use session parameter when actually needed:

```python
# If strategy only trades London session
# Apply session filter to data source, not every indicator
src = market_data_source(session="London")  # NOT DIRECTLY SUPPORTED

# Instead, filter signals
london = sessions(session_type="London")()
in_london = london.active

signal = (rsi(period=14)(src.c) < 30) and in_london
```

---

## Common Pitfalls

### 1. Look-Ahead Bias

**Wrong (in trading strategy):**
```python
# Using future data
future_return = (src.c[-5] - src.c) / src.c
entry = future_return > 0.02  # Cheating!
```

**Correct:**
```python
# Only use past/current data
momentum = (src.c - src.c[10]) / src.c[10]
entry = momentum > 0.02
```

**Exception:** Look-ahead is OK in research scripts for target variables.

### 2. Overfitting Parameters

**Wrong:**
```python
# Hyper-optimized parameters (likely overfit)
ema_fast = ema(period=17)(src.c)  # Why 17?
ema_slow = ema(period=43)(src.c)  # Why 43?
rsi_threshold = 28.3              # Why 28.3?
```

**Correct:**
```python
# Use standard, round parameters
ema_fast = ema(period=20)(src.c)
ema_slow = ema(period=50)(src.c)
rsi_threshold = 30
```

### 3. Ignoring Transaction Costs

**Problem:**
```python
# High-frequency crossovers (lots of trades)
fast = ema(period=5)(src.c)
slow = ema(period=10)(src.c)
entry = crossover(fast, slow)  # Too frequent!
```

**Solution:**
```python
# Add filters to reduce trade frequency
fast = ema(period=5)(src.c)
slow = ema(period=10)(src.c)
cross = crossover(fast, slow)

# Require additional confirmation
vol_confirm = src.v > sma(period=20)(src.v) * 1.5
entry = cross and vol_confirm  # Fewer trades
```

### 4. Not Accounting for Regime Changes

**Fragile:**
```python
# Same strategy always
entry = rsi(period=14)(src.c) < 30
```

**Robust:**
```python
# Adapt to regime
adx_val = adx(period=14)(src.h, src.l, src.c)
trending = adx_val > 25

rsi_val = rsi(period=14)(src.c)

# Different thresholds by regime
entry = conditional_select(
    trending, rsi_val < 40,   # Trend: less extreme
    rsi_val < 30              # Range: more extreme
)
```

### 5. False Precision

**Wrong:**
```python
# Overly precise (meaningless)
threshold = 0.0237894
multiplier = 1.4732891
```

**Correct:**
```python
# Reasonable precision
threshold = 0.024  # or 2.4%
multiplier = 1.5
```

---

## Best Practices

### 1. Start Simple, Add Complexity Gradually

```python
# Phase 1: Simple MA cross
entry_v1 = crossover(ema(period=20)(src.c), ema(period=50)(src.c))

# Phase 2: Add volume
entry_v2 = entry_v1 and (src.v > sma(period=20)(src.v))

# Phase 3: Add trend filter
daily_trend = ema(period=50, timeframe="1D")(src.c)
entry_v3 = entry_v2 and (src.c > daily_trend)

# Only add if each step improves performance
```

### 2. Use Meaningful Variable Names

**Bad:**
```python
x1 = ema(period=20)(src.c)
x2 = ema(period=50)(src.c)
s = x1 > x2
```

**Good:**
```python
fast_ema = ema(period=20)(src.c)
slow_ema = ema(period=50)(src.c)
bullish_trend = fast_ema > slow_ema
```

### 3. Comment Complex Logic

```python
# Calculate session-specific volatility
# Rationale: London session typically more volatile than Asian
london = sessions(session_type="London")()
asian = sessions(session_type="Tokyo")()

london_atr = atr(period=14, session="London")(src.c)
asian_atr = atr(period=14, session="Tokyo")(src.c)

# Use appropriate volatility for current session
current_atr = conditional_select(
    london.active, london_atr,
    asian.active, asian_atr,
    atr(period=14)(src.c)  # Default
)
```

### 4. Group Related Calculations

```python
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

Consider:
- **Missing data**: What if indicators return null?
- **Extreme values**: What if price spikes 50%?
- **Low volume**: What if volume is zero?
- **Gaps**: How does strategy handle gaps?

```python
# Handle missing data
rsi_val = rsi(period=14)(src.c)
safe_rsi = first_non_null(rsi_val, 50.0)  # Default to neutral

# Handle extreme values
returns = (src.c - src.c[1]) / src.c[1]
capped_returns = conditional_select(
    returns > 0.1, 0.1,    # Cap at 10%
    returns < -0.1, -0.1,  # Floor at -10%
    returns
)
```

### 6. Document Assumptions

```python
# ASSUMPTIONS:
# - Data: SPY on 1-hour bars
# - Timeframe: Intraday strategies only (not daily)
# - Session: US market hours (09:30-16:00 ET)
# - Volatility: Typical market conditions (VIX < 30)
# - Gap threshold: 1.5% is significant for SPY

src = market_data_source()
gaps = session_gap(fill_percent=100, min_gap_size=1.5, timeframe="1H")()
```

### 7. Keep It Maintainable

```python
# GOOD: Clear logic flow
# Step 1: Identify trend
trend = ema(period=50)(src.c)
uptrend = src.c > trend

# Step 2: Find entry point
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30

# Step 3: Confirm with volume
avg_vol = sma(period=20)(src.v)
high_volume = src.v > (avg_vol * 1.5)

# Step 4: Combine
entry = uptrend and oversold and high_volume

# BAD: Everything in one line
entry = (src.c > ema(period=50)(src.c)) and (rsi(period=14)(src.c) < 30) and (src.v > (sma(period=20)(src.v) * 1.5))
```

---

## Summary

### Advanced Techniques

1. **Conditional Logic**: Use `conditional_select` for multi-way branching
2. **Dynamic Selection**: Pre-calculate options, select based on conditions
3. **Signal Scoring**: Weight and combine multiple signals
4. **Regime Adaptation**: Different logic for different market states

### Optimization

1. **Cache Calculations**: Store reused values
2. **Avoid Redundancy**: Calculate indicators once
3. **Minimize Lags**: Store lagged values if used multiple times
4. **Group Operations**: Batch related calculations

### Common Mistakes

1. **Look-Ahead Bias**: Don't use future data in strategies
2. **Overfitting**: Use round, standard parameters
3. **Transaction Costs**: Filter signals to reduce trade frequency
4. **False Precision**: 2-3 significant digits sufficient
5. **Regime Blindness**: Adapt strategy to market conditions

### Best Practices

1. **Incremental Complexity**: Start simple, add features if they help
2. **Readable Code**: Meaningful names, comments, grouping
3. **Test Edge Cases**: Missing data, extremes, gaps
4. **Document Assumptions**: What market conditions is this for?
5. **Maintainability**: Clear logic flow, avoid one-liners

---

**Next:** [Error Reference →](07-error-reference.md)
