---
page_type: implementation
layout: default
order: 5
category: Strategies
description: Intraday trading patterns including London breakout, gap fade, kill zones, and SMC order blocks
parent: ./index.md
---

# Intraday & Microstructure

## London Breakout

**Strategy:** Trade breakout from London session range

```epochscript
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

```epochscript
src = market_data_source()

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

```epochscript
src = market_data_source()

# NY Kill Zone (07:00-10:00 ET)
ny_kz = sessions(session_type="NewYorkKillZone")()
in_kill_zone = ny_kz.active

# Momentum indicators
rsi_val = rsi(period=14)(src.c)
mac = macd(short_period=12, long_period=26, signal_period=9)(src.c)

# Strong momentum signals
strong_bullish = (rsi_val > 60) and (mac.macd > mac.macd_signal)
strong_bearish = (rsi_val < 40) and (mac.macd < mac.macd_signal)

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

:::note
For detailed Smart Money Concepts (order blocks, liquidity, BOS/CHoCH, FVGs), see [ICT & Smart Money](../ict-smart-money.md)
:::

**Strategy:** Enter on order block retest with FVG fill

```epochscript
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

```epochscript
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

# Stop at opposite side of range
stop_long = src.c < or_low
stop_short = src.c > or_high

trade_signal_executor()(
    enter_long=entry_long,
    enter_short=entry_short,
    exit_long=stop_long,
    exit_short=stop_short
)
```

---
