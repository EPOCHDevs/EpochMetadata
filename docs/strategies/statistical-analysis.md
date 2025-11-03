---
page_type: reference
layout: default
order: 8
category: Strategies
description: Statistical & ML analysis - HMM, Hurst Exponent, Linear Regression, Z-Score, Correlation
parent: ./index.md
---

# Statistical Analysis & Machine Learning

HMM regime detection, Hurst exponent, linear regression, correlation analysis.

---

## Hidden Markov Model

```epochscript
returns = (src.c / src.c[1] - 1) * 100
vol = volatility(period=20)(src.c)
volume_ratio = src.v / sma(period=20)(src.v)

# 3-state regime detection
hmm = hidden_markov_model(
    n_states=3,
    max_iterations=1000,
    lookback_window=252,
    compute_zscore=true
)(x=returns, y=vol, z=volume_ratio)

# State interpretation (higher state number typically = higher vol/returns)
bull_regime = (hmm.state == 2) and (hmm.state_probability > 0.7)
bear_regime = (hmm.state == 0) and (hmm.state_probability > 0.7)

# Regime transition detection
regime_change = hmm.state != hmm.state[1]
exit_on_change = regime_change  # Exit when regime shifts
```

**n_states:** 3 = bull/neutral/bear, 5 = more granular (crash/bear/neutral/bull/euphoria). **lookback_window:** Rolling training window (252 = 1-year adaptive). **State ordering:** Not guaranteed—inspect `state_probability` and price behavior to map states.

---

## Hurst Exponent

```epochscript
returns = (src.c / src.c[1] - 1) * 100

# Rolling Hurst for adaptive regime detection
hurst = rolling_hurst_exponent(window=252)(returns)

# H < 0.5 = mean-reverting, H > 0.5 = trending
mean_reverting = hurst.result < 0.45
trending = hurst.result > 0.55
random_walk = hurst.result >= 0.45 and hurst.result <= 0.55

# Strategy routing
mr_signal = rsi(period=14)(src.c) < 30
tf_signal = ema(period=20)(src.c) > ema(period=50)(src.c)

entry = conditional_select(
    mean_reverting, mr_signal,  # Mean-revert when H<0.45
    trending, tf_signal,         # Trend-follow when H>0.55
    false                        # Avoid random walk
)
```

**Rolling vs Expanding:** `rolling_hurst_exponent(window=252)` = adaptive, `hurst_exponent(min_period=100)` = expanding (less responsive). **Lag grid:** Automatically scaled by window size—no manual tuning required.

---

## Linear Regression

```epochscript
bar_index = cumsum(1)
reg = linear_fit(window=50)(x=bar_index, y=src.c)

# Trend strength
strong_uptrend = reg.slope > 0.5
weak_uptrend = reg.slope > 0 and reg.slope < 0.3

# Deviation from trend (residual)
overextended = reg.residual > 2
underextended = reg.residual < -2

# Regression channel
fitted = reg.intercept + (reg.slope * bar_index)
std_dev = stdev(period=50)(reg.residual)
upper_band = fitted + (2 * std_dev)
lower_band = fitted - (2 * std_dev)

# Mean reversion to channel
entry = strong_uptrend and underextended and (src.c < lower_band)
exit = overextended or (src.c > upper_band)
```

**Slope units:** Points per bar (e.g., slope=0.5 = +0.5 price units per bar). **Residual:** Actual - fitted value (not standardized—use with stdev for z-score equivalent).

---

## Z-Score

```epochscript
price_z = zscore(period=20)(src.c)

# Standardized thresholds
oversold = price_z < -2
overbought = price_z > 2

# RSI z-score (more robust than fixed 30/70)
rsi_val = rsi(period=14)(src.c)
rsi_z = zscore(period=60)(rsi_val)
extreme_oversold = rsi_z < -1.5

# Volume spike detection
vol_z = zscore(period=20)(src.v)
unusual_volume = vol_z > 2
```

**Period selection:** 20 = short-term deviations, 60 = medium-term, 252 = long-term anomalies.

---

## Correlation & Covariance

```epochscript
spy_ret = (spy.c / spy.c[1] - 1) * 100
stock_ret = (src.c / src.c[1] - 1) * 100

# Correlation for pairs trading
corr = rolling_correlation(window=60)(x=spy_ret, y=stock_ret)
cointegrated = corr.result > 0.7

# EWM for responsiveness
ewm_corr = ewm_correlation(span=30)(x=spy_ret, y=stock_ret)
correlation_breaking = (ewm_corr.result < 0.5) and (corr.result > 0.7)

# Pairs trade setup
spread = src.c - spy.c
spread_z = zscore(period=60)(spread)

entry = cointegrated and (spread_z < -2) and not correlation_breaking
exit = spread_z > 0 or correlation_breaking
```

**EWM vs Rolling:** EWM reacts faster to correlation changes (good for dynamic hedging), rolling more stable (good for pair selection).

---

## Advanced: HMM + Hurst Adaptive Strategy

```epochscript
# Combine probabilistic and fractal regime detection
returns = (src.c / src.c[1] - 1) * 100
vol = volatility(period=20)(src.c)
volume_ratio = src.v / sma(period=20)(src.v)

# HMM for bull/bear/neutral
hmm = hidden_markov_model(n_states=3, lookback_window=252)(
    x=returns, y=vol, z=volume_ratio
)
bull_state = (hmm.state == 2) and (hmm.state_probability > 0.8)

# Hurst for mean-revert vs trend
hurst = rolling_hurst_exponent(window=252)(returns)
mean_reverting = hurst.result < 0.45
trending = hurst.result > 0.55

# Linear regression for entry timing
bar_index = cumsum(1)
reg = linear_fit(window=50)(x=bar_index, y=src.c)

# Strategy logic
# Bull+Trending = follow slope, enter on pullback
bull_trend = bull_state and trending and (reg.slope > 0.3)
pullback = reg.residual < -1.5

# Bull+Mean-reverting = buy extreme dips
bull_mr = bull_state and mean_reverting
price_z = zscore(period=20)(src.c)
extreme_dip = price_z < -2.5

entry = (bull_trend and pullback) or (bull_mr and extreme_dip)
exit = not bull_state or (hmm.state != hmm.state[1])  # Exit on regime change
```

---

## Pairs Trading with Stationarity Check

```epochscript
stock_a = market_data_source(symbol="AAPL")
stock_b = market_data_source(symbol="MSFT")

spread = stock_a.c - stock_b.c

# Verify spread is stationary
stat = stationary_check(window=120, significance_level=0.05)(spread)
valid_pair = stat.is_stationary and (stat.p_value < 0.01)

# Check correlation
ret_a = (stock_a.c / stock_a.c[1] - 1) * 100
ret_b = (stock_b.c / stock_b.c[1] - 1) * 100
corr = rolling_correlation(window=60)(x=ret_a, y=ret_b)
highly_correlated = corr.result > 0.75

# Only trade if stationary AND correlated
tradeable = valid_pair and highly_correlated

# Mean reversion entry
spread_z = zscore(period=60)(spread)
entry_long_a = tradeable and (spread_z < -2)
entry_short_a = tradeable and (spread_z > 2)

# Exit on stationarity breakdown
exit = not valid_pair or not highly_correlated or (abs(spread_z) < 0.5)
```

**Next:** [Advanced Volatility →](./advanced-volatility.md)
