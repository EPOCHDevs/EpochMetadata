# Glossary

Technical terms and definitions used in Epoch script documentation.

---

## Language Terms

**Assignment Statement**
Statement that binds a value to a variable: `x = expression`

**Attribute Access**
Accessing transform outputs using dot notation: `src.c`, `bb.bbands_upper`

**Chaining**
Composing transforms together: `ema(period=10)(ema(period=20)(src.c))`

**Constant Folding**
Compiler optimization that evaluates constant expressions at compile time

**Declarative**
Programming paradigm where you describe *what* you want, not *how* to compute it

**Expression Statement**
Statement that calls a transform without assignment (sinks only): `trade_signal_executor()(enter_long=buy)`

**Immutable**
Cannot be changed after creation (applies to variables - single assignment only)

**Lag Operator**
Accessing historical values using bracket notation: `src.c[1]` (previous bar)

**Literal**
Fixed value in code: `42`, `3.14`, `"AAPL"`, `True`

**Sink**
Transform with no outputs (side effects only): `trade_signal_executor`, `gap_report`

**Source**
Transform with no inputs (data generation): `market_data_source`, `session_gap`

**Ternary Conditional**
Inline if/else expression: `value_if_true if condition else value_if_false`

**Transform**
Core building block - function with options, inputs, and outputs

**Tuple Unpacking**
Assigning multiple outputs to multiple variables: `lower, middle, upper = bbands(...)(input)`

**Type Inference**
Automatic determination of variable types by compiler

**Vectorized**
Operations applied to entire data series at once (not element-by-element)

---

## Trading Terms

**ADX (Average Directional Index)**
Trend strength indicator (0-100): <25 = weak, >25 = strong trend

**ATR (Average True Range)**
Volatility measure in price units (used for stops, position sizing)

**Breakout**
Price moving above resistance or below support

**Crossover / Crossunder**
When one line crosses above/below another (e.g., moving average cross)

**EMA (Exponential Moving Average)**
Weighted average giving more importance to recent prices

**Gap**
Price discontinuity between bars (overnight or intraday)

**MACD (Moving Average Convergence Divergence)**
Trend and momentum indicator (MACD line, signal line, histogram)

**Mean Reversion**
Strategy betting price returns to average after extreme moves

**Momentum**
Rate of price change (positive = upward, negative = downward)

**Overbought / Oversold**
Extreme price conditions (e.g., RSI > 70 overbought, < 30 oversold)

**RSI (Relative Strength Index)**
Momentum oscillator (0-100 scale, measures speed and magnitude of price changes)

**SMA (Simple Moving Average)**
Unweighted average of N periods

**Squeeze**
Low volatility period (e.g., Bollinger Band width contraction)

**Trend Following**
Strategy that enters in direction of prevailing trend

**Volatility**
Measure of price fluctuation (higher = more movement)

---

## Market Microstructure Terms

**Asian Kill Zone**
High-liquidity period during Asian session (19:00-23:00 ET)

**BOS (Break of Structure)**
SMC concept: Price breaking previous high/low (trend continuation)

**CHOCH (Change of Character)**
SMC concept: Shift in market structure (potential reversal)

**Fair Value Gap (FVG)**
SMC concept: Price imbalance between three candles (gap for retracement)

**Intraday**
Within a single trading day (minute, hour bars)

**Kill Zone**
ICT concept: High-liquidity time windows with institutional activity

**Liquidity**
Ease of buying/selling without moving price (also SMC: stop hunts, sweeps)

**London Kill Zone**
High-liquidity period during London open (02:00-05:00 ET)

**Market Session**
Geographic trading hours: Sydney, Tokyo, London, New York

**New York Kill Zone**
High-liquidity period during NY morning (07:00-10:00 ET)

**Order Block**
SMC concept: Last opposing candle before strong move (institutional supply/demand zone)

**PSC (Previous Session Close)**
Closing price of previous trading session

**Session Gap**
Price gap between previous session close and current session open

**SMC (Smart Money Concepts)**
Trading methodology focused on institutional order flow

**Swing High / Swing Low**
Local maximum/minimum (pivot point)

---

## TimeFrame Terms

**Base TimeFrame**
TimeFrame on which strategy runs (e.g., 15Min, 1H, 1D)

**Business Day**
Trading day (excludes weekends/holidays): `"1B"`, `"5B"`

**Higher TimeFrame**
Longer period than base (e.g., 1D when strategy runs on 1H)

**Intraday TimeFrame**
Sub-daily resolution: `"1Min"`, `"5Min"`, `"15Min"`, `"30Min"`, `"1H"`, `"4H"`

**Lower TimeFrame**
Shorter period than base (less common)

**Multi-TimeFrame**
Using multiple resolutions in one strategy (e.g., daily trend, hourly entry)

**Resampling**
Aggregating data to different timeframe (e.g., 1Min → 1H)

**TimeFrame**
Data resolution or aggregation period: `"1Min"`, `"1H"`, `"1D"`, `"1W"`, `"1M"`

---

## Statistical Terms

**Beta**
Sensitivity to market moves (beta = cov(stock, market) / var(market))

**Correlation**
Measure of linear relationship (-1 to +1): +1 = perfect positive, -1 = perfect negative, 0 = no relationship

**Covariance**
Joint variability of two variables (scale-dependent, unlike correlation)

**EWM (Exponentially Weighted Moving)**
Weighting scheme giving more importance to recent observations

**Lead-Lag**
When one series predicts another (e.g., crypto vol leads tech drawdowns)

**Rolling Window**
Fixed-size sliding window for calculations (e.g., rolling 20-day correlation)

**Standard Deviation**
Measure of dispersion (σ, used in Bollinger Bands)

**Volatility (Historical)**
Standard deviation of returns (annualized)

**Z-Score**
Number of standard deviations from mean: (value - mean) / stddev

---

## Quantitative Terms

**Alpha**
Return above benchmark (skill-based return)

**Beta Hedging**
Adjusting exposure to market risk based on beta

**Cross-Sectional**
Analysis across multiple assets at same point in time (vs time-series)

**Factor**
Characteristic used to rank/select assets (momentum, value, quality)

**Look-Ahead Bias**
Using future data in historical analysis (fatal error in backtesting)

**Pairs Trading**
Statistical arbitrage: Long undervalued asset, short overvalued asset in correlated pair

**Regime**
Market state (trending, ranging, high vol, low vol)

**Sharpe Ratio**
Risk-adjusted return: (return - risk_free) / volatility

**Statistical Arbitrage**
Trading based on statistical relationships (mean reversion of spread, correlation)

**Universe**
Set of assets for cross-sectional analysis (e.g., S&P 500)

---

## Calendar Terms

**Day-of-Week Effect**
Pattern where returns vary by weekday (Monday effect, Friday effect)

**January Effect**
Seasonal pattern of small-cap outperformance in January

**Month-of-Year Effect**
Seasonal patterns by calendar month

**Sell in May**
Adage: "Sell in May and go away" (summer underperformance)

**Turn-of-Month**
Period around month-end (equity inflows, rebalancing)

---

## Technical Terms

**Card Report**
Visualization displaying single metrics (numeric, boolean, quantile)

**Chart Report**
Visualization: bar chart, pie chart, histogram, line chart

**Auto-Plotting**
Automatic visualization of every indicator without manual configuration

**Event Marker**
Timeline navigation system for jumping to important signals and patterns

**Research Dashboard**
Interactive analysis interface for gap reports, tables, and charts (separate from trading charts)

**Sidebar Cards**
Real-time metrics display organized in card groups (numeric, boolean, quantile)

**Compiler**
Program that translates Epoch script to executable format

**Handle**
Named output of a transform: `c` (close), `bbands_upper`, etc.

**Option**
Transform parameter: `period=20`, `stddev=2`

**Reporter**
Transform that generates visualizations (sink)

**Runtime**
Execution environment for compiled strategies

**Table Report**
Tabular data visualization with SQL-like queries

**Type Casting**
Converting between types (automatic: bool ↔ number)

**Type System**
Rules governing data types (Boolean, Integer, Decimal, String, etc.)

---

## Fundamental Terms

**Balance Sheet**
Financial statement: assets, liabilities, equity

**Cash Flow Statement**
Financial statement: operating, investing, financing cash flows

**EPS (Earnings Per Share)**
Net income divided by shares outstanding

**Financial Ratio**
Calculated metric: P/E, P/B, ROE, debt ratios

**Income Statement**
Financial statement: revenue, expenses, earnings

**P/E (Price-to-Earnings)**
Valuation: stock price / earnings per share

**ROE (Return on Equity)**
Profitability: net income / shareholder equity

---

## Abbreviations

**ADX** - Average Directional Index
**ATR** - Average True Range
**BOS** - Break of Structure
**CHOCH** - Change of Character
**EMA** - Exponential Moving Average
**EPS** - Earnings Per Share
**EWM** - Exponentially Weighted Moving
**FRED** - Federal Reserve Economic Data
**FVG** - Fair Value Gap
**FX** - Foreign Exchange (forex)
**HMA** - Hull Moving Average
**ICT** - Inner Circle Trader
**KAMA** - Kaufman Adaptive Moving Average
**MACD** - Moving Average Convergence Divergence
**ML** - Machine Learning
**OHLCV** - Open, High, Low, Close, Volume
**P/B** - Price-to-Book
**P/E** - Price-to-Earnings
**PSC** - Previous Session Close
**ROE** - Return on Equity
**RSI** - Relative Strength Index
**SMA** - Simple Moving Average
**SMC** - Smart Money Concepts
**VWAP** - Volume Weighted Average Price

---

## Quick Lookup

**Common Thresholds:**
- RSI oversold: < 30
- RSI overbought: > 70
- ADX trending: > 25
- Bollinger %B oversold: < 0.2
- Bollinger %B overbought: > 0.8
- Correlation strong: > 0.7 or < -0.7

**Common Periods:**
- Short-term MA: 10-20
- Medium-term MA: 50-100
- Long-term MA: 200+
- RSI standard: 14
- ATR standard: 14
- Bollinger Bands: 20-period, 2 stddev
- MACD: 12, 26, 9

**TimeFrame Aliases:**
- 1 hour = `"1H"`
- 1 day = `"1D"`
- 1 week = `"1W"`
- 1 month = `"1M"`

---

**Back to:** [Index](index.md)
