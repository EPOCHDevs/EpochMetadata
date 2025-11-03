---
page_type: concept
layout: default
order: 2
category: Core Concepts
description: Intraday session filtering and boundary detection
parent: ./index.md
---

# Sessions

Intraday session filtering and detection. Two modes: filter data to specific sessions or detect session boundaries.

---

## Two Session Features

| Feature | Purpose | Usage |
|---------|---------|-------|
| **`session` parameter** | Filter data to specific session | `ema(period=20, session="London")(src.c)` |
| **`sessions()` transform** | Detect session boundaries | `london = sessions(session_type="London")()` |

---

## Session Parameter (Data Filtering)

Execute transform on ONLY the bars within a specific trading session.

```epochscript
# EMA calculated ONLY on London session bars
london_ema = ema(period=20, session="London")(src.c)

# RSI calculated ONLY on New York session bars
ny_rsi = rsi(period=14, session="NewYork")(src.c)

# Gap detection for Asian session only
asian_gaps = session_gap(fill_percent=100, session="AsianKillZone")()
```

**Use cases:**
- Session-specific indicators (London volatility, NY volume)
- Isolate trading to high-liquidity periods
- Compare behavior across sessions

---

## sessions() Transform (Session Detection)

Mark session boundaries and track session high/low.

**Syntax:**
```epochscript
session = sessions(session_type="session_name")()
```

**Outputs:**
- `active` - Boolean: True during session
- `high` - Decimal: Session high price
- `low` - Decimal: Session low price
- `opened` - Boolean: True on session open bar
- `closed` - Boolean: True on session close bar

**Example:**
```epochscript
src = market_data_source()
london = sessions(session_type="London")()

# Trade only during London session
in_session = london.active
entry = bullish_condition and in_session

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Exit at session close
exit_signal = london.closed
```

---

## Available Sessions

### Geographic Sessions
```epochscript
"Sydney"    # Sydney, Australia (10 PM - 7 AM GMT)
"Tokyo"     # Tokyo, Japan (12 AM - 9 AM GMT)
"London"    # London, UK (8 AM - 5 PM GMT)
"NewYork"   # New York, USA (1 PM - 10 PM GMT)
```

### Kill Zones (ICT - High Liquidity Periods)
```epochscript
"AsianKillZone"         # 19:00-23:00 ET
"LondonOpenKillZone"    # 02:00-05:00 ET
"NewYorkKillZone"       # 07:00-10:00 ET
"LondonCloseKillZone"   # 10:00-12:00 ET
```

---

## Combined Session + TimeFrame

```epochscript
# Daily bars, but only from New York session data
daily_ny = market_data_source(
    timeframe="1D",
    session="NewYork"
)().c

# Hourly EMA, London session only
hourly_london = ema(
    period=20,
    timeframe="1H",
    session="London"
)(src.c)
```

**Execution order:**
1. Filter by `session` (if specified)
2. Resample to `timeframe` (if specified)
3. Execute transform

---

## Requirements

:::warning
Session transforms require intraday data.

If strategy runs on daily (1D) timeframe, session transforms will fail with:
`Transform 'sessions' requires intraday data`

Ensure strategy is configured for intraday timeframe (1Min, 5Min, 1H, etc.)
:::

---

## See Also

- [Timeframes](./timeframes.md) - Combine sessions with multi-timeframe analysis
- [Design Guidelines](../design-guidelines.md) - Session validation errors
