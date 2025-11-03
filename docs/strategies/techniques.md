---
page_type: implementation
layout: default
order: 2
category: Strategies
description: Complex conditional logic, dynamic parameter selection, and signal scoring systems
parent: ./index.md
---

# Advanced Techniques

Advanced coding patterns for complex conditional logic, dynamic parameter selection, and multi-signal scoring.

---

## Complex Conditional Logic

Advanced techniques for handling multi-condition logic without imperative control flow.

### Nested Ternary (Avoid When Possible)

:::warning
Nested ternaries become unreadable quickly. Use `conditional_select` instead.
:::

**Problematic approach:**

```epochscript
# Works but hard to read and maintain
label = (
    "STRONG_BUY" if rsi < 20 else (
    "BUY" if rsi < 30 else (
    "NEUTRAL" if rsi < 70 else (
    "SELL" if rsi < 80 else "STRONG_SELL"
    ))))
```

**Better approach:**

```epochscript
# Much clearer - reads like a decision table
label = conditional_select(
    rsi < 20, "STRONG_BUY",
    rsi < 30, "BUY",
    rsi < 70, "NEUTRAL",
    rsi < 80, "SELL",
    "STRONG_SELL"  # Default case
)
```

### Multi-Level Filters

Build complex signals by combining multiple independent filters.

:::tip
Layer filters hierarchically - each level adds confirmation to the previous levels.
:::

```epochscript
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

# Signal strength classification (for reporting/analysis)
signal_strength = conditional_select(
    strong_signal, "STRONG",
    weak_signal, "WEAK",
    "NONE"
)

# Entry decision based on signal strength
entry = strong_signal  # Only enter on strong signals
```

### Regime-Based Logic Switching

Adapt strategy logic based on detected market regimes.

:::note
Different market conditions require different strategies. Detect the regime first, then route to appropriate logic.

For regime detection methods (trending vs ranging, volatility states, ADX/DMI, Vortex), see [Regime Detection](./regime-detection.md)
:::

```epochscript
src = market_data_source()

# Detect regime using ADX and volatility
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

Techniques for adapting indicator parameters and exit conditions based on market state.

### Adaptive Periods Based on Volatility

:::warning
**Limitation:** Transform options must be literals, not variables. You cannot pass computed values as parameters.
:::

**What doesn't work:**

```epochscript
# This won't work - can't use variable as parameter
vol = volatility(period=20)(src.c)
ema_period = conditional_select(vol < 10, 12, vol < 20, 20, 50)
adaptive = ema(period=ema_period)(src.c)  # ERROR: period must be literal
```

**Workaround:** Pre-calculate multiple indicators, then select:

```epochscript
src = market_data_source()

# Calculate multiple EMAs with different periods
ema_12 = ema(period=12)(src.c)
ema_20 = ema(period=20)(src.c)
ema_26 = ema(period=26)(src.c)
ema_50 = ema(period=50)(src.c)

# Select based on volatility
vol = volatility(period=20)(src.c)
adaptive_ema = conditional_select(
    vol < 10, ema_12,   # Low vol: fast EMA
    vol < 20, ema_20,   # Medium vol: standard EMA
    vol < 30, ema_26,   # Higher vol: slower EMA
    ema_50              # High vol: very slow EMA
)

# Use adaptive EMA
signal = src.c > adaptive_ema
```

### State-Dependent Exits

Build multiple exit conditions combined with OR logic.

:::note
EpochScript doesn't support stateful position tracking. Use proxies like moving averages to approximate entry levels.
:::

**Idealized approach (requires state):**

```epochscript
# This concept requires state tracking - not directly supported
profit_target_hit = src.c > (entry_price * 1.05)  # 5% profit
stop_loss_hit = src.c < (entry_price * 0.98)      # 2% stop
```

**Practical workaround:**

```epochscript
src = market_data_source()

# Entry signal
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
entry = crossover(fast, slow)

# Use moving average as proxy for entry level
ema_entry = ema(period=50)(src.c)

# Multiple exit conditions
profit_condition = src.c > (ema_entry * 1.05)  # 5% above MA
stop_condition = src.c < (ema_entry * 0.98)    # 2% below MA
trend_reversal = crossunder(fast, slow)
rsi_overbought = rsi(period=14)(src.c) > 80

# Exit on any condition
exit = profit_condition or stop_condition or trend_reversal or rsi_overbought
```

---

## Multi-Condition Signal Scoring

Combine multiple signals into a weighted score for entry filtering and signal strength analysis.

### Weighted Scoring System

:::tip
Scoring systems allow you to quantify signal strength and filter entries based on confidence levels.
:::

```epochscript
src = market_data_source()

# Individual signals (boolean)
trend_up = ema(period=20)(src.c) > ema(period=50)(src.c)
rsi_ok = rsi(period=14)(src.c) < 70
macd_result = macd(fast=12, slow=26, signal=9)(src.c)
macd_bullish = macd_result[0] > macd_result[1]
volume_high = src.v > sma(period=20)(src.v) * 1.2

# Convert booleans to numbers (True = 1, False = 0)
trend_score = conditional_select(trend_up, 1, 0)
rsi_score = conditional_select(rsi_ok, 1, 0)
macd_score = conditional_select(macd_bullish, 1, 0)
volume_score = conditional_select(volume_high, 1, 0)

# Weighted sum (weights: trend=3, rsi=2, macd=2, volume=1)
total_score = (trend_score * 3) + (rsi_score * 2) + (macd_score * 2) + (volume_score * 1)
max_score = 8

# Classify signal strength by score
signal_quality = conditional_select(
    total_score >= 7, "STRONG",   # Strong signal (7-8 points)
    total_score >= 5, "MEDIUM",   # Medium signal (5-6 points)
    total_score >= 3, "WEAK",     # Weak signal (3-4 points)
    "NONE"                        # No signal (0-2 points)
)

# Entry threshold - only take strong signals
entry = total_score >= 7
```

### Confidence-Based Execution

Classify signals by confidence level and adjust entry behavior accordingly.

:::note
EpochScript generates signals and analysis only. Position sizing and risk management are handled by the execution layer, not in the script.
:::

```epochscript
src = market_data_source()

# Signal components
technical_bullish = ema(period=20)(src.c) > ema(period=50)(src.c)
momentum_bullish = rsi(period=14)(src.c) < 60
volume_confirming = src.v > sma(period=20)(src.v)

# Confidence levels
confidence_high = technical_bullish and momentum_bullish and volume_confirming
confidence_medium = technical_bullish and (momentum_bullish or volume_confirming)
confidence_low = technical_bullish

# Entry decision based on confidence
entry = conditional_select(
    confidence_high, True,   # Always enter
    confidence_medium, True, # Enter with caution
    confidence_low, False,   # Skip
    False                    # Default: no entry
)
```

:::note
**Best Practice:** Higher confidence signals can use wider stops and larger position sizes, while lower confidence signals should use tighter risk management.
:::

---

## Summary

**Key techniques:**

1. **Conditional Logic**
   - Use `conditional_select` for multi-way branching
   - Layer filters hierarchically
   - Detect regimes and route logic accordingly

2. **Dynamic Parameters**
   - Pre-calculate multiple indicator variants
   - Select based on market conditions using `conditional_select`
   - Use proxies (like EMAs) for stateful concepts

3. **Signal Scoring**
   - Convert boolean signals to numeric scores
   - Apply weights to prioritize important signals
   - Use scores for entry filtering and quality classification

**Next:** [Guidelines & Best Practices →](./guidelines.md)
