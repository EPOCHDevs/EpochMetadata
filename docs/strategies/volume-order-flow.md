---
page_type: reference
layout: default
order: 12
category: Strategies
description: Volume analysis and order flow - OBV, A/D Line, VWAP, Klinger, MFI, CMF
parent: ./index.md
---

# Volume & Order Flow Analysis

Institutional flow detection using volume-based indicators, divergences, VWAP.

---

## OBV & A/D Line Divergences

```epochscript
obv_val = obv()(src.c, src.v)
ad = ad_line()(src.h, src.l, src.c, src.v)

# Bearish divergence (price higher, volume lower)
price_new_high = src.c > max(period=20)(src.c[1])
obv_lower = obv_val < obv_val[20]
ad_lower = ad.result < ad.result[20]

bearish_div = price_new_high and (obv_lower or ad_lower)

# Bullish divergence
price_new_low = src.l < min(period=20)(src.l[1])
obv_higher = obv_val > obv_val[20]
ad_higher = ad.result > ad.result[20]

bullish_div = price_new_low and (obv_higher or ad_higher)
```

---

## VWAP Mean Reversion

```epochscript
vwap_val = vwap()(src.h, src.l, src.c, src.v)

# Deviation from VWAP
deviation = (src.c - vwap_val.result) / vwap_val.result
extended_above = deviation > 0.02  # 2% above
extended_below = deviation < -0.02  # 2% below

# Mean reversion entry
entry_long = extended_below and (src.c > src.o)  # Bullish candle at deviation
entry_short = extended_above and (src.c < src.o)

exit = abs(deviation) < 0.005  # Back to VWAP
```

---

## Klinger Oscillator

```epochscript
klinger = klinger_oscillator(fast=34, slow=55, signal=13)(
    src.h, src.l, src.c, src.v
)

# Crossover signals
bullish_cross = crossover(klinger.klinger, klinger.signal)
bearish_cross = crossunder(klinger.klinger, klinger.signal)

# Divergence (institutional flow reversal)
price_low = src.c < min(period=20)(src.c[1])
klinger_higher = klinger.klinger > klinger.klinger[20]
bullish_div = price_low and klinger_higher
```

---

## Chaikin Money Flow & MFI

```epochscript
cmf = chaikin_money_flow(period=20)(src.h, src.l, src.c, src.v)
mfi_val = mfi(period=14)(src.h, src.l, src.c, src.v)

# Accumulation/Distribution
strong_acc = cmf.result > 0.20
strong_dist = cmf.result < -0.20

# Volume-weighted overbought/oversold
mfi_oversold = mfi_val.result < 20
mfi_overbought = mfi_val.result > 80

# Combined signal
entry = strong_acc and mfi_oversold
exit = strong_dist or mfi_overbought
```

---

## Volume-Confirmed Breakout

```epochscript
# Multi-volume indicator confirmation
obv_val = obv()(src.c, src.v)
ad = ad_line()(src.h, src.l, src.c, src.v)
cmf = chaikin_money_flow(period=20)(src.h, src.l, src.c, src.v)

obv_rising = obv_val > sma(period=20)(obv_val)
ad_rising = ad.result > ad.result[5]
cmf_positive = cmf.result > 0.1

volume_confirmed = obv_rising and ad_rising and cmf_positive

# Price breakout
resistance = max(period=50)(src.h[1])
breakout = src.c > resistance

entry = breakout and volume_confirmed
```

**Next:** [Advanced Momentum →](./advanced-momentum.md)
