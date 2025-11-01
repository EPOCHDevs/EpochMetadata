# Visualization System

Interactive charts, cards, events, and dashboards for strategy development.

---

## Table of Contents

1. [Overview](#overview)
2. [Auto-Plotting System](#auto-plotting-system)
3. [Sidebar Card System](#sidebar-card-system)
4. [Event Marker System](#event-marker-system)
5. [Research Dashboard](#research-dashboard)
6. [Workflow Integration](#workflow-integration)

---

## Overview

Epoch script provides automatic visualization for every aspect of your strategy. The visualization system serves as a **real-time feedback loop** during development:

**Four Pillars:**
1. **Auto-Plotting**: Every indicator automatically appears on charts
2. **Sidebar Cards**: Real-time metrics organized in card groups
3. **Event Markers**: Timeline navigation for important signals/patterns
4. **Research Dashboard**: Interactive analysis of gap reports, tables, charts

**No Configuration Required:**
- Write strategy code → Visualization happens automatically
- Change parameters → Charts update instantly
- Add indicators → New plots appear automatically

---

## Auto-Plotting System

### How It Works

**Every transform is automatically plotted** based on its category:

| Category | Plot Type | Location | Example |
|----------|-----------|----------|---------|
| **Overlay** | Line on price chart | Main panel | EMA, SMA, Bollinger Bands |
| **Oscillator** | Line in separate panel | Below chart | RSI, Stochastic, Williams %R |
| **Histogram** | Bars in separate panel | Below chart | MACD histogram, Volume |
| **Boolean** | Markers on chart | Main panel | Crossover signals, Breakouts |
| **Multi-output** | Multiple lines | Appropriate panel | Bollinger (3 lines), MACD (line + histogram) |

**No manual plotting** - just write the code:

```python
src = market_data_source()

# These automatically plot:
ema_20 = ema(period=20)(src.c)        # → Line overlay on price chart
rsi_val = rsi(period=14)(src.c)       # → Panel below chart (0-100 scale)
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)  # → Panel with 2 lines
```

### Chart Overlay Indicators

**Plotted on main price chart** (same scale as OHLCV):

```python
src = market_data_source()

# Moving averages - overlays
fast_ema = ema(period=12)(src.c)      # Blue line on chart
slow_ema = ema(period=26)(src.c)      # Red line on chart
sma_200 = sma(period=200)(src.c)      # Major trend line

# Bollinger Bands - 3 overlays
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
# → Upper band, Middle (SMA), Lower band all on price chart

# VWAP - overlay
vwap_line = market_data_source().vw   # Volume-weighted average price

# Adaptive moving averages - overlays
kama_line = kama(period=20)(src.c)    # Kaufman Adaptive MA
hma_line = hma(period=20)(src.c)      # Hull MA (low lag)
```

**Visual behavior:**
- Lines color-coded for clarity
- Tooltip shows exact values on hover
- Zoom/pan synchronized with price chart
- Legend shows all overlays

### Panel Indicators (Oscillators)

**Plotted in separate panels** below price chart:

```python
src = market_data_source()

# RSI - separate panel (0-100 scale)
rsi_val = rsi(period=14)(src.c)
# → Panel with 30/70 reference lines (oversold/overbought)

# Stochastic - separate panel (0-100 scale)
k, d = stoch(k_period=14, k_smooth=3, d_period=3)(src.h, src.l, src.c)
# → Panel with %K and %D lines, 20/80 reference lines

# MACD - separate panel
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)
histogram = macd_line - signal_line
# → Panel with MACD line, signal line, and histogram

// CCI - separate panel (unbounded)
cci_val = cci(period=20)(src.h, src.l, src.c)
# → Panel with ±100 reference lines

# ADX - separate panel (0-100 scale)
adx_val = adx(period=14)(src.h, src.l, src.c)
# → Panel with 25 reference line (trending threshold)
```

**Panel features:**
- Auto-scaled to indicator's natural range
- Reference lines for key thresholds
- Independent zoom/pan
- Collapsible panels

### Boolean Signal Markers

**Boolean transforms create visual markers** on the chart:

```python
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Crossovers - markers on chart
buy_signal = crossover(fast, slow)
sell_signal = crossover(slow, fast)
# → Green arrow up on buy bars, Red arrow down on sell bars

# Pattern detection - markers
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
overbought = rsi_val > 70
// → Marker at bottom of chart when oversold, top when overbought

# Breakouts - markers
bb = bbands(period=20, stddev=2)(src.c)
breakout_up = crossover(src.c, bb.bbands_upper)
breakout_down = crossunder(src.c, bb.bbands_lower)
# → Markers at breakout points
```

**Marker styles:**
- ▲ Green arrow: Bullish signals
- ▼ Red arrow: Bearish signals
- ● Dot: Neutral events
- Custom colors for different signal types

### Multi-Output Visualization

**Transforms with multiple outputs** plot all appropriately:

```python
src = market_data_source()

# Bollinger Bands - 3 overlay lines
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
# → All 3 lines on price chart, shaded area between upper/lower

# MACD - panel with multiple elements
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)
# → Panel with:
#    - MACD line (blue)
#    - Signal line (orange)
#    - Histogram (macd - signal, green/red bars)

# Stochastic - panel with 2 lines
k, d = stoch(k_period=14, k_smooth=3, d_period=3)(src.h, src.l, src.c)
# → Panel with %K (fast) and %D (slow) lines

# Aroon - panel with 2 lines
aroon_up, aroon_down = aroon(period=25)(src.h, src.l)
# → Panel with Aroon Up and Aroon Down (0-100 scale)

# Ichimoku - 5 overlay lines
tenkan, kijun, senkou_a, senkou_b, chikou = ichimoku()(src.h, src.l, src.c)
# → All 5 lines on price chart, cloud shading between senkou_a/senkou_b
```

### Volume Visualization

**Volume has special treatment:**

```python
src = market_data_source()
volume = src.v
# → Histogram at bottom of chart (green = up day, red = down day)

# Volume indicators
obv_val = obv()(src.c, src.v)
# → Line plot in volume panel or separate panel

# Volume moving average (overlay on volume)
avg_vol = sma(period=20)(src.v)
# → Line overlay on volume histogram
```

### Live Updating

**During backtesting or paper trading:**
- Charts update bar-by-bar as strategy executes
- Can pause, step forward/backward
- Hover any bar → see all indicator values at that timestamp
- Scrub timeline → charts rewind/fast-forward

**During live trading:**
- Real-time updates as new bars form
- Current (incomplete) bar shown differently
- Indicator values recalculate tick-by-tick

---

## Sidebar Card System

### Overview

**Sidebar displays real-time metrics** organized in collapsible card groups:

```
┌─────────────────────────┐
│  POSITION METRICS       │ ← Card Group
├─────────────────────────┤
│  Position Size    1.5   │ ← Numeric Card
│  Entry Price      145.2 │
│  Current P&L      +2.3% │
│  Unrealized       +348  │
└─────────────────────────┘
┌─────────────────────────┐
│  SIGNAL STATUS          │
├─────────────────────────┤
│  Trend Up         ✓     │ ← Boolean Card
│  RSI Oversold     ✗     │
│  Volume Confirm   ✓     │
└─────────────────────────┘
```

### Card Types

#### Numeric Cards

Display single numeric metrics:

```python
# These create numeric cards automatically
src = market_data_source()
current_price = src.c          # "Current Price: 145.23"
volume = src.v                 # "Volume: 2,450,300"

rsi_val = rsi(period=14)(src.c)   # "RSI (14): 42.5"
atr_val = atr(period=14)(src.h, src.l, src.c)  # "ATR (14): 2.35"

# Multiple numeric values
fast = ema(period=12)(src.c)   # "EMA (12): 144.8"
slow = ema(period=26)(src.c)   # "EMA (26): 143.2"
```

**Card features:**
- Auto-formatted (decimals, commas, percentages)
- Color-coded (green positive, red negative for P&L)
- Updates in real-time
- Tooltip shows formula/calculation

#### Boolean Cards

Display True/False status with checkmark/X:

```python
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Boolean conditions → cards
uptrend = fast > slow          # "Uptrend: ✓" or "Uptrend: ✗"
rsi_oversold = rsi(period=14)(src.c) < 30  # "RSI Oversold: ✓/✗"
volume_high = src.v > sma(period=20)(src.v) * 1.5  # "High Volume: ✓/✗"

# Complex conditions
strong_buy = uptrend and rsi_oversold and volume_high  # "Strong Buy: ✓/✗"
```

**Visual:**
- ✓ Green checkmark when True
- ✗ Red X when False
- Dimmed when not applicable

#### Quantile Cards

Show percentile/distribution information:

```python
# Quantile cards show where current value sits in distribution
rsi_val = rsi(period=14)(src.c)
# Card shows: "RSI Percentile: 23rd" (current RSI vs 100-bar history)

volatility = volatility(period=20)(src.c)
# "Volatility Percentile: 87th" (high volatility vs history)
```

#### Index/Categorical Cards

Display categorical states:

```python
# Market regime
adx_val = adx(period=14)(src.h, src.l, src.c)
regime = conditional_select(
    adx_val > 50, "Strong Trend",
    adx_val > 25, "Trend",
    adx_val > 20, "Weak Trend",
    "Range"
)
# Card: "Market Regime: Trend"

# Session status
london = sessions(session_type="London")()
current_session = conditional_select(
    london.active, "London",
    "Other"
)
# Card: "Active Session: London"
```

### Card Groups

**Organize related cards** into collapsible groups:

**Automatic grouping** by category:
- **Price & Volume**: OHLCV, VWAP
- **Trend Indicators**: EMAs, SMAs, ADX
- **Momentum**: RSI, MACD, Stochastic
- **Volatility**: ATR, Bollinger Width, Historical Vol
- **Signals**: Entry/exit conditions, pattern detections
- **Position**: Size, P&L, stops

**Example sidebar:**
```
PRICE & VOLUME
├─ Close: 145.23
├─ Volume: 2.4M
└─ VWAP: 144.95

TREND
├─ EMA (20): 144.50
├─ EMA (50): 142.10
├─ ADX: 32.5
└─ Trend Up: ✓

MOMENTUM
├─ RSI (14): 58.2
├─ MACD: 0.45
└─ Signal: 0.38

SIGNALS
├─ Entry Signal: ✗
├─ Exit Signal: ✗
└─ Strong Buy: ✗

POSITION
├─ Size: 1.5
├─ Entry: 143.20
├─ P&L: +2.3%
└─ Unrealized: +$348
```

### Real-Time Updates

**Cards update live:**
- During backtesting: Update bar-by-bar
- During paper/live trading: Update tick-by-tick
- Color flashes on change (green = increased, red = decreased)
- Timestamp shown for last update

**Interactive features:**
- Click card → highlight indicator on chart
- Hover → show detailed tooltip
- Pin important cards to top
- Hide/show card groups

---

## Event Marker System

### Purpose

**Mark important timestamps** and navigate via scrollable timeline:

```python
# Mark RSI extremes
rsi_val = rsi(period=14)(src.c)
oversold_event = rsi_val < 30
overbought_event = rsi_val > 70

event_marker(color_map={
    Success: ["oversold_event"],
    Error: ["overbought_event"]
})(
    oversold_event=oversold_event,
    overbought_event=overbought_event
)
```

**Creates scrollable list:**
```
EVENTS (Timeline)
├─ 2024-01-15 09:35  ● Oversold Event    [Success]
├─ 2024-01-15 11:20  ● Overbought Event  [Error]
├─ 2024-01-15 14:45  ● Oversold Event    [Success]
└─ 2024-01-16 10:10  ● Overbought Event  [Error]
```

**Click any event** → Chart jumps to that timestamp

### Creating Event Markers

#### Basic Usage

```python
src = market_data_source()

# Define events
entry_signal = ...  # Boolean condition
exit_signal = ...   // Boolean condition
pattern_detected = ...  # Boolean condition

# Create event marker
event_marker(color_map={
    Success: ["entry_signal"],
    Error: ["exit_signal"],
    Warning: ["pattern_detected"]
})(
    entry_signal=entry_signal,
    exit_signal=exit_signal,
    pattern_detected=pattern_detected
)
```

**Color categories:**
- `Success` (Green): Bullish signals, entries, confirmations
- `Error` (Red): Bearish signals, exits, warnings
- `Warning` (Yellow): Neutral events, patterns, regime changes

#### Entry/Exit Tracking

```python
src = market_data_source()
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Entry/exit signals
buy_signal = crossover(fast, slow)
sell_signal = crossover(slow, fast)

// Stop loss hit
stop_hit = src.c < entry_price * 0.98  # Hypothetical

# Mark all trade events
event_marker(color_map={
    Success: ["buy_signal"],
    Error: ["sell_signal", "stop_hit"]
})(
    buy_signal=buy_signal,
    sell_signal=sell_signal,
    stop_hit=stop_hit
)
```

**Timeline shows:**
```
├─ 2024-01-10 10:30  ▲ Buy Signal       [Success]
├─ 2024-01-12 14:20  ▼ Sell Signal      [Error]
├─ 2024-01-15 09:45  ⚠ Stop Hit         [Error]
└─ 2024-01-17 11:15  ▲ Buy Signal       [Success]
```

#### Pattern Detection Events

```python
src = market_data_source()

# Detect patterns
hs = head_and_shoulders(tolerance=0.02)()
tri = triangles(min_pattern_bars=15)()

# Pattern events
hs_detected = hs.pattern_detected
triangle_breakout = tri.breakout

# Mark pattern milestones
event_marker(color_map={
    Warning: ["hs_detected"],
    Success: ["triangle_breakout"]
})(
    hs_detected=hs_detected,
    triangle_breakout=triangle_breakout
)
```

#### Regime Change Events

```python
src = market_data_source()
adx_val = adx(period=14)(src.h, src.l, src.c)
vol = volatility(period=20)(src.c)

# Regime transitions
entering_trend = crossover(adx_val, 25)
leaving_trend = crossunder(adx_val, 20)
vol_spike = vol > vol[10] * 1.5

# Mark regime changes
event_marker(color_map={
    Success: ["entering_trend"],
    Warning: ["leaving_trend", "vol_spike"]
})(
    entering_trend=entering_trend,
    leaving_trend=leaving_trend,
    vol_spike=vol_spike
)
```

#### Gap Fill Events

```python
# Mark gap fills
gaps = session_gap(fill_percent=100, timeframe="1Min")()

large_gap = gaps.gap_size > 2.0
gap_filled = gaps.gap_filled
partial_fill = (gaps.gap_retrace > 50) and not gaps.gap_filled

event_marker(color_map={
    Warning: ["large_gap"],
    Success: ["gap_filled"],
    Error: ["partial_fill"]
})(
    large_gap=large_gap,
    gap_filled=gap_filled,
    partial_fill=partial_fill
)
```

**Timeline:**
```
├─ 2024-01-10 09:30  ⚠ Large Gap (2.3%)         [Warning]
├─ 2024-01-10 10:45  ✓ Gap Filled               [Success]
├─ 2024-01-11 09:30  ⚠ Large Gap (1.8%)         [Warning]
└─ 2024-01-11 15:30  ✗ Partial Fill (62%)       [Error]
```

### Event Navigation

**Scrollable timeline** (chronological order):
- Most recent events at top
- Click event → Chart jumps to timestamp
- Hover event → Preview chart at that moment
- Filter by category (Success/Warning/Error)
- Search events by type

**Keyboard shortcuts:**
- `N` - Next event
- `P` - Previous event
- `F` - Filter events
- `Esc` - Clear selection

### Use Cases

**Strategy Development:**
- Mark all entry signals → Review each visually → Refine conditions
- Mark pattern detections → Validate accuracy → Tune parameters
- Mark regime changes → See if strategy adapts correctly

**Debugging:**
- Mark unexpected behavior → Jump to problem bars
- Mark edge cases → Study what happened
- Mark false signals → Identify why they occurred

**Performance Analysis:**
- Mark all trades → Calculate win rate visually
- Mark stops hit → Analyze stop placement
- Mark missed opportunities → Improve signal timing

---

## Research Dashboard

### Overview

**Separate view for research reports** generated by research scripts:

**Workflow:**
1. Write research script (gap_report, table_report, etc.)
2. Run script → Generate reports
3. Open dashboard → Explore interactively
4. Analyze findings → Refine strategy
5. Re-run research → Validate improvements

### Report Types

#### Gap Reports

Comprehensive gap analysis with cards, charts, and tables:

```python
# Research script
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

**Dashboard displays:**

**Cards:**
- Fill Rate: 67.3%
- Average Gap Size: 1.24%
- Median Fill Time: 42 minutes
- Total Gaps: 1,247

**Histograms:**
- Gap Size Distribution (bins: 0-0.5%, 0.5-1%, 1-1.5%, etc.)
- Fill Time Distribution (bins: 0-15min, 15-30min, 30-60min, etc.)

**Tables:**
- Gaps by Hour (pivot table)
- Gaps by Day of Week
- Gaps by Size Bucket
- Detailed gap log (timestamp, size, fill time, filled?)

**Interactive:**
- Click histogram bar → Filter table to that bucket
- Click table row → Jump to that gap on chart
- Export table to CSV
- Adjust histogram bin size dynamically

#### Table Reports

SQL-style data tables:

```python
# Research script
returns = (src.c - src.c[1]) / src.c[1]
rsi_val = rsi(period=14)(src.c)
volume = src.v

table_report(sql="""
    SELECT
        CASE
            WHEN rsi_val < 30 THEN 'Oversold'
            WHEN rsi_val > 70 THEN 'Overbought'
            ELSE 'Neutral'
        END as regime,
        AVG(returns) as avg_return,
        STDDEV(returns) as volatility,
        AVG(volume) as avg_volume,
        COUNT(*) as count
    FROM input
    GROUP BY regime
    ORDER BY avg_return DESC
""")(
    rsi_val=rsi_val,
    returns=returns,
    volume=volume
)
```

**Dashboard shows:**
```
┌─────────────┬─────────────┬────────────┬─────────────┬───────┐
│ Regime      │ Avg Return  │ Volatility │ Avg Volume  │ Count │
├─────────────┼─────────────┼────────────┼─────────────┼───────┤
│ Oversold    │ +0.42%      │ 1.85%      │ 3.2M        │ 347   │
│ Neutral     │ +0.05%      │ 1.12%      │ 2.1M        │ 8,234 │
│ Overbought  │ -0.18%      │ 1.67%      │ 2.8M        │ 412   │
└─────────────┴─────────────┴────────────┴─────────────┴───────┘
```

**Interactive:**
- Sort by any column
- Filter rows
- Export to CSV/Excel
- Click row → Filter chart to that regime
- Drill down into details

#### Chart Reports

Visualizations for research:

```python
# Histogram report
returns = (src.c - src.c[1]) / src.c[1]
histogram_chart_report(bins=20, title="Return Distribution")(
    returns=returns
)

# Bar chart report
rsi_val = rsi(period=14)(src.c)
regime = conditional_select(
    rsi_val < 30, "Oversold",
    rsi_val > 70, "Overbought",
    "Neutral"
)
bar_chart_report(x="regime", y="count")(
    regime=regime
)

# Pie chart report
pie_chart_report(category="regime", value="count")(
    regime=regime
)
```

**Dashboard renders:**
- Interactive charts (zoom, pan, hover)
- Multiple charts side-by-side
- Export as PNG/SVG
- Adjust parameters (bins, colors, labels)

### Dashboard Features

**Layout:**
- Multi-tab view (one tab per report type)
- Resizable panels
- Drag-and-drop rearrange
- Save layout preferences

**Interactivity:**
- Linked brushing (select in one chart → highlight in others)
- Cross-filtering (filter table → update charts)
- Time range selection (slider to focus on date range)
- Compare multiple research runs side-by-side

**Export/Sharing:**
- Export entire dashboard as PDF
- Export individual reports (CSV, PNG)
- Share dashboard link
- Schedule automatic updates

---

## Workflow Integration

### Strategy Development Cycle

**Iterative workflow** with visual feedback:

**1. Initial Strategy**
```python
src = market_data_source()
rsi_val = rsi(period=14)(src.c)
entry = rsi_val < 30
trade_signal_executor()(enter_long=entry)
```
→ Run → **Chart shows:** RSI plot + entry markers
→ **Sidebar shows:** RSI value, entry signal status
→ **Visual feedback:** See all RSI < 30 signals on chart

**2. Refine from Visual**
*Observation:* Too many signals in choppy markets
```python
# Add trend filter (visible on chart)
ema_50 = ema(period=50)(src.c)
uptrend = src.c > ema_50

entry_v2 = (rsi_val < 30) and uptrend
```
→ **Chart now shows:** RSI + EMA overlay + filtered entry markers
→ **Sidebar shows:** Uptrend status
→ **Visual feedback:** Fewer, higher-quality signals

**3. Add Volume Confirmation**
```python
avg_vol = sma(period=20)(src.v)
volume_ok = src.v > (avg_vol * 1.2)

entry_v3 = (rsi_val < 30) and uptrend and volume_ok
```
→ **Chart shows:** Volume histogram + volume MA + final entry markers
→ **Sidebar shows:** Volume confirm status
→ **Visual feedback:** Even fewer signals, but stronger

**4. Mark Events for Analysis**
```python
event_marker(color_map={
    Success: ["entry_v3"],
    Warning: ["rsi_val < 30"],  # All oversold
    Error: ["not uptrend"]      # Trend violations
})(
    entry_v3=entry_v3,
    oversold_raw=rsi_val < 30,
    trend_violation=not uptrend
)
```
→ **Timeline shows:** All oversold moments vs actual entries
→ **Click event:** See why signal was/wasn't taken
→ **Insight:** Trend filter eliminating good signals in ranging markets

### Debugging with Visualization

**Problem:** Strategy not entering when expected

**Debug Steps:**

**1. Check Individual Conditions**
```python
# Mark each condition separately
rsi_ok = rsi_val < 30
trend_ok = uptrend
vol_ok = volume_ok

event_marker(color_map={
    Success: ["rsi_ok"],
    Warning: ["trend_ok"],
    Error: ["vol_ok"]
})(
    rsi_ok=rsi_ok,
    trend_ok=trend_ok,
    vol_ok=vol_ok
)
```
→ **Timeline shows:** When each condition is True
→ **Visual:** See which condition is blocking entry

**2. Inspect Indicator Values**
→ **Sidebar cards show:** Current values of RSI, EMA, Volume
→ **Hover chart:** See exact values at any timestamp
→ **Identify:** RSI is 31 (just barely above 30 threshold)

**3. Tune Threshold**
```python
# Relax RSI threshold slightly
rsi_ok = rsi_val < 35  # Was 30
```
→ **Visual feedback:** More entries appear
→ **Validate:** Check if entries are still high-quality

### Parameter Tuning

**Optimize RSI period visually:**

```python
# Test multiple periods
rsi_14 = rsi(period=14)(src.c)
rsi_21 = rsi(period=21)(src.c)
rsi_28 = rsi(period=28)(src.c)

# All plot in same panel (different colors)
entry_14 = rsi_14 < 30
entry_21 = rsi_21 < 30
entry_28 = rsi_28 < 30
```
→ **Visual comparison:** See which period gives cleaner signals
→ **Sidebar shows:** All three RSI values side-by-side
→ **Decision:** Pick period that aligns with your trading style

**Optimize moving average periods:**
```python
# Test EMA periods
ema_12 = ema(period=12)(src.c)
ema_20 = ema(period=20)(src.c)
ema_26 = ema(period=26)(src.c)

# All overlay on chart (different colors)
```
→ **Visual:** See lag differences, whipsaws, trend alignment
→ **Choose:** Period that balances responsiveness vs stability

### Best Practices

**1. Use Event Markers Liberally**
Mark everything interesting:
- Intended signals
- Almost-signals (missed by small margin)
- False signals (regret trades)
- Regime changes
- Unexpected behavior

**2. Organize Sidebar Thoughtfully**
Group related metrics:
- Put critical signals at top
- Use card groups to collapse less important info
- Pin key metrics during development

**3. Leverage Dashboard for Research**
Before implementing strategy:
- Run gap analysis → Understand gap behavior
- Run table reports → Validate statistical edges
- Run correlation studies → Find leading indicators
- Export findings → Document assumptions

**4. Iterate with Visual Feedback**
Don't code blindly:
- Add indicator → Check plot
- Add condition → Mark events
- Change threshold → See impact immediately
- Compare versions side-by-side

**5. Use Timeline Navigation**
Review strategy execution:
- Jump to winning trades → What made them work?
- Jump to losing trades → What went wrong?
- Jump to missed opportunities → Why didn't we enter?
- Jump to false signals → What caused them?

---

## Summary

### Four Pillars of Visualization

1. **Auto-Plotting**: Zero-config visualization
   - Overlays, panels, histograms, markers
   - Updates in real-time
   - Synchronized zoom/pan

2. **Sidebar Cards**: Real-time metrics
   - Numeric, boolean, quantile, categorical
   - Organized in collapsible groups
   - Color-coded, interactive

3. **Event Markers**: Timeline navigation
   - Mark signals, patterns, regimes
   - Scrollable chronological list
   - Click to jump to timestamp

4. **Research Dashboard**: Interactive analysis
   - Gap reports, tables, charts
   - Linked brushing, filtering
   - Export, share, schedule

### Key Benefits

**Faster Development:**
- Immediate visual feedback
- No manual plotting configuration
- Quickly validate ideas

**Better Understanding:**
- See strategy behavior visually
- Identify problems immediately
- Understand why signals fire

**Easier Debugging:**
- Mark events to isolate issues
- Jump to problem timestamps
- Compare indicator values

**Deeper Research:**
- Interactive dashboards
- Statistical analysis
- Pattern discovery

---

**Next:** [Glossary →](glossary.md)
