---
page_type: reference
layout: default
order: 10
category: Strategies
description: Market regime detection - Trend vs Range, Volatility States, ADX/DMI, Vortex Indicator
parent: ./index.md
---

# Regime Detection

Objective trend/range classification, volatility states, multi-indicator regime scoring.

---

## Vortex Indicator

```epochscript
vtx = vortex(period=14)(src.h, src.l, src.c)

# Direction
bullish = vtx.plus_indicator > vtx.minus_indicator
bearish = vtx.minus_indicator > vtx.plus_indicator

# Strength (spread between lines)
spread = abs(vtx.plus_indicator - vtx.minus_indicator)
strong_trend = spread > 0.15
weak_choppy = spread < 0.05

# Crossover signals
cross_bull = crossover(vtx.plus_indicator, vtx.minus_indicator)
cross_bear = crossunder(vtx.plus_indicator, vtx.minus_indicator)
```

**Interpretation:** Spread > 0.15 = strong directional, < 0.05 = ranging/choppy. Crossovers similar to DMI.

---

## ADX / DMI

```epochscript
adx_result = adx(period=14)(src.h, src.l, src.c)

# Trend strength (direction-agnostic)
strong_trend = adx_result.adx > 25
ranging = adx_result.adx < 20

# Direction
bullish = adx_result.plus_di > adx_result.minus_di

# Regime classification
strong_uptrend = strong_trend and bullish
strong_downtrend = strong_trend and not bullish

# Route strategy
trend_signal = ema(period=20)(src.c) > ema(period=50)(src.c)
mr_signal = rsi(period=14)(src.c) < 30

entry = conditional_select(
    strong_trend, trend_signal,  # Trend-follow in trending markets
    ranging, mr_signal,           # Mean-revert in ranges
    false
)
```

---

## Volatility Regimes

```epochscript
vol = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)

# Percentile-based regimes
vol_p20 = percentile(period=252)(vol.result, 20)
vol_p50 = percentile(period=252)(vol.result, 50)
vol_p80 = percentile(period=252)(vol.result, 80)

very_low = vol.result < vol_p20
low_normal = vol.result >= vol_p20 and vol.result < vol_p50
normal_high = vol.result >= vol_p50 and vol.result < vol_p80
high_vol = vol.result >= vol_p80

# Expansion/contraction
vol_ma = sma(period=50)(vol.result)
expanding = vol.result > vol_ma * 1.2
contracting = vol.result < vol_ma * 0.8

# Vol breakout
vol_squeeze = contracting[5] and expanding
```

---

## Trend vs Range Detection

```epochscript
# Multi-indicator confirmation
adx_result = adx(period=14)(src.h, src.l, src.c)
vtx = vortex(period=14)(src.h, src.l, src.c)
vol = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)

# Trending = ADX + Vortex confirm + reasonable vol
adx_trending = adx_result.adx > 25
vtx_strong = abs(vtx.plus_indicator - vtx.minus_indicator) > 0.12
vol_ok = vol.result < 0.30

trending = adx_trending and vtx_strong and vol_ok

# Ranging = low ADX + low vortex spread
adx_ranging = adx_result.adx < 20
vtx_weak = abs(vtx.plus_indicator - vtx.minus_indicator) < 0.08

ranging = adx_ranging and vtx_weak and vol_ok

# High vol chaos
chaos = vol.result >= 0.30
```

---

## Multi-Regime Scoring System

```epochscript
# Combine all indicators
adx_result = adx(period=14)(src.h, src.l, src.c)
vtx = vortex(period=14)(src.h, src.l, src.c)
vol = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)

# HMM for statistical regime
returns = (src.c / src.c[1] - 1) * 100
vol_simple = volatility(period=20)(src.c)
volume_ratio = src.v / sma(period=20)(src.v)

hmm = hidden_markov_model(n_states=3, lookback_window=252)(
    x=returns, y=vol_simple, z=volume_ratio
)

# Regime scores
trending_score = (
    conditional_select(adx_result.adx > 25, 1, 0) +
    conditional_select(abs(vtx.plus_indicator - vtx.minus_indicator) > 0.12, 1, 0)
)

bullish_score = (
    conditional_select(adx_result.plus_di > adx_result.minus_di, 1, 0) +
    conditional_select(vtx.plus_indicator > vtx.minus_indicator, 1, 0) +
    conditional_select(hmm.state == 2, 1, 0)
)

vol_score = conditional_select(
    vol.result < percentile(period=252)(vol.result, 20), 0,  # Very low
    vol.result < percentile(period=252)(vol.result, 80), 1,  # Normal
    2  # High
)

# Regime classification
strong_bull_trend = (trending_score >= 2) and (bullish_score >= 2) and (vol_score <= 1)
weak_bull = (bullish_score >= 2) and (trending_score < 2) and (vol_score <= 1)
high_vol_avoid = vol_score == 2

# Strategy routing
entry = conditional_select(
    strong_bull_trend, ema(period=20)(src.c) > ema(period=50)(src.c),  # Trend-follow
    weak_bull, rsi(period=14)(src.c) < 35,  # Buy dips
    false  # Avoid in other regimes
)
```

---

## Regime Transition Detection

```epochscript
# Track regime over time
adx_result = adx(period=14)(src.h, src.l, src.c)

trending_now = adx_result.adx > 25
trending_prev = trending_now[1]

# Detect transitions
trend_to_range = trending_prev and not trending_now
range_to_trend = not trending_prev and trending_now

# Vol transitions
vol = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)
vol_p50 = percentile(period=252)(vol.result, 50)

high_vol_now = vol.result > vol_p50
high_vol_prev = high_vol_now[1]

vol_spike = not high_vol_prev and high_vol_now

# Transition = uncertain, reduce exposure
in_transition = trend_to_range or range_to_trend or vol_spike
stable_regime = not in_transition

entry_ok = stable_regime
```

**Next:** [Cross-Sectional Ranking →](./cross-sectional-ranking.md)
