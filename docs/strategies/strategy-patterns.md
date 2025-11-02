# Strategy Patterns

Complete trading strategy blueprints organized by trading style.

---

## Table of Contents

1. [Technical Trading](#technical-trading)
2. [Quantitative Strategies](#quantitative-strategies)
3. [Intraday & Microstructure](#intraday--microstructure)
4. [Fundamental & Macro](#fundamental--macro)

---

## Technical Trading

### MA Crossover System

**Strategy:** Buy when fast MA crosses above slow MA, sell on reverse cross

**Parameters:**
- Fast period: 12-20 (responsive)
- Slow period: 26-50 (trend)

```python
src = market_data_source()

# Moving averages
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Entry signals
buy = crossover(fast, slow)
sell = crossover(slow, fast)

# Execute
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

**Enhancements:**
```python
# Add volume confirmation
avg_volume = sma(period=20)(src.v)
volume_confirm = src.v > (avg_volume * 1.2)

# Add ADX filter (only trade in trending markets)
adx_val = adx(period=14)(src.h, src.l, src.c)
trending = adx_val > 25

# Enhanced signals
buy_confirmed = buy and volume_confirm and trending
sell_confirmed = sell and volume_confirm and trending

trade_signal_executor()(enter_long=buy_confirmed, enter_short=sell_confirmed)
```

### RSI Mean Reversion

**Strategy:** Buy oversold, sell overbought, exit at midline

**Parameters:**
- RSI period: 14 (standard)
- Oversold: < 30
- Overbought: > 70

```python
src = market_data_source()

# RSI calculation
rsi_val = rsi(period=14)(src.c)

# Entry conditions
oversold = rsi_val < 30
overbought = rsi_val > 70

# Exit conditions
exit_long = rsi_val > 50
exit_short = rsi_val < 50

# Execute
trade_signal_executor()(
    enter_long=oversold,
    enter_short=overbought,
    exit_long=exit_long,
    exit_short=exit_short
)
```

**With Trend Filter:**
```python
# Only mean-revert with the trend
trend_ema = ema(period=50)(src.c)
uptrend = src.c > trend_ema
downtrend = src.c < trend_ema

# Filtered entries
buy = oversold and uptrend
sell = overbought and downtrend

trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### Bollinger Band Squeeze Breakout

**Strategy:** Enter on volatility expansion after compression

**Phases:**
1. Squeeze: Band width narrows (volatility compression)
2. Breakout: Price breaks bands with volume
3. Continuation: Ride the expansion

```python
src = market_data_source()

# Bollinger Bands
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
width = bband_width()(lower, middle, upper)

# Squeeze detection (width at multi-week low)
squeeze_threshold = 1.5  # Varies by asset
in_squeeze = width < squeeze_threshold

# Breakout detection
breakout_up = crossover(src.c, upper)
breakout_down = crossover(lower, src.c)

# Volume confirmation
avg_vol = sma(period=20)(src.v)
volume_surge = src.v > (avg_vol * 1.5)

# Entry after squeeze
buy = in_squeeze[1] and breakout_up and volume_surge
sell = in_squeeze[1] and breakout_down and volume_surge

# Exit when bands tighten again or price reverts to middle
exit_long = crossunder(src.c, middle) or (width < squeeze_threshold)
exit_short = crossover(src.c, middle) or (width < squeeze_threshold)

trade_signal_executor()(
    enter_long=buy,
    enter_short=sell,
    exit_long=exit_long,
    exit_short=exit_short
)
```

### MACD Divergence

**Strategy:** Enter on MACD/price divergence (early reversal detection)

```python
src = market_data_source()

# MACD
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)
histogram = macd_line - signal_line

# Price extremes
price_high = src.c > src.c[20]
price_low = src.c < src.c[20]

# MACD extremes
macd_lower = histogram < histogram[20]
macd_higher = histogram > histogram[20]

# Divergence
bullish_div = price_low and macd_higher  # Price lower, MACD higher
bearish_div = price_high and macd_lower  # Price higher, MACD lower

# MACD crossover confirmation
macd_cross_up = crossover(macd_line, signal_line)
macd_cross_down = crossover(signal_line, macd_line)

# Entry with divergence + crossover
buy = bullish_div and macd_cross_up
sell = bearish_div and macd_cross_down

trade_signal_executor()(enter_long=buy, enter_short=sell)
```

### Multi-Timeframe Trend Following

**Strategy:** Higher timeframe for trend, lower for entry timing

**Timeframe Setup:**
- Strategy runs on: 15Min
- Trend filter: 1D
- Confirmation: 1H

```python
src = market_data_source()

# Daily trend (1D timeframe)
daily_ema = ema(period=50, timeframe="1D")(src.c)
daily_uptrend = src.c > daily_ema
daily_downtrend = src.c < daily_ema

# Hourly confirmation (1H timeframe)
hourly_ema_fast = ema(period=20, timeframe="1H")(src.c)
hourly_ema_slow = ema(period=50, timeframe="1H")(src.c)
hourly_bullish = hourly_ema_fast > hourly_ema_slow
hourly_bearish = hourly_ema_fast < hourly_ema_slow

# Intraday entry (15Min base timeframe)
intraday_fast = ema(period=12)(src.c)
intraday_slow = ema(period=26)(src.c)
intraday_cross_up = crossover(intraday_fast, intraday_slow)
intraday_cross_down = crossover(intraday_slow, intraday_fast)

# Aligned entry (all timeframes agree)
buy = daily_uptrend and hourly_bullish and intraday_cross_up
sell = daily_downtrend and hourly_bearish and intraday_cross_down

# Exit when daily trend changes
exit_long = daily_downtrend
exit_short = daily_uptrend

trade_signal_executor()(
    enter_long=buy,
    enter_short=sell,
    exit_long=exit_long,
    exit_short=exit_short
)
```

---

## Quantitative Strategies

### Cross-Sectional Momentum

**Strategy:** Rank assets by momentum, buy top performers

**Requirements:**
- Universe of 50+ stocks
- Single-asset script won't work
- Requires external universe configuration

```python
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
```python
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

```python
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

```python
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

```python
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

```python
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

## Intraday & Microstructure

### London Breakout

**Strategy:** Trade breakout from London session range

```python
src = market_data_source()

# London session detection
london = sessions(session_type="London")()

# Session state
in_london = london.active
london_closed = london.closed

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Entry during or shortly after London
entry_long = (in_london or london_closed[1]) and breakout_high
entry_short = (in_london or london_closed[1]) and breakout_low

# Stop at opposite side of range
stop_long = src.c < london.low
stop_short = src.c > london.high

trade_signal_executor()(
    enter_long=entry_long,
    enter_short=entry_short,
    exit_long=stop_long,
    exit_short=stop_short
)
```

### Gap Fade Strategy

**Strategy:** Fade large overnight gaps (mean reversion)

```python
# Detect overnight gaps
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Gap characteristics
large_gap = gaps.gap_size > 1.5  # 1.5% gap
gap_not_filled = not gaps.gap_filled

# Fade entry (bet on reversion to previous close)
# Enter when gap exists and hasn't filled yet
fade_entry = large_gap and gap_not_filled

# Exit when gap fills (target reached)
exit_fade = gaps.gap_filled

# Stop if gap expands further (failed fade)
gap_expanding = gaps.gap_retrace < 20  # Less than 20% retraced

trade_signal_executor()(
    enter_long=fade_entry,
    exit_long=(exit_fade or gap_expanding)
)
```

### NY Kill Zone Momentum

**Strategy:** Trade momentum during high-liquidity NY kill zone

```python
src = market_data_source()

# NY Kill Zone (07:00-10:00 ET)
ny_kz = sessions(session_type="NewYorkKillZone")()
in_kill_zone = ny_kz.active

# Momentum indicators
rsi_val = rsi(period=14)(src.c)
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)

# Strong momentum signals
strong_bullish = (rsi_val > 60) and (macd_line > signal_line)
strong_bearish = (rsi_val < 40) and (macd_line < signal_line)

# Only trade during kill zone
entry_long = in_kill_zone and strong_bullish
entry_short = in_kill_zone and strong_bearish

# Exit at end of kill zone
kill_zone_ending = ny_kz.closed

trade_signal_executor()(
    enter_long=entry_long,
    enter_short=entry_short,
    exit_long=kill_zone_ending,
    exit_short=kill_zone_ending
)
```

### SMC Order Block Strategy

**Strategy:** Enter on order block retest with FVG fill

```python
src = market_data_source()

# SMC indicators
ob = order_blocks()(src.o, src.h, src.l, src.c)
fvg = fair_value_gap()(src.o, src.h, src.l, src.c)
shl = swing_highs_lows(left_count=5, right_count=5)()

# Bullish setup: Order block + FVG fill + structure break
bullish_ob = ob.bullish
fvg_filled = fvg.bullish_filled
structure_break = src.c > shl.swing_high_price

entry_long = bullish_ob and fvg_filled and structure_break

# Stop below order block
stop = ob.bullish_ob_low
exit_long = src.c < stop

trade_signal_executor()(
    enter_long=entry_long,
    exit_long=exit_long
)
```

### Opening Range Breakout

**Strategy:** Trade breakout from first 30 minutes

```python
src = market_data_source()

# Get first bar of NY session
ny = sessions(session_type="NewYork")()
session_opened = ny.opened

# Track opening range (first 30 min on 5Min chart)
# Assuming 5Min timeframe: 30 min = 6 bars
# Use session high/low at bar 6 after open
or_complete = session_opened[6]  # 6 bars ago = 30 min after open
or_high = ny.high[6]  # High at that time
or_low = ny.low[6]   # Low at that time

# Breakout
breakout_up = or_complete and (src.c > or_high)
breakout_down = or_complete and (src.c < or_low)

# Volume confirmation
avg_vol = sma(period=20)(src.v)
high_volume = src.v > (avg_vol * 1.3)

entry_long = breakout_up and high_volume
entry_short = breakout_down and high_volume

trade_signal_executor()(enter_long=entry_long, enter_short=entry_short)
```

---

## Fundamental & Macro

### Earnings Momentum

**Strategy:** Buy stocks with positive earnings surprises and momentum

```python
# Fundamental data
income = income_statement()
eps = income.basic_earnings_per_share

# Earnings growth (QoQ)
eps_growth = (eps - eps[1]) / eps[1]

# Price momentum
returns_20 = roc(period=20)(close)

# Combined signal
strong_earnings = eps_growth > 0.15  # 15% earnings growth
strong_momentum = returns_20 > 5.0   # 5% price momentum

entry = strong_earnings and strong_momentum

trade_signal_executor()(enter_long=entry)
```

### Value + Momentum Combo

**Strategy:** Buy undervalued stocks with positive momentum

```python
# Value metrics
ratios = financial_ratios()
pe = ratios.price_to_earnings
pb = ratios.price_to_book

# Value signal (low P/E and P/B)
undervalued = (pe < 15) and (pb < 2.0)

# Momentum signal
momentum = roc(period=60)(close)
positive_momentum = momentum > 0

# Quality filter
roe = ratios.return_on_equity
high_quality = roe > 15.0

# Combined signal
entry = undervalued and positive_momentum and high_quality

trade_signal_executor()(enter_long=entry)
```

### Economic Surprise Strategy

**Strategy:** Trade based on economic indicator surprises

```python
# Economic data (FRED)
gdp = economic_indicator(indicator="GDP")()
unemployment = economic_indicator(indicator="UNRATE")()
cpi = economic_indicator(indicator="CPIAUCSL")()

# Calculate surprise (actual vs expectation)
# Expectation = moving average
gdp_ma = sma(period=4)(gdp)  # 4 quarters
gdp_surprise = (gdp - gdp_ma) / gdp_ma

# Positive surprise (growth accelerating)
positive_surprise = gdp_surprise > 0.01  # 1% above trend

# Market data
src = market_data_source()

# Trend following on positive surprise
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)
trend_up = fast > slow

entry = positive_surprise and trend_up

trade_signal_executor()(enter_long=entry)
```

### Turn-of-Month Effect

**Strategy:** Exploit monthly equity inflows (institutional rebalancing)

```python
src = market_data_source()

# Turn-of-month window (3 days before, 5 days after)
tom = turn_of_month(days_before=3, days_after=5)()
in_window = tom.in_window

# Bullish bias during window
trend = ema(period=50)(src.c)
above_trend = src.c > trend

# Enter on turn-of-month + uptrend
entry = in_window and above_trend

# Exit after window closes
exit = not in_window

trade_signal_executor()(
    enter_long=entry,
    exit_long=exit
)
```

### January Effect (Small-Cap)

**Strategy:** Overweight small-caps in January

```python
# January detection
january = month_of_year(target_month="January")()
is_january = january.is_target_month

# Small-cap momentum (cross-sectional)
returns = roc(period=20)(close)
momentum = cs_momentum()(returns)

# Stronger filter in January (top 20 vs top 10)
entry = conditional_select(
    is_january, top_k(k=20)(momentum),  # More aggressive in Jan
    top_k(k=10)(momentum)               # Normal rest of year
)

trade_signal_executor()(enter_long=entry)
```

---

## Pattern Combinations

### Multi-Indicator Confirmation

**Strategy:** Require agreement from multiple indicator types

```python
src = market_data_source()

# Trend (moving averages)
fast_ma = ema(period=20)(src.c)
slow_ma = ema(period=50)(src.c)
trend_up = fast_ma > slow_ma

# Momentum (RSI)
rsi_val = rsi(period=14)(src.c)
momentum_ok = rsi_val < 70  # Not overbought

# Volatility (ATR expansion)
atr_val = atr(period=14)(src.h, src.l, src.c)
vol_expanding = atr_val > atr_val[5]

# Volume (above average)
avg_vol = sma(period=20)(src.v)
volume_ok = src.v > (avg_vol * 1.2)

# All must agree
entry = trend_up and momentum_ok and vol_expanding and volume_ok

trade_signal_executor()(enter_long=entry)
```

### Regime-Adaptive Strategy

**Strategy:** Switch strategy based on market regime

```python
src = market_data_source()

# Regime detection (ADX for trend strength)
adx_val = adx(period=14)(src.h, src.l, src.c)
trending = adx_val > 25
ranging = adx_val < 20

# Trend-following signals
ma_fast = ema(period=12)(src.c)
ma_slow = ema(period=26)(src.c)
trend_signal = ma_fast > ma_slow

# Mean reversion signals
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
overbought = rsi_val > 70

# Adaptive entry (trend-follow in trends, mean-revert in ranges)
entry_long = conditional_select(
    trending, trend_signal,    # Trend mode
    ranging, oversold,         # Range mode
    False                      # Neutral (don't trade)
)

entry_short = conditional_select(
    trending, not trend_signal,  # Trend mode
    ranging, overbought,         # Range mode
    False
)

trade_signal_executor()(enter_long=entry_long, enter_short=entry_short)
```

---

## Summary

### Strategy Categories

**Technical (5 patterns):** MA crossover, RSI, Bollinger, MACD, Multi-TF
**Quantitative (5 patterns):** Cross-sectional, Pairs, Beta hedging, Vol targeting, Lead-lag
**Intraday (5 patterns):** London breakout, Gap fade, Kill zone, SMC, Opening range
**Fundamental (5 patterns):** Earnings, Value+momentum, Economic, Turn-of-month, January

**Total: 20 complete strategy blueprints**

### Key Principles

1. **Confirmation**: Combine multiple indicators
2. **Risk Management**: Use stops, volatility sizing, regime filters
3. **Context**: Match strategy to market regime
4. **Timeframes**: Use multiple timeframes for context
5. **Volume**: Confirm signals with volume
6. **Simplicity**: Start simple, add complexity only if needed

---

**Next:** [Research Workflows →](./research-workflows.md)
