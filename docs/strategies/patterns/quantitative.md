---
page_type: implementation
layout: default
order: 4
category: Strategies
description: Quantitative strategies including cross-sectional analysis, pairs trading, and volatility targeting
parent: ./index.md
---

# Quantitative Strategies

:::note
For detailed cross-sectional ranking techniques (top K, bottom K, factor portfolios, relative strength rotation), see [Cross-Sectional Ranking](../cross-sectional-ranking.md)
:::

## Cross-Sectional Momentum

**Strategy:** Rank assets by momentum, buy top performers

**Requirements:**
- Universe of 50+ stocks
- Single-asset script won't work
- Requires external universe configuration

```epochscript
src = market_data_source()
close = src.c

# Calculate momentum (20-day rate of change)
returns_20 = roc(period=20)(close)

# Rank across universe
momentum_rank = cs_momentum()(returns_20)

# Select top 10 stocks
top_10 = top_k(k=10)(momentum_rank)

# Monthly rebalance (using month_of_year or custom logic)
# Rebalancing handled by execution engine

trade_signal_executor()(enter_long=top_10)
```

**Multi-Factor Enhancement:**
```epochscript
# Momentum factor
momentum = roc(period=20)(close)

# Volatility factor (prefer low volatility)
vol = volatility(period=60)(close)
low_vol_score = 1.0 / vol  # Invert so low vol = high score

# Quality factor (from fundamentals)
roe = financial_ratios().roe
quality_score = roe / 100.0  # Normalize

# Combine factors (equal weight)
combined_score = (momentum + low_vol_score + quality_score) / 3.0

# Rank and select
ranking = cs_momentum()(combined_score)
top_20 = top_k(k=20)(ranking)

trade_signal_executor()(enter_long=top_20)
```

### Statistical Arbitrage (Pairs Trading)

**Strategy:** Trade mean reversion of spread when correlation is high

```epochscript
# Two correlated assets
asset1 = market_data_source()  # Stock A
asset2 = market_data_source()  # Stock B (separate configuration)

# Calculate spread
spread = asset1.c - asset2.c

# Spread statistics
spread_ma = sma(period=20)(spread)
spread_std = stddev(period=20)(spread)

# Z-score of spread
z_score = (spread - spread_ma) / spread_std

# Monitor correlation regime
corr = rolling_corr(window=20)(x=asset1.c, y=asset2.c)
high_corr = corr > 0.8  # Only trade when correlated

# Entry: Extreme z-score + high correlation
entry_long_spread = high_corr and (z_score < -2.0)   # Spread cheap
entry_short_spread = high_corr and (z_score > 2.0)   # Spread expensive

# Exit: Mean reversion or correlation breakdown
exit_long = (abs(z_score) < 0.5) or (corr < 0.5)
exit_short = (abs(z_score) < 0.5) or (corr < 0.5)

# Execute (long spread = long A, short B)
trade_signal_executor()(
    enter_long=entry_long_spread,
    enter_short=entry_short_spread,
    exit_long=exit_long,
    exit_short=exit_short
)
```

### Dynamic Beta Hedging

**Strategy:** Adjust exposure based on rolling beta to market

```epochscript
stock = market_data_source()
market = market_data_source()  # SPY or market index

# Calculate rolling beta
cov_stock_market = rolling_cov(window=60)(x=stock.c, y=market.c)
var_market = rolling_cov(window=60)(x=market.c, y=market.c)
beta = cov_stock_market / var_market

# Beta regimes
low_beta = beta < 0.8   # Stock less sensitive to market (good)
high_beta = beta > 1.2  # Stock more sensitive to market (risky)

# Bullish signal (EMA cross)
fast = ema(period=20)(stock.c)
slow = ema(period=50)(stock.c)
bullish = crossover(fast, slow)

# Entry only in low-beta regime
entry = bullish and low_beta

# Exit when beta spikes (risk-off)
exit = high_beta

trade_signal_executor()(
    enter_long=entry,
    exit_long=exit
)
```

### Volatility Targeting

**Strategy:** Scale position size inversely with volatility

```epochscript
src = market_data_source()

# Target annualized volatility (e.g., 15%)
target_vol = 15.0

# Realized volatility
realized_vol = volatility(period=20)(src.c)

# Position sizing multiplier (scale inversely)
vol_scalar = target_vol / realized_vol
# Clamp to reasonable range
vol_scalar_clamped = conditional_select(
    vol_scalar > 2.0, 2.0,  # Max 2x
    vol_scalar < 0.5, 0.5,  # Min 0.5x
    vol_scalar
)

# Base signal (trend-following)
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)
trend_up = fast > slow

# Execution (position sizing handled externally)
# This signals direction; size adjusted by vol_scalar
trade_signal_executor()(enter_long=trend_up)
```

### Lead-Lag Arbitrage

**Strategy:** Trade based on lead-lag relationship between assets

```epochscript
# Leading indicator (e.g., crypto volatility)
leader = market_data_source()
leader_vol = volatility(period=20)(leader.c)

# Lagging indicator (e.g., tech stock drawdowns)
lagger = market_data_source()
lagger_dd = ulcer_index(period=20)(lagger.c)

# Concurrent correlation
concurrent_corr = rolling_corr(window=20)(x=leader_vol, y=lagger_dd)

# Lagged correlations (test different lags)
leader_vol_lag5 = leader_vol[5]
lag5_corr = rolling_corr(window=20)(x=leader_vol_lag5, y=lagger_dd)

# Lead-lag signal (if lag5_corr > concurrent_corr, leader predicts)
lead_lag_exists = lag5_corr > (concurrent_corr + 0.1)

# Trade lagger based on leader's signal
leader_spike = leader_vol > leader_vol[1] * 1.5
predicted_lagger_stress = lead_lag_exists and leader_spike

# Short lagger when leader predicts stress
trade_signal_executor()(enter_short=predicted_lagger_stress)
```

---
