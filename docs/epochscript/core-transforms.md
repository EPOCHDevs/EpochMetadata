# Core Transforms Reference

Essential transforms covering 80% of trading use cases. Organized by trading purpose.

---

## Table of Contents

1. [Data Sources & Execution](#data-sources--execution)
2. [Trend Following](#trend-following)
3. [Momentum & Oscillators](#momentum--oscillators)
4. [Volatility Analysis](#volatility-analysis)
5. [Volume Analysis](#volume-analysis)
6. [Comparison & Logic](#comparison--logic)
7. [Statistical Analysis](#statistical-analysis)
8. [Market Microstructure](#market-microstructure)
9. [Chart Patterns & Price Action](#chart-patterns--price-action)
10. [Calendar Effects](#calendar-effects)
11. [Reporting](#reporting)

---

## Data Sources & Execution

### market_data_source

**Purpose:** Access OHLCV market data (required for all strategies)

**Syntax:**
```python
src = market_data_source()
```

**Outputs:**
- `o` - Open price
- `h` - High price
- `l` - Low price
- `c` - Close price
- `v` - Volume
- `vw` - Volume-weighted average price (VWAP)
- `n` - Number of trades

**Example:**
```python
src = market_data_source()
close = src.c
high = src.h
volume = src.v

# Use in calculations
is_bullish = close > close[1]
high_volume = volume > 1000000
```

### trade_signal_executor

**Purpose:** Execute trades based on boolean signals (REQUIRED for trading strategies)

**Syntax:**
```python
trade_signal_executor()(
    enter_long=buy_signal,
    enter_short=sell_signal,
    exit_long=exit_buy,
    exit_short=exit_sell
)
```

**Inputs (all Boolean, optional but at least one required):**
- `enter_long` - Signal to open long position
- `enter_short` - Signal to open short position
- `exit_long` - Signal to close long position
- `exit_short` - Signal to close short position

**Execution Rules:**
- Exits processed BEFORE entries
- Conflicting long+short entry = no trade
- Can flip positions in one bar

**Example:**
```python
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

buy = crossover(fast, slow)
sell = crossover(slow, fast)

trade_signal_executor()(enter_long=buy, enter_short=sell)
```

---

## Trend Following

### ema

**Purpose:** Exponential Moving Average (more responsive than SMA)

**Options:**
- `period` (Integer, required) - Lookback period (common: 10, 20, 50, 200)

**Inputs:**
- Single numeric series (typically close price)

**Outputs:**
- `result` - EMA value

**Use Cases:**
- Trend identification (price vs EMA)
- Crossover systems (fast EMA vs slow EMA)
- Dynamic support/resistance

**Example:**
```python
src = market_data_source()
ema_20 = ema(period=20)(src.c)
ema_50 = ema(period=50)(src.c)

# Uptrend when price above EMA
uptrend = src.c > ema_20

# Golden cross
golden_cross = crossover(ema_20, ema_50)
```

**Tuning Guidance:**
- **Short-term (10-20)**: Responsive, more signals, more noise
- **Medium-term (50-100)**: Balanced trend following
- **Long-term (200+)**: Strong trends only, fewer signals

### sma

**Purpose:** Simple Moving Average (equal weighting)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - SMA value

**Use Cases:**
- Classic trend following
- Support/resistance levels
- Baseline for mean reversion

**Example:**
```python
sma_20 = sma(period=20)(src.c)
sma_200 = sma(period=200)(src.c)

# Major trend
bull_market = src.c > sma_200
```

### hma

**Purpose:** Hull Moving Average (reduced lag, smoother)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - HMA value

**Use Cases:**
- Low-lag trend identification
- Crossovers with reduced false signals
- Price action strategies

**Example:**
```python
hma_20 = hma(period=20)(src.c)

# Hull MA slope as trend filter
hma_rising = hma_20 > hma_20[1]
```

### kama

**Purpose:** Kaufman Adaptive Moving Average (adapts to volatility)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - KAMA value

**Use Cases:**
- Volatile market adaptation
- Reduced whipsaws in choppy markets
- Trend/range regime detection

**Example:**
```python
kama_20 = kama(period=20)(src.c)

# KAMA flattens in consolidation, trends in breakouts
trend_strength = abs(kama_20 - kama_20[5]) / atr(period=14)(src.c)
```

### adx

**Purpose:** Average Directional Index (trend strength, 0-100 scale)

**Options:**
- `period` (Integer, default=14) - Lookback period

**Inputs:**
- Requires: high, low, close

**Outputs:**
- `result` - ADX value (0-100)

**Interpretation:**
- **0-25**: Weak trend, range-bound
- **25-50**: Developing trend
- **50-75**: Strong trend
- **75-100**: Very strong trend

**Use Cases:**
- Trend vs range filter
- Position sizing (larger in strong trends)
- Strategy regime switching

**Example:**
```python
src = market_data_source()
adx_val = adx(period=14)(src.h, src.l, src.c)

# Only trade in trending markets
strong_trend = adx_val > 25
entry = ma_cross_signal and strong_trend
```

### aroon

**Purpose:** Aroon Indicator (time since highs/lows, 0-100 scale)

**Options:**
- `period` (Integer, default=25) - Lookback period

**Inputs:**
- Requires: high, low

**Outputs:**
- `aroon_up` - Time since high (100 = recent high)
- `aroon_down` - Time since low (100 = recent low)

**Use Cases:**
- Trend direction (up vs down)
- Trend strength (both high = consolidation)
- Trend reversal detection

**Example:**
```python
src = market_data_source()
aroon_up, aroon_down = aroon(period=25)(src.h, src.l)

# Strong uptrend
uptrend = (aroon_up > 70) and (aroon_down < 30)

# Range-bound (both near 50)
consolidation = (abs(aroon_up - 50) < 20) and (abs(aroon_down - 50) < 20)
```

### lag

**Purpose:** Shift series backward by N periods (custom lag)

**Options:**
- `periods` (Integer, required) - Number of periods to shift

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - Lagged series

**Use Cases:**
- Custom historical comparisons
- Lead-lag analysis
- Rate of change calculations

**Example:**
```python
# Calculate 10-period momentum manually
lag_10 = lag(periods=10)(src.c)
momentum = src.c - lag_10
```

**Note:** Prefer bracket notation for simple lags: `src.c[10]` is clearer than `lag(periods=10)(src.c)`

---

## Momentum & Oscillators

### rsi

**Purpose:** Relative Strength Index (momentum oscillator, 0-100 scale)

**Options:**
- `period` (Integer, default=14) - Lookback period

**Inputs:**
- Single numeric series (typically close)

**Outputs:**
- `result` - RSI value (0-100)

**Interpretation:**
- **< 30**: Oversold (potential buy)
- **> 70**: Overbought (potential sell)
- **50**: Neutral

**Use Cases:**
- Mean reversion entries (oversold/overbought)
- Divergence detection (price vs RSI)
- Trend confirmation (RSI stays above 50 in uptrends)

**Example:**
```python
src = market_data_source()
rsi_val = rsi(period=14)(src.c)

# Mean reversion
oversold = rsi_val < 30
overbought = rsi_val > 70

# Trend-following variant (stay long above 50)
bullish_regime = rsi_val > 50
```

**Tuning Guidance:**
- **Short periods (5-9)**: More sensitive, more signals
- **Standard (14)**: Balanced, widely used
- **Long periods (21-28)**: Smoother, fewer signals

### macd

**Purpose:** Moving Average Convergence Divergence (trend + momentum)

**Options:**
- `fast` (Integer, default=12) - Fast EMA period
- `slow` (Integer, default=26) - Slow EMA period
- `signal` (Integer, default=9) - Signal line EMA period

**Inputs:**
- Single numeric series

**Outputs:**
- `macd` - MACD line (fast EMA - slow EMA)
- `signal` - Signal line (EMA of MACD)

**Interpretation:**
- **MACD > Signal**: Bullish momentum
- **MACD < Signal**: Bearish momentum
- **Histogram** (macd - signal): Momentum strength

**Use Cases:**
- Crossover signals (MACD vs signal)
- Divergence detection
- Trend confirmation
- Zero-line crosses (trend changes)

**Example:**
```python
src = market_data_source()
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)

# Classic crossover
buy = crossover(macd_line, signal_line)
sell = crossover(signal_line, macd_line)

# Histogram
histogram = macd_line - signal_line
momentum_increasing = histogram > histogram[1]
```

### stoch

**Purpose:** Stochastic Oscillator (momentum, 0-100 scale)

**Options:**
- `k_period` (Integer, default=14) - %K lookback
- `k_smooth` (Integer, default=3) - %K smoothing
- `d_period` (Integer, default=3) - %D smoothing

**Inputs:**
- Requires: high, low, close

**Outputs:**
- `stoch_k` - Fast stochastic (%K)
- `stoch_d` - Slow stochastic (%D, signal line)

**Interpretation:**
- **< 20**: Oversold
- **> 80**: Overbought
- **%K crosses above %D**: Bullish
- **%K crosses below %D**: Bearish

**Use Cases:**
- Oversold/overbought identification
- Crossover signals
- Divergence analysis

**Example:**
```python
src = market_data_source()
k, d = stoch(k_period=14, k_smooth=3, d_period=3)(src.h, src.l, src.c)

# Oversold crossover
oversold_cross = (k < 20) and crossover(k, d)

# Overbought crossunder
overbought_cross = (k > 80) and crossover(d, k)
```

### cci

**Purpose:** Commodity Channel Index (unbounded oscillator)

**Options:**
- `period` (Integer, default=20) - Lookback period

**Inputs:**
- Requires: high, low, close

**Outputs:**
- `result` - CCI value (unbounded, typically -200 to +200)

**Interpretation:**
- **> +100**: Overbought
- **< -100**: Oversold
- **Crossovers at ±100**: Entry/exit signals

**Use Cases:**
- Trend-following in strong moves
- Overbought/oversold extremes
- Divergence detection

**Example:**
```python
src = market_data_source()
cci_val = cci(period=20)(src.h, src.l, src.c)

# Breakout above +100
bullish_breakout = crossover(cci_val, 100)

# Extreme oversold
extreme_oversold = cci_val < -200
```

### mfi

**Purpose:** Money Flow Index (volume-weighted RSI, 0-100 scale)

**Options:**
- `period` (Integer, default=14) - Lookback period

**Inputs:**
- Requires: high, low, close, volume

**Outputs:**
- `result` - MFI value (0-100)

**Interpretation:**
- Similar to RSI but incorporates volume
- **< 20**: Oversold with volume confirmation
- **> 80**: Overbought with volume confirmation

**Use Cases:**
- Volume-confirmed overbought/oversold
- Divergence with price (volume weakness)
- Accumulation/distribution detection

**Example:**
```python
src = market_data_source()
mfi_val = mfi(period=14)(src.h, src.l, src.c, src.v)

# Strong oversold with volume
volume_oversold = mfi_val < 20

# Volume divergence (price up, MFI down)
price_higher = src.c > src.c[10]
mfi_lower = mfi_val < mfi_val[10]
bearish_divergence = price_higher and mfi_lower
```

### willr

**Purpose:** Williams %R (momentum oscillator, -100 to 0 scale)

**Options:**
- `period` (Integer, default=14) - Lookback period

**Inputs:**
- Requires: high, low, close

**Outputs:**
- `result` - Williams %R (-100 to 0)

**Interpretation:**
- **> -20**: Overbought
- **< -80**: Oversold
- Inverted scale (0 = high, -100 = low)

**Use Cases:**
- Fast momentum changes
- Entry timing in trends
- Reversal detection at extremes

**Example:**
```python
src = market_data_source()
willr_val = willr(period=14)(src.h, src.l, src.c)

# Oversold bounce
oversold = willr_val < -80
bounce = crossover(willr_val, -80)
entry = oversold and bounce
```

### roc

**Purpose:** Rate of Change (percentage change over N periods)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - Percentage change

**Use Cases:**
- Momentum measurement
- Relative strength rankings
- Trend strength quantification

**Example:**
```python
src = market_data_source()
momentum_20 = roc(period=20)(src.c)

# Strong momentum
strong_bull = momentum_20 > 5.0  # +5% in 20 periods
strong_bear = momentum_20 < -5.0
```

---

## Volatility Analysis

### atr

**Purpose:** Average True Range (volatility measure in price units)

**Options:**
- `period` (Integer, default=14) - Lookback period

**Inputs:**
- Requires: high, low, close

**Outputs:**
- `result` - ATR value

**Use Cases:**
- Stop-loss placement (e.g., 2x ATR)
- Position sizing (scale inversely with volatility)
- Breakout confirmation (volume + ATR expansion)
- Volatility regime detection

**Example:**
```python
src = market_data_source()
atr_val = atr(period=14)(src.h, src.l, src.c)

# Stop-loss at 2 ATR below entry
entry_price = src.c
stop_loss = entry_price - (2 * atr_val)

# Volatility expansion
vol_expanding = atr_val > atr_val[5]
```

**Typical values:** 0.5-2% of price for stocks, varies by asset class

### bbands

**Purpose:** Bollinger Bands (volatility envelope around moving average)

**Options:**
- `period` (Integer, default=20) - SMA period
- `stddev` (Decimal, default=2.0) - Standard deviations

**Inputs:**
- Single numeric series

**Outputs:**
- `bbands_lower` - Lower band (SMA - stddev * σ)
- `bbands_middle` - Middle band (SMA)
- `bbands_upper` - Upper band (SMA + stddev * σ)

**Use Cases:**
- Mean reversion (price touches bands)
- Breakout detection (squeeze then expansion)
- Volatility regime (band width)
- Relative price position (%B)

**Example:**
```python
src = market_data_source()
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# Mean reversion at bands
oversold = src.c < lower
overbought = src.c > upper

# Breakout from bands
bullish_breakout = crossover(src.c, upper)

# Walking the bands (strong trend)
walking_upper = src.c > middle and (src.c > upper[1])
```

### bband_percent

**Purpose:** Bollinger %B (position within bands, 0-1 normalized)

**Options:** None (operates on existing Bollinger Bands)

**Inputs:**
- Requires: lower, middle, upper (from bbands)

**Outputs:**
- `result` - %B value (0 = lower band, 1 = upper band, 0.5 = middle)

**Interpretation:**
- **< 0**: Below lower band
- **0-0.2**: Near lower band (oversold)
- **0.4-0.6**: Mid-range (neutral)
- **0.8-1.0**: Near upper band (overbought)
- **> 1**: Above upper band

**Use Cases:**
- Normalized overbought/oversold
- Mean reversion entries
- Relative strength comparison across assets

**Example:**
```python
src = market_data_source()
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
bb_percent = bband_percent()(lower, middle, upper)

# Mean reversion
extreme_oversold = bb_percent < 0.1
extreme_overbought = bb_percent > 0.9

# Trend strength (consistently high %B in uptrend)
strong_uptrend = bb_percent > 0.7
```

### bband_width

**Purpose:** Bollinger Band Width (volatility squeeze detection)

**Options:** None

**Inputs:**
- Requires: lower, middle, upper (from bbands)

**Outputs:**
- `result` - Band width (normalized)

**Use Cases:**
- Volatility squeeze (low width → breakout imminent)
- Volatility expansion (high width → trend in progress)
- Market regime classification

**Example:**
```python
src = market_data_source()
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
width = bband_width()(lower, middle, upper)

# Squeeze detection (width at 6-month low)
lowest_width = width == min(width[120])  # Squeeze
expansion = width > width[1]  # Breakout starting

# Trade breakout after squeeze
squeeze_ready = width < 1.5  # Threshold varies by asset
breakout = crossover(src.c, upper)
entry = squeeze_ready[1] and breakout
```

### ulcer_index

**Purpose:** Ulcer Index (downside volatility, drawdown-based risk)

**Options:**
- `period` (Integer, default=14) - Lookback period
- `use_sum` (Boolean, default=False) - Use sum instead of RMS

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - Ulcer Index value

**Use Cases:**
- Downside risk measurement
- Portfolio stress testing
- Risk-adjusted position sizing
- Bear market detection

**Example:**
```python
src = market_data_source()
ulcer = ulcer_index(period=20)(src.c)

# High drawdown risk
elevated_risk = ulcer > ulcer[60]  # Above 60-day average

# Reduce position in high-risk regimes
position_scale = 1.0 / (1.0 + ulcer)
```

### volatility (Historical Volatility)

**Purpose:** Annualized historical volatility (standard deviation of returns)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - Annualized volatility (%)

**Use Cases:**
- Option pricing inputs
- Risk management
- Volatility regime detection
- Position sizing

**Example:**
```python
src = market_data_source()
vol_20 = volatility(period=20)(src.c)
vol_60 = volatility(period=60)(src.c)

# Volatility regime shift
vol_spike = vol_20 > (vol_60 * 1.5)

# Low vol environment (good for trend-following)
low_vol = vol_20 < 15  # 15% annualized
```

---

## Volume Analysis

### obv

**Purpose:** On-Balance Volume (cumulative volume indicator)

**Options:** None

**Inputs:**
- Requires: close, volume

**Outputs:**
- `result` - OBV value (cumulative)

**Use Cases:**
- Volume trend confirmation
- Divergence detection (price vs OBV)
- Accumulation/distribution phases
- Breakout confirmation

**Example:**
```python
src = market_data_source()
obv_val = obv()(src.c, src.v)

# Volume trend
obv_rising = obv_val > obv_val[5]

# Divergence (price makes new high, OBV doesn't)
price_high = src.c > src.c[20]
obv_lower = obv_val < obv_val[20]
bearish_divergence = price_high and obv_lower
```

### ad

**Purpose:** Accumulation/Distribution Line (volume-weighted price indicator)

**Options:** None

**Inputs:**
- Requires: high, low, close, volume

**Outputs:**
- `result` - A/D line value

**Use Cases:**
- Smart money flow detection
- Divergence analysis
- Trend confirmation
- Distribution detection in tops

**Example:**
```python
src = market_data_source()
ad_line = ad()(src.h, src.l, src.c, src.v)

# Rising A/D confirms uptrend
ad_confirming = ad_line > ad_line[10]

# Distribution (price up, A/D down)
price_up = src.c > src.c[10]
ad_down = ad_line < ad_line[10]
distribution = price_up and ad_down
```

### adosc

**Purpose:** A/D Oscillator (difference of two EMAs of A/D line)

**Options:**
- `fast` (Integer, default=3) - Fast EMA period
- `slow` (Integer, default=10) - Slow EMA period

**Inputs:**
- Requires: high, low, close, volume

**Outputs:**
- `result` - A/D Oscillator value

**Use Cases:**
- Momentum of accumulation/distribution
- Zero-line crosses (trend changes)
- Divergence detection

**Example:**
```python
src = market_data_source()
adosc_val = adosc(fast=3, slow=10)(src.h, src.l, src.c, src.v)

# Accumulation phase
accumulation = adosc_val > 0

# Momentum shift
cross_up = crossover(adosc_val, 0)
```

---

## Comparison & Logic

### crossover

**Purpose:** Detect when first series crosses above second

**Options:** None

**Inputs:**
- Two numeric series

**Outputs:**
- `result` - Boolean (True on crossover bar only)

**Use Cases:**
- Moving average crosses
- Indicator threshold crosses
- Signal line crosses

**Example:**
```python
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Golden cross
golden_cross = crossover(fast, slow)

# RSI crosses above 50
rsi_val = rsi(period=14)(src.c)
rsi_bullish = crossover(rsi_val, 50)
```

### crossunder

**Purpose:** Detect when first series crosses below second

**Options:** None

**Inputs:**
- Two numeric series

**Outputs:**
- `result` - Boolean (True on crossunder bar only)

**Use Cases:**
- Bearish crossovers
- Stop-loss triggers
- Exit signals

**Example:**
```python
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)

# Death cross
death_cross = crossunder(fast, slow)

# Price crosses below support
support = sma(period=50)(src.c)
breakdown = crossunder(src.c, support)
```

### first_non_null

**Purpose:** Return first non-null value (coalesce, fallback chain)

**Options:** None

**Inputs:**
- Multiple values (any type)

**Outputs:**
- `result` - First non-null value, or null if all null

**Use Cases:**
- Missing data handling
- Multi-source data selection
- Default value fallbacks
- Indicator failover

**Example:**
```python
# Data quality: use adjusted if available, else raw
clean_price = first_non_null(adjusted_close, raw_close, 0.0)

# Multi-level fallback
best_estimate = first_non_null(
    real_time_price,
    delayed_quote,
    last_settlement
)

# Handle missing indicators
safe_rsi = first_non_null(rsi_14, rsi_20, 50.0)  # Default neutral
```

### conditional_select

**Purpose:** Multi-way conditional (SQL CASE WHEN, cleaner than nested ternary)

**Options:** None

**Inputs:**
- Alternating: condition1, value1, condition2, value2, ..., [default]

**Outputs:**
- `result` - Value from first matching condition, or default

**Use Cases:**
- Market regime classification
- Multi-level signal strength
- Complex decision trees
- Replacing nested ternary expressions

**Example:**
```python
# Market regime
rsi_val = rsi(period=14)(src.c)
regime = conditional_select(
    rsi_val < 30, "oversold",
    rsi_val > 70, "overbought",
    "neutral"
)

# Position sizing by confidence
trend_up = ema_fast > ema_slow
rsi_ok = rsi_val < 40
volume_high = volume > avg_volume * 1.5

position_size = conditional_select(
    (trend_up and rsi_ok and volume_high), 3,  # Strong
    (trend_up and rsi_ok), 2,                  # Moderate
    trend_up, 1,                               # Weak
    0                                          # No position
)

# Much clearer than nested ternary
```

### Comparison Operators

**Purpose:** Compare values

**Available:**
- `gt(a, b)` - Greater than (>)
- `gte(a, b)` - Greater than or equal (>=)
- `lt(a, b)` - Less than (<)
- `lte(a, b)` - Less than or equal (<=)
- `eq(a, b)` - Equal (==)
- `neq(a, b)` - Not equal (!=)

**Outputs:**
- Boolean result

**Note:** Typically use operator syntax (`a > b`) instead of function calls. These exist for programmatic construction.

**Example:**
```python
# Prefer operators
is_high = close > 100

# Function form (rarely needed)
is_high = gt(close, 100)
```

### Aggregate Operators

**Purpose:** Combine multiple values

**Available:**
- `agg_sum` - Sum of inputs
- `agg_mean` - Average of inputs
- `agg_min` - Minimum value
- `agg_max` - Maximum value
- `agg_all_of` - All inputs true (logical AND)
- `agg_any_of` - At least one input true (logical OR)
- `agg_none_of` - No inputs true

**Example:**
```python
# Combine multiple indicators
signal1 = rsi_val < 30
signal2 = macd_bullish
signal3 = volume_high

# All must be true
strong_buy = agg_all_of(signal1, signal2, signal3)

# Any true
any_signal = agg_any_of(signal1, signal2, signal3)

# Average of multiple RSI periods
rsi_14 = rsi(period=14)(close)
rsi_21 = rsi(period=21)(close)
avg_rsi = agg_mean(rsi_14, rsi_21)
```

---

## Statistical Analysis

### rolling_corr

**Purpose:** Rolling Pearson correlation between two series (-1 to +1)

**Options:**
- `window` (Integer, required) - Rolling window size

**Inputs:**
- `x` - First series
- `y` - Second series

**Outputs:**
- `correlation` - Correlation coefficient (-1 to +1)

**Interpretation:**
- **+1**: Perfect positive correlation
- **0**: No linear relationship
- **-1**: Perfect negative correlation

**Use Cases:**
- Pairs trading (find correlated assets)
- Lead-lag analysis
- Hedge effectiveness
- Market regime detection (correlation breakdown)

**Example:**
```python
# Pairs trading
asset1 = market_data_source()
asset2 = market_data_source()

corr = rolling_corr(window=20)(x=asset1.c, y=asset2.c)

# Trade when correlation is high
high_corr = corr > 0.8

# Detect correlation breakdown (pair divergence)
corr_breakdown = corr < 0.5
```

### rolling_cov

**Purpose:** Rolling covariance between two series

**Options:**
- `window` (Integer, required) - Rolling window size

**Inputs:**
- `x` - First series
- `y` - Second series

**Outputs:**
- `covariance` - Covariance value

**Use Cases:**
- Beta calculation (cov(stock, market) / var(market))
- Portfolio variance
- Risk attribution
- Multi-asset volatility

**Example:**
```python
stock = market_data_source()
market = market_data_source()  # SPY or index

# Calculate beta
cov_stock_market = rolling_cov(window=60)(x=stock.c, y=market.c)
var_market = rolling_cov(window=60)(x=market.c, y=market.c)
beta = cov_stock_market / var_market

# High beta = more market-sensitive
high_beta = beta > 1.5
```

### ewm_corr

**Purpose:** Exponentially weighted moving correlation (more responsive)

**Options:**
- `span` (Integer, required) - Decay span (decay = 2/(span+1))

**Inputs:**
- `x` - First series
- `y` - Second series

**Outputs:**
- `correlation` - EWM correlation (-1 to +1)

**Use Cases:**
- Adaptive hedge ratios
- Real-time correlation monitoring
- Regime change detection
- Dynamic risk management

**Example:**
```python
stock = market_data_source()
hedge = market_data_source()

# Fast and slow correlations
fast_corr = ewm_corr(span=10)(x=stock.c, y=hedge.c)
slow_corr = ewm_corr(span=50)(x=stock.c, y=hedge.c)

# Correlation regime shift
corr_divergence = fast_corr - slow_corr
decoupling = corr_divergence < -0.2  # Asset becoming independent
```

### ewm_cov

**Purpose:** Exponentially weighted moving covariance

**Options:**
- `span` (Integer, required) - Decay span

**Inputs:**
- `x` - First series
- `y` - Second series

**Outputs:**
- `covariance` - EWM covariance

**Use Cases:**
- Dynamic beta calculation
- Adaptive portfolio variance
- Risk parity weighting
- GARCH-style volatility

**Example:**
```python
stock = market_data_source()
market = market_data_source()

# Dynamic beta (adapts quickly)
ewm_cov_val = ewm_cov(span=20)(x=stock.c, y=market.c)
ewm_var_market = ewm_cov(span=20)(x=market.c, y=market.c)
dynamic_beta = ewm_cov_val / ewm_var_market

# Adjust position based on beta regime
low_beta = dynamic_beta < 0.8   # Defensive
high_beta = dynamic_beta > 1.2  # Aggressive
```

### zscore

**Purpose:** Z-score (standardized score, measures standard deviations from mean)

**Options:**
- `period` (Integer, required) - Lookback period

**Inputs:**
- Single numeric series

**Outputs:**
- `result` - Z-score value (typically -3 to +3)

**Interpretation:**
- **0**: At mean
- **+2**: 2 standard deviations above mean (expensive)
- **-2**: 2 standard deviations below mean (cheap)
- **±3**: Extreme deviation

**Use Cases:**
- Mean reversion entries
- Relative value analysis
- Statistical arbitrage
- Pairs trading (z-score of spread)

**Example:**
```python
src = market_data_source()

# Price z-score (mean reversion)
z = zscore(period=20)(src.c)

# Extreme oversold
extreme_low = z < -2.5
entry = extreme_low

# Exit at mean
exit = abs(z) < 0.5

# Pairs trading spread z-score
spread = asset1.c - asset2.c
spread_z = zscore(period=20)(spread)
trade_spread = abs(spread_z) > 2.0
```

---

## Market Microstructure

### session_gap

**Purpose:** Detect overnight gaps in intraday data

**Options:**
- `fill_percent` (Decimal, default=100) - % of gap that must fill
- `timeframe` (String, required) - Intraday timeframe (e.g., "1Min")

**Inputs:** None (uses internal market data)

**Outputs:**
- `gap_filled` - Boolean: True when gap fills
- `gap_retrace` - Decimal: % of gap retraced
- `gap_size` - Decimal: Gap size (% or price units)
- `psc` - Decimal: Previous session close
- `psc_timestamp` - Timestamp: Previous close time

**Use Cases:**
- Gap fade strategies (bet on mean reversion)
- Gap continuation strategies (breakouts)
- Intraday risk assessment
- Research on gap fill behavior

**Example:**
```python
# Detect overnight gaps with 1-minute data
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Gap-fill strategy
gap_exists = gaps.gap_size > 1.0  # 1% gap
gap_filled = gaps.gap_filled

# Fade large gaps (mean reversion)
fade_entry = gap_exists and not gap_filled
exit = gap_filled
```

### bar_gap

**Purpose:** Detect intraday bar-to-bar gaps (halts, liquidity events)

**Options:**
- `fill_percent` (Decimal, default=100) - % of gap that must fill
- `min_gap_size` (Decimal, required) - Minimum gap size to detect
  - Stocks: 2.0% typical
  - FX: 0.04% (4 pips)
- `timeframe` (String, required) - Timeframe

**Inputs:** None (uses internal market data)

**Outputs:**
- `gap_filled` - Boolean: Gap filled
- `gap_retrace` - Decimal: % retraced
- `gap_size` - Decimal: Gap size
- `psc` - Decimal: Previous close
- `psc_timestamp` - Timestamp: Previous timestamp

**Use Cases:**
- Trading halt detection
- Liquidity gap analysis
- News event detection
- Intraday volatility spikes

**Example:**
```python
# Detect 2% intraday gaps (trading halts, news)
gaps = bar_gap(fill_percent=100, min_gap_size=2.0, timeframe="1Min")()

# Trade gap continuation (assume news)
large_gap = gaps.gap_size > 3.0
continuation = large_gap and not gaps.gap_filled

# FX pip gaps (4 pips = 0.04%)
fx_gaps = bar_gap(fill_percent=100, min_gap_size=0.04, timeframe="1Min")()
```

### sessions

**Purpose:** Detect trading sessions and track session high/low

**Options:**
- `session_type` (String, required) - Session name

**Inputs:** None (uses internal timestamp + OHLC)

**Outputs:**
- `active` - Boolean: True during session
- `high` - Decimal: Session high price
- `low` - Decimal: Session low price
- `opened` - Boolean: True on session open bar
- `closed` - Boolean: True on session close bar

**Available Sessions:**
- Geographic: "Sydney", "Tokyo", "London", "NewYork"
- Kill Zones: "AsianKillZone", "LondonOpenKillZone", "NewYorkKillZone", "LondonCloseKillZone"

**Use Cases:**
- Session-based strategies
- Opening range breakouts
- Session high/low breakouts
- Time-of-day filters

**Example:**
```python
src = market_data_source()
london = sessions(session_type="London")()

# Only trade during London session
in_session = london.active
entry = bullish_signal and in_session

# Breakout from session range
breakout_high = src.c > london.high
breakout_low = src.c < london.low

# Exit at session close
exit = london.closed
```

### swing_highs_lows

**Purpose:** Detect swing points (local extrema)

**Options:**
- `left_count` (Integer, default=5) - Bars to left of pivot
- `right_count` (Integer, default=5) - Bars to right of pivot

**Inputs:** None (uses internal OHLC)

**Outputs:**
- `swing_high` - Boolean: True at swing high
- `swing_low` - Boolean: True at swing low
- `swing_high_price` - Decimal: High price
- `swing_low_price` - Decimal: Low price

**Use Cases:**
- Support/resistance identification
- Trendline construction
- Structure analysis (higher highs/lows)
- Stop placement

**Example:**
```python
shl = swing_highs_lows(left_count=5, right_count=5)()

# Recent swing high as resistance
resistance = shl.swing_high_price

# Break of structure (new high above last swing high)
new_high = src.c > shl.swing_high_price

# Support at last swing low
support = shl.swing_low_price
```

---

## Chart Patterns & Price Action

### order_blocks

**Purpose:** Smart Money Concepts - Order blocks (institutional supply/demand zones)

**Options:** None

**Inputs:**
- Requires: open, high, low, close

**Outputs:**
- `bullish` - Boolean: Bullish order block detected
- `bearish` - Boolean: Bearish order block detected
- `bullish_ob_high` - Decimal: Bullish OB high
- `bullish_ob_low` - Decimal: Bullish OB low
- `bearish_ob_high` - Decimal: Bearish OB high
- `bearish_ob_low` - Decimal: Bearish OB low

**Use Cases:**
- SMC entry zones
- Institutional support/resistance
- Retest entries
- Confluence with other SMC concepts

**Example:**
```python
src = market_data_source()
ob = order_blocks()(src.o, src.h, src.l, src.c)

# Entry on bullish order block
bullish_zone = ob.bullish
entry = bullish_zone and (src.c > ob.bullish_ob_high)

# Stop below order block
stop = ob.bullish_ob_low
```

### fair_value_gap

**Purpose:** SMC - Fair Value Gaps (FVG, imbalances for potential retracement)

**Options:** None

**Inputs:**
- Requires: open, high, low, close

**Outputs:**
- `bullish` - Boolean: Bullish FVG detected
- `bearish` - Boolean: Bearish FVG detected
- `bullish_filled` - Boolean: Bullish FVG filled (retraced)
- `bearish_filled` - Boolean: Bearish FVG filled
- `fvg_high` - Decimal: FVG high boundary
- `fvg_low` - Decimal: FVG low boundary

**Use Cases:**
- Retracement entry zones
- Gap-fill targets
- Confluence with order blocks
- Liquidity analysis

**Example:**
```python
src = market_data_source()
fvg = fair_value_gap()(src.o, src.h, src.l, src.c)

# Bullish FVG fill entry
bullish_fvg = fvg.bullish
fvg_retest = fvg.bullish_filled
entry = bullish_fvg and fvg_retest

# Target: opposite side of FVG
target = fvg.fvg_high
```

### head_and_shoulders

**Purpose:** Classic reversal pattern detection

**Options:**
- `tolerance` (Decimal, default=0.02) - Price tolerance (2%)
- `min_pattern_bars` (Integer, default=20) - Minimum bars for pattern

**Inputs:** None (uses internal OHLC)

**Outputs:**
- `pattern_detected` - Boolean: H&S pattern complete
- `neckline` - Decimal: Neckline price
- `target` - Decimal: Projected target (neckline - head height)

**Use Cases:**
- Top reversal detection
- Bearish breakdowns
- Swing trading entries

**Example:**
```python
hs = head_and_shoulders(tolerance=0.02)()

# Pattern complete and neckline broken
pattern_complete = hs.pattern_detected
breakdown = src.c < hs.neckline

entry_short = pattern_complete and breakdown
target = hs.target
```

### triangles

**Purpose:** Continuation/reversal triangle patterns

**Options:**
- `min_pattern_bars` (Integer, default=15) - Minimum pattern duration
- `tolerance` (Decimal, default=0.015) - Trendline tolerance

**Inputs:** None (uses internal OHLC)

**Outputs:**
- `ascending` - Boolean: Ascending triangle
- `descending` - Boolean: Descending triangle
- `symmetrical` - Boolean: Symmetrical triangle
- `upper_line` - Decimal: Upper trendline
- `lower_line` - Decimal: Lower trendline
- `breakout` - Boolean: Breakout detected

**Use Cases:**
- Consolidation breakouts
- Trend continuation
- Volatility compression trades

**Example:**
```python
tri = triangles(min_pattern_bars=15)()

# Ascending triangle breakout (bullish)
asc_breakout = tri.ascending and tri.breakout and (src.c > tri.upper_line)

# Symmetrical triangle (wait for direction)
sym = tri.symmetrical
breakout_up = sym and (src.c > tri.upper_line)
breakout_down = sym and (src.c < tri.lower_line)
```

---

## Calendar Effects

### turn_of_month

**Purpose:** Turn-of-month effect (equity inflows at month end)

**Options:**
- `days_before` (Integer, default=3) - Days before month end
- `days_after` (Integer, default=5) - Days after month start

**Inputs:** None (uses internal timestamp)

**Outputs:**
- `in_window` - Boolean: True during turn-of-month window

**Use Cases:**
- Exploit monthly equity inflows
- Portfolio rebalancing timing
- Seasonal pattern trading

**Example:**
```python
tom = turn_of_month(days_before=3, days_after=5)()

# Only trade during turn-of-month
in_tom = tom.in_window
entry = bullish_signal and in_tom

# Exit after window closes
exit = not in_tom
```

### day_of_week

**Purpose:** Day-of-week effects (Monday effect, Friday effect)

**Options:**
- `target_day` (String, required) - "Monday", "Tuesday", ..., "Friday"

**Inputs:** None (uses internal timestamp)

**Outputs:**
- `is_target_day` - Boolean: True on specified day

**Use Cases:**
- Monday reversals
- Friday momentum
- Weekend position management
- Intraday patterns by day

**Example:**
```python
monday = day_of_week(target_day="Monday")()
friday = day_of_week(target_day="Friday")()

# Monday reversal (fade Friday move)
friday_up = friday.is_target_day and (src.c > src.o)
monday_fade = monday.is_target_day and friday_up[1]

# Friday momentum continuation
friday_continuation = friday.is_target_day and trend_up
```

### month_of_year

**Purpose:** Seasonal patterns (January effect, Sell in May, etc.)

**Options:**
- `target_month` (String, required) - "January", "February", ..., "December"

**Inputs:** None (uses internal timestamp)

**Outputs:**
- `is_target_month` - Boolean: True during specified month

**Use Cases:**
- January effect (small-cap outperformance)
- Sell in May and go away
- Holiday seasonality
- Tax-loss harvesting

**Example:**
```python
january = month_of_year(target_month="January")()
may = month_of_year(target_month="May")()

# January effect (increase small-cap exposure)
jan_effect = january.is_target_month

# Sell in May
reduce_exposure = may.is_target_month

# Summer doldrums (May-September)
summer = month_of_year(target_month="June")() or \
         month_of_year(target_month="July")() or \
         month_of_year(target_month="August")()
```

---

## Reporting

### gap_report

**Purpose:** Comprehensive gap analysis report (cards, charts, tables)

**Options:**
- `fill_time_pivot_hour` (Integer, required) - Hour to pivot table by
- `histogram_bins` (Integer, default=15) - Number of histogram bins

**Inputs:**
- `gap_filled` - Boolean series (from session_gap or bar_gap)
- `gap_retrace` - Decimal series
- `gap_size` - Decimal series
- `psc` - Decimal series (previous close)
- `psc_timestamp` - Timestamp series

**Outputs:** None (generates report)

**Use Cases:**
- Gap research analysis
- Fill behavior statistics
- Time-of-day patterns
- Gap size distribution

**Example:**
```python
# Overnight gap analysis
gaps = session_gap(fill_percent=100, timeframe="1Min")()

gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

**Report Components:**
- Cards: Fill rate, average size, median time to fill
- Histograms: Gap size distribution, fill time distribution
- Tables: Gaps by hour, gaps by day of week, gaps by size bucket

### event_marker

**Purpose:** Create timeline events for quick navigation to important signals and patterns

**Options:**
- `color_map` (Dict, required) - Map event names to colors (Success, Warning, Error, Info)

**Inputs:**
- Named boolean series for events (e.g., `breakout=breakout_signal`)

**Outputs:** None (generates scrollable event list)

**Use Cases:**
- Jump to RSI overbought/oversold conditions
- Navigate to all breakout signals
- Review all divergence patterns
- Inspect regime changes
- Analyze signal clusters

**Example:**
```python
# RSI extremes
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
overbought = rsi_val > 70

# Bollinger breakouts
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
upper_break = src.c > upper
lower_break = src.c < lower

# Create event markers
event_marker(color_map={
    Success: ["oversold", "lower_break"],    # Green
    Error: ["overbought", "upper_break"],    # Red
    Warning: ["regime_change"]               # Yellow
})(
    oversold=oversold,
    overbought=overbought,
    upper_break=upper_break,
    lower_break=lower_break,
    regime_change=regime_shift
)
```

**How It Works:**
1. Creates scrollable list in sidebar showing all event occurrences
2. Each event shows timestamp and name
3. Clicking event jumps chart to that timestamp
4. Color-coded by severity/type (Success, Warning, Error, Info)
5. Useful for reviewing signal quality and timing

**Color Categories:**
- **Success** (Green): Positive signals, entry opportunities, bullish patterns
- **Warning** (Yellow): Caution signals, regime changes, neutral patterns
- **Error** (Red): Exit signals, overbought conditions, bearish patterns
- **Info** (Blue): Informational events, milestones, reference points

### table_report

**Purpose:** SQL-like table visualization with aggregations

**Options:**
- `sql` (String, required) - SQL query (SELECT, FROM, WHERE, GROUP BY, ORDER BY)

**Inputs:**
- Named columns (referenced in SQL)

**Outputs:** None (generates table report)

**Use Cases:**
- Custom aggregations
- Pivot tables
- Statistical summaries
- Multi-dimensional analysis

**Example:**
```python
# Aggregate returns by day of week
returns = (src.c - src.c[1]) / src.c[1]
dow = day_of_week_number()  # 0-6

table_report(sql="""
    SELECT
        dow,
        AVG(returns) as avg_return,
        STDDEV(returns) as volatility,
        COUNT(*) as count
    FROM input
    GROUP BY dow
    ORDER BY dow
""")(dow=dow, returns=returns)
```

---

## Summary

This catalog covers the 80 essential transforms for institutional trading strategies. For the complete list of 300+ transforms, see [Appendix: Full Catalog](appendix-full-catalog.md).

### Transform Categories

- **Data & Execution**: 2 transforms
- **Trend**: 7 transforms
- **Momentum**: 9 transforms
- **Volatility**: 7 transforms
- **Volume**: 3 transforms
- **Comparison & Logic**: 7 transforms
- **Statistical**: 5 transforms
- **Market Microstructure**: 4 transforms
- **Chart Patterns**: 3 transforms
- **Calendar**: 3 transforms
- **Reporting**: 3 transforms (gap_report, event_marker, table_report)

**Total: ~80 curated transforms**

### Quick Reference by Use Case

**Trend Following:** ema, sma, hma, kama, adx, aroon
**Mean Reversion:** rsi, stoch, cci, bbands, zscore
**Breakout:** bbands, bband_width, atr, volume
**Momentum:** macd, roc, mfi, willr
**Volatility:** atr, bbands, ulcer_index, volatility
**Volume:** obv, ad, adosc, mfi
**Pairs Trading:** rolling_corr, rolling_cov, zscore
**Intraday:** sessions, session_gap, bar_gap
**SMC:** order_blocks, fair_value_gap, swing_highs_lows
**Patterns:** head_and_shoulders, triangles
**Calendar:** turn_of_month, day_of_week, month_of_year

---

**Next:** [Strategy Patterns →](../strategies/strategy-patterns.md)
