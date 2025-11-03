---
page_type: reference
layout: default
order: 9
category: Strategies
description: Professional volatility estimators - Garman-Klass, Parkinson, Yang-Zhang, Hodges-Tompkins
parent: ./index.md
---

# Advanced Volatility Estimators

Range-based volatility using OHLC data: 5-14x more efficient than close-to-close.

---

## Efficiency Comparison

| Estimator | Efficiency vs Close² | Drift-Independent | Handles Gaps | Best For |
|-----------|---------------------|-------------------|--------------|----------|
| Parkinson | 5.2x | No (fails in trends) | No | Ranging markets |
| Garman-Klass | 7.4x | No | Partial | General use (most popular) |
| Rogers-Satchell | 8.1x | Yes | No | Trending markets |
| Yang-Zhang | 14x | Yes | Yes | Professional risk mgmt |
| Hodges-Tompkins | 1.0x | N/A (close-only) | No | Autocorrelation correction |

---

## Garman-Klass (7.4x)

```epochscript
vol_gk = garman_klass(period=20, trading_days=252)(src.l, src.h, src.o, src.c)

# Vol regime detection
vol_p20 = percentile(period=252)(vol_gk.result, 20)
vol_p80 = percentile(period=252)(vol_gk.result, 80)

low_vol = vol_gk.result < vol_p20
high_vol = vol_gk.result > vol_p80

# Vol breakout
vol_ma = sma(period=50)(vol_gk.result)
vol_expansion = vol_gk.result > vol_ma * 1.5
```

**Drift assumption:** Assumes zero drift—underestimates in strong trends. Use Yang-Zhang for trending markets.

---

## Yang-Zhang (14x - Industry Standard)

```epochscript
vol_yz = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)

# Vol targeting
target_vol = 0.16  # 16% annual target
vol_ratio = target_vol / vol_yz.result
reduce_exposure = vol_yz.result > target_vol * 1.5
increase_exposure = vol_yz.result < target_vol * 0.5

# Dynamic stop distance
daily_vol = vol_yz.result / sqrt(252)
stop_distance = src.c * daily_vol * 2.0  # 2x daily vol

# Vol term structure
vol_30 = yang_zhang(period=30, trading_periods=252)(src.l, src.h, src.o, src.c)
vol_60 = yang_zhang(period=60, trading_periods=252)(src.l, src.h, src.o, src.c)

upward_sloping = vol_60.result > vol_30.result  # Expecting vol increase
inverted = vol_30.result > vol_60.result  # Expecting vol decrease
```

**Gap handling:** Combines overnight volatility + open-close volatility + Rogers-Satchell intraday. **Use case:** Risk parity, vol targeting, Sharpe ratio calc.

---

## Parkinson (5.2x)

```epochscript
vol_park = parkinson(period=20, trading_periods=252)(src.l, src.h)

# Overestimates in trends—use for ranging markets
ranging = adx(period=14)(src.h, src.l, src.c).adx < 20
vol_ok = ranging and (vol_park.result < 0.30)
```

**Fails when:** Strong trend present (drift assumption violated). **Best when:** Sideways, range-bound markets.

---

## Comparative Analysis

```epochscript
vol_park = parkinson(period=20, trading_periods=252)(src.l, src.h)
vol_gk = garman_klass(period=20, trading_days=252)(src.l, src.h, src.o, src.c)
vol_yz = yang_zhang(period=20, trading_periods=252)(src.l, src.h, src.o, src.c)

# Detect gap vs intraday vol
park_vs_gk = vol_park.result / vol_gk.result
gap_vol_present = vol_yz.result > vol_gk.result * 1.2  # YZ>GK = gaps

# High intraday vol (range-based)
gk_vs_close = vol_gk.result / (stdev(period=20)(returns) * sqrt(252))
high_intraday = gk_vs_close > 1.5  # Lots of intraday movement

# Regime classification
trending_gappy = gap_vol_present and high_intraday
clean_ranging = not gap_vol_present and (park_vs_gk > 0.95)
```

**Next:** [Regime Detection →](./regime-detection.md)
