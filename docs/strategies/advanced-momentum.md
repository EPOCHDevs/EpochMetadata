---
page_type: reference
layout: default
order: 13
category: Strategies
description: Advanced momentum indicators - Fisher Transform, Ultimate Oscillator, Trix, Qstick
parent: ./index.md
---

# Advanced Momentum Indicators

Fisher Transform, Ultimate Oscillator, TRIX, Qstick, Mesa Sine, Schaff Trend Cycle.

:::note
This covers exotic momentum indicators. For basic RSI/MACD strategies, see [Technical Patterns](./patterns/technical.md)
:::

---

## Fisher Transform

```epochscript
fisher = fisher_transform(period=10)(src.h, src.l)

# Extreme reversal signals
fisher_oversold = fisher.fisher < -2.0
fisher_overbought = fisher.fisher > 2.0

# Crossover
bullish_cross = crossover(fisher.fisher, fisher.signal)
bearish_cross = crossunder(fisher.fisher, fisher.signal)

entry = fisher_oversold and bullish_cross
exit = fisher_overbought or bearish_cross
```

**Sharp turning points:** Extreme values (±2) + crossover = high-probability reversals.

---

## Ultimate Oscillator

```epochscript
uo = ultimate_oscillator(fast=7, mid=14, slow=28)(src.h, src.l, src.c)

# Divergence at extremes
price_lower_low = src.l < min(period=20)(src.l[1])
uo_higher_low = uo.result > uo.result[20]

bullish_div = price_lower_low and uo_higher_low and (uo.result < 30)

price_higher_high = src.h > max(period=20)(src.h[1])
uo_lower_high = uo.result < uo.result[20]

bearish_div = price_higher_high and uo_lower_high and (uo.result > 70)
```

---

## TRIX

```epochscript
trix_val = trix(period=14)(src.c)

# Zero-line crosses
bullish_trend = trix_val.trix > 0
bearish_trend = trix_val.trix < 0

bull_cross = crossover(trix_val.trix, 0)
bear_cross = crossunder(trix_val.trix, 0)

# Signal line crossover (MACD-style)
bullish_signal = crossover(trix_val.trix, trix_val.signal)
bearish_signal = crossunder(trix_val.trix, trix_val.signal)

entry = bullish_trend and bullish_signal
exit = bear_cross or bearish_signal
```

---

## Qstick

```epochscript
qstick_val = qstick(period=14)(src.o, src.c)

# Candle momentum
strong_bullish_candles = qstick_val.result > 0.5
weak_bullish = qstick_val.result > 0 and qstick_val.result < 0.3

# Distribution detection (price up but bearish candles)
price_rising = src.c > src.c[10]
qstick_falling = qstick_val.result < qstick_val.result[10]

distribution = price_rising and qstick_falling  # Hidden weakness

entry = (qstick_val.result > 0.3) and (qstick_val.result > qstick_val.result[3])
exit = distribution
```

---

## Multi-Oscillator Confirmation

```epochscript
fisher = fisher_transform(period=10)(src.h, src.l)
uo = ultimate_oscillator()(src.h, src.l, src.c)
trix_val = trix(period=14)(src.c)
qstick_val = qstick(period=14)(src.o, src.c)

# Reversal confluence
fisher_os = fisher.fisher < -1.5
fisher_cross = crossover(fisher.fisher, fisher.signal)

uo_div = (src.l < min(period=20)(src.l[1])) and (uo.result > uo.result[20])

trix_bullish = trix_val.trix > 0

qstick_acc = qstick_val.result > 0.2

# High-probability setup
reversal = fisher_os and fisher_cross and uo_div
trend_ok = trix_bullish
momentum_ok = qstick_acc

entry = reversal and trend_ok and momentum_ok

# Exit
fisher_ob = fisher.fisher > 2.0
trix_bearish = crossunder(trix_val.trix, 0)

exit = fisher_ob or trix_bearish
```

**Next:** [Candlestick Patterns →](./candlestick-patterns.md)
