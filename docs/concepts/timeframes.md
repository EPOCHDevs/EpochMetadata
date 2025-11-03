---
page_type: concept
layout: default
order: 1
category: Core Concepts
description: Multi-timeframe analysis with automatic data resampling
parent: ./index.md
---

# Timeframes

Multi-timeframe analysis through automatic data resampling. Execute transforms on any timeframe regardless of strategy base resolution.

---

## Syntax

```epochscript
transform(period=N, timeframe="TF")(input)
```

The framework automatically resamples data to the specified timeframe before transform execution.

**Example:**
```epochscript
# Strategy runs on 15Min bars
src = market_data_source()

# Calculate on daily timeframe (auto-resampled)
daily_ema = ema(period=50, timeframe="1D")(src.c)

# Compare intraday price vs daily trend
above_trend = src.c > daily_ema
```

---

## Standard Timeframes

### Intraday
```epochscript
"1Min"    # 1 minute
"5Min"    # 5 minutes
"15Min"   # 15 minutes
"30Min"   # 30 minutes
"1H"      # 1 hour
"4H"      # 4 hours
```

### Daily and Above
```epochscript
"1D"      # 1 day
"1W"      # 1 week
"1M"      # 1 month
```

:::warning
Use short forms only: `"1D"`, `"1H"`, `"1W"`

Invalid: `"1Day"`, `"1Hour"`, `"1Week"`
:::

---

## Advanced Timeframes

### Month Anchoring
```epochscript
"1MS"     # Month start
"1ME"     # Month end

# Example: Month-end closing price
month_end = market_data_source(timeframe="1ME")().c
```

### Week Anchoring
```epochscript
"1W-MON"  # Week starting Monday (default)
"1W-FRI"  # Week starting Friday
"1W-SUN"  # Week starting Sunday

# Example: Forex weekly (Friday start)
weekly_fri = ema(period=10, timeframe="1W-FRI")(src.c)
```

### Business Days
```epochscript
"1B"      # 1 business day
"5B"      # 5 business days (1 week)
"21B"     # ~1 month of business days
```

---

## Multi-Timeframe Pattern

Higher timeframe for trend, lower timeframe for entry:

```epochscript
src = market_data_source()

# Daily trend filter
daily_trend = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_trend

# Intraday entry signals
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
entry = crossover(fast, slow)

# Trade only with daily trend
buy = entry and uptrend
trade_signal_executor()(enter_long=buy)
```

---

## See Also

- [Sessions](./sessions.md) - Combine timeframes with session filtering
- [Design Guidelines](../design-guidelines.md) - Timeframe validation errors
