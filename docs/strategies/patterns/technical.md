---
page_type: implementation
layout: default
order: 3
category: Strategies
description: Technical trading patterns including MA crossovers, RSI, Bollinger Bands, and MACD strategies
parent: ./index.md
---

# Technical Trading

## MA Crossover System

**Strategy:** Buy when fast MA crosses above slow MA, sell on reverse cross

**Parameters:**
- Fast period: 12-20 (responsive)
- Slow period: 26-50 (trend)

```epochscript
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
```epochscript
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

```epochscript
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
```epochscript
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

```epochscript
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

```epochscript
src = market_data_source()

# MACD
mac = macd(short_period=12, long_period=26, signal_period=9)(src.c)
histogram = mac.macd - mac.macd_signal

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

```epochscript
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
