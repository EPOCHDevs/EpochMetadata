---
page_type: reference
layout: default
order: 14
category: Strategies
description: Japanese candlestick patterns - Doji, Hammers, Engulfing, Stars, Harami
parent: ./index.md
---

# Candlestick Pattern Recognition

Automated pattern detection with context filters (support/resistance, trend, volume).

---

## Pattern Detection + Context

```epochscript
# Doji family
doji_candle = doji()(src.o, src.h, src.l, src.c)
dragonfly = dragonfly_doji()(src.o, src.h, src.l, src.c)
gravestone = gravestone_doji()(src.o, src.h, src.l, src.c)

# Hammers
hammer_p = hammer()(src.o, src.h, src.l, src.c)
inv_hammer = inverted_hammer()(src.o, src.h, src.l, src.c)
hanging = hanging_man()(src.o, src.h, src.l, src.c)
shooting = shooting_star()(src.o, src.h, src.l, src.c)

# Engulfing
bull_eng = bullish_engulfing()(src.o, src.h, src.l, src.c)
bear_eng = bearish_engulfing()(src.o, src.h, src.l, src.c)

# Stars
morning = morning_star()(src.o, src.h, src.l, src.c)
evening = evening_star()(src.o, src.h, src.l, src.c)

# Context filters
downtrend = ema(period=20)(src.c) < ema(period=50)(src.c)
uptrend = not downtrend

at_support = src.l < sma(period=50)(src.c) * 0.98
at_resistance = src.h > sma(period=50)(src.c) * 1.02

vol_surge = src.v > sma(period=20)(src.v) * 1.3

# Bullish patterns at support in downtrend
bullish_pattern = hammer_p.result or inv_hammer.result or bull_eng.result or morning.result
bullish_signal = bullish_pattern and downtrend and at_support and vol_surge

# Bearish patterns at resistance in uptrend
bearish_pattern = shooting.result or hanging.result or bear_eng.result or evening.result
bearish_signal = bearish_pattern and uptrend and at_resistance
```

---

## Next Candle Confirmation

```epochscript
# Detect hammer yesterday
hammer_prev = hammer()(src.o[1], src.h[1], src.l[1], src.c[1])

# Confirm with today's bullish candle
confirm = hammer_prev.result and (src.c > src.o) and (src.c > src.c[1])

entry = confirm and downtrend
```

---

## Multi-Pattern Confluence

```epochscript
# Bullish reversal confluence
hammer_p = hammer()(src.o, src.h, src.l, src.c)
bull_eng = bullish_engulfing()(src.o, src.h, src.l, src.c)
morning = morning_star()(src.o, src.h, src.l, src.c)
dragonfly = dragonfly_doji()(src.o, src.h, src.l, src.c)

# Multiple patterns at same location = stronger signal
pattern_count = (
    conditional_select(hammer_p.result, 1, 0) +
    conditional_select(bull_eng.result, 1, 0) +
    conditional_select(morning.result, 1, 0) +
    conditional_select(dragonfly.result, 1, 0)
)

strong_reversal = pattern_count >= 2  # 2+ patterns together

entry = strong_reversal and downtrend and at_support
```

**Next:** [Chart Formations →](./chart-formations.md)
